#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string.h>
#include "engine/vk/resources/descriptors.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/texture.h"
#include "engine/vk/resources/buffer.h"
#include "iostream/logger_bridge.h"

static VkDescriptorSet  g_sets[VK_DESCRIPTOR_MAX];
static int              g_set_count = 0;

/* One small material-scalars UBO per textured material set.
 * VK_NULL_HANDLE for sets that never write binding 2 (UI, whose sets share this same registry/layout). */
static VkBuffer         g_material_bufs[VK_DESCRIPTOR_MAX];
static VkDeviceMemory   g_material_mems[VK_DESCRIPTOR_MAX];

int vk_descriptors_init_layout(void)
{
    VkDescriptorSetLayoutBinding bindings[3]    = {0}; // A binding is a single "slot" in a descriptor set, which can be a texture, sampler, or UBO.
    // binding 0 = diffuse texture, 
    bindings[0].binding                         = 0;
    bindings[0].descriptorType                  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount                 = 1;
    bindings[0].stageFlags                      = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // binding 1 = bump texture,
    bindings[1].binding                         = 1;
    bindings[1].descriptorType                  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount                 = 1;
    bindings[1].stageFlags                      = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    // binding 2 = small per-material UBO (Kd/Ks/Ns)
    bindings[2].binding                         = 2;
    bindings[2].descriptorType                  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // small UBO with Kd/Ks/Ns, different from othr
    bindings[2].descriptorCount                 = 1;
    bindings[2].stageFlags                      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslci   = {0};
    dslci.sType                             = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslci.bindingCount                      = 3;                // yes.
    dslci.pBindings                         = bindings;
    if (vkCreateDescriptorSetLayout(g_device, &dslci, NULL, &g_descriptor_set_layout) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_init_layout: vkCreateDescriptorSetLayout failed");
        return 0;
    }

    // A pool size is a count of how many descriptors of a given type can be allocated from the pool. we have 2 different type of descriptors (image+sampler, and UBO).
    VkDescriptorPoolSize psizes[2]  = {0};
    psizes[0].type                  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    psizes[0].descriptorCount       = VK_DESCRIPTOR_MAX * 2;
    psizes[1].type                  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    psizes[1].descriptorCount       = VK_DESCRIPTOR_MAX;

    VkDescriptorPoolCreateInfo dpci = {0};
    dpci.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets                    = VK_DESCRIPTOR_MAX;
    dpci.poolSizeCount              = 2;
    dpci.pPoolSizes                 = psizes;
    if (vkCreateDescriptorPool(g_device, &dpci, NULL, &g_descriptor_pool) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_init_layout: vkCreateDescriptorPool failed");
        return 0;
    }

    ftpp_logf(FTPP_LOG_DEBUG, "vk_descriptors_init_layout: layout+pool ready (max %d sets)", VK_DESCRIPTOR_MAX);
    return 1;
}

int vk_descriptors_create_for_textures(int diffuse_texture_handle, int bump_texture_handle, const float material_ubo[8])
{
    if (g_set_count >= VK_DESCRIPTOR_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_textures: registry full (max %d)", VK_DESCRIPTOR_MAX);
        return -1;
    }
    VkImageView diffuse_view    = vk_texture_get_view(diffuse_texture_handle);
    VkImageView bump_view       = vk_texture_get_view(bump_texture_handle);
    if (diffuse_view == VK_NULL_HANDLE || bump_view == VK_NULL_HANDLE) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_textures: invalid texture handle(s)");
        return -1;
    }

        // query the allocate info for MATERIAL UBO. (we already did for the texture with vk_texture_get_view.)
        VkDescriptorSetAllocateInfo dsai    = {0};
        dsai.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool                 = g_descriptor_pool;
        dsai.descriptorSetCount             = 1;
        dsai.pSetLayouts                    = &g_descriptor_set_layout; // for one set
        VkDescriptorSet set                 = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(g_device, &dsai, &set) != VK_SUCCESS) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_textures: vkAllocateDescriptorSets failed");
            return -1;
        }
        /* Static per-material data, written once: no need to keep it mapped. (host visible) */
        VkBuffer material_buf       = VK_NULL_HANDLE;
        VkDeviceMemory material_mem = VK_NULL_HANDLE;
        VkDeviceSize ubo_size       = sizeof(float) * 8;
        if (!vk_create_buffer(ubo_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &material_buf, &material_mem)) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_textures: material UBO creation failed");
            return -1;
        }
        void *mapped = NULL;
        vkMapMemory(g_device, material_mem, 0, ubo_size, 0, &mapped);
        memcpy(mapped, material_ubo, (size_t)ubo_size); // memcpy material_ubo into the mapped UBO memory (GPU)
        vkUnmapMemory(g_device, material_mem);

    VkDescriptorImageInfo diis[2]   = {0}; // diffuse and bump textures, each with their own sampler. The UBO is separate.
    diis[0].imageLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diis[0].imageView               = diffuse_view;
    diis[0].sampler                 = vk_texture_get_sampler();
    diis[1].imageLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diis[1].imageView               = bump_view;
    diis[1].sampler                 = vk_texture_get_sampler();

    VkDescriptorBufferInfo dbi      = {0};  // the UBO with Kd/Ks/Ns, which is written once and never changes for this material set.
    dbi.buffer                      = material_buf;
    dbi.offset                      = 0;
    dbi.range = ubo_size;
    
    /* *** CREATE THE DESCRIPTOR SET *** */
    VkWriteDescriptorSet writes[3] = {0};
    // 0 = diffuse texture, 1 = bump texture, 2 = material UBO
    writes[0].sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet            = set;
    writes[0].dstBinding        = 0;
    writes[0].descriptorType    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount   = 1;
    writes[0].pImageInfo        = &diis[0];
    
    writes[1] = writes[0];
    writes[1].dstBinding        = 1;
    writes[1].pImageInfo        = &diis[1];
    
    writes[2].sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet            = set;
    writes[2].dstBinding        = 2;
    writes[2].descriptorType    = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].descriptorCount   = 1;
    writes[2].pBufferInfo       = &dbi;
    vkUpdateDescriptorSets(g_device, 3, writes, 0, NULL);

    int handle              = g_set_count;
    g_material_bufs[handle] = material_buf;
    g_material_mems[handle] = material_mem;
    g_sets[g_set_count++]   = set;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_descriptors_create_for_textures: created set %d (diffuse=%d, bump=%d)", handle, diffuse_texture_handle, bump_texture_handle);
    return handle;
}

int vk_descriptors_create_for_ui(int texture_handle)
{
    if (g_set_count >= VK_DESCRIPTOR_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_ui: registry full (max %d)", VK_DESCRIPTOR_MAX);
        return -1;
    }
    VkImageView view    = vk_texture_get_view(texture_handle);
    if (view == VK_NULL_HANDLE) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_ui: invalid texture handle");
        return -1;
    }

    VkDescriptorSetAllocateInfo dsai =   {0};
    dsai.sType              =   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool     =   g_descriptor_pool;
    dsai.descriptorSetCount =   1;
    dsai.pSetLayouts        =   &g_descriptor_set_layout;
    VkDescriptorSet set     =   VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(g_device, &dsai, &set) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_descriptors_create_for_ui: vkAllocateDescriptorSets failed");
        return -1;
    }

    VkDescriptorImageInfo dii =   {0};
    dii.imageLayout         =   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dii.imageView           =   view;
    dii.sampler             =   vk_texture_get_ui_sampler();

    VkWriteDescriptorSet writes[2] =   {0};
    writes[0].sType         =   VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet        =   set;
    writes[0].dstBinding    =   0;
    writes[0].descriptorType =  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    writes[0].pImageInfo    =   &dii;
    
    writes[1]               =   writes[0];
    writes[1].dstBinding    =   1;
    vkUpdateDescriptorSets(g_device, 2, writes, 0, NULL); // update both bindings to the same texture, since the UI pipeline's shader statically samples both bindings (diffuse and bump) but only uses the diffuse map.

    int handle              = g_set_count;
    g_sets[g_set_count++]   = set;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_descriptors_create_for_ui: created set %d (texture=%d)", handle, texture_handle);
    return handle;
}

VkDescriptorSet vk_descriptors_get(int handle)
{
    if (handle < 0 || handle >= g_set_count) return VK_NULL_HANDLE;
    return g_sets[handle];
}

/* The descriptor sets themselves are freed implicitly by destroying
 * the pool.
 * However, UBOs are separate VkBuffer/VkDeviceMemory objects. */
void vk_descriptors_destroy_all(void)
{
    for (int i = 0; i < g_set_count; ++i) {
        if (g_material_bufs[i] != VK_NULL_HANDLE) vk_destroy_buffer(g_material_bufs[i], g_material_mems[i]);
        g_material_bufs[i]      = VK_NULL_HANDLE;
        g_material_mems[i]      = VK_NULL_HANDLE;
    }
    if (g_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(g_device, g_descriptor_pool, NULL);
        g_descriptor_pool       = VK_NULL_HANDLE;
    }
    g_set_count = 0;
    if (g_descriptor_set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(g_device, g_descriptor_set_layout, NULL);
        g_descriptor_set_layout     = VK_NULL_HANDLE;
    }
}
