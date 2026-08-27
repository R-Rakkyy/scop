#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/resources/mesh.h"
#include "engine/vk/resources/buffer.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory mem;
} MeshSlot;

static MeshSlot g_slots[VK_MESH_MAX];
static int g_slot_count = 0;

int vk_mesh_upload_from_data(const s_VkMeshVertexIn *vertices, uint32_t vertex_count)
{
    if (!vertices || vertex_count == 0) return -1;
    if (g_slot_count >= VK_MESH_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_mesh_upload_from_data: mesh registry full (max %d)", VK_MESH_MAX);
        return -1;
    }

    VkDeviceSize bufSize        = sizeof(s_VkMeshVertexIn) * (VkDeviceSize)vertex_count;
    VkBuffer staging            = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem  = VK_NULL_HANDLE;

    // slow and host-visible staging buffer for uploading vertex data to device-local memory
    if (!vk_create_buffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging, &staging_mem)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_mesh_upload_from_data: failed to create staging buffer");
        return -1;
    }

    void *mapped = NULL;
    VkResult _r  = vkMapMemory(g_device, staging_mem, 0, bufSize, 0, &mapped);
    if (_r != VK_SUCCESS || !mapped) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_mesh_upload_from_data: vkMapMemory failed: %d", _r);
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }
    memcpy(mapped, vertices, (size_t)bufSize);  // copy the vertices into the staging buffer
    vkUnmapMemory(g_device, staging_mem);       // unmap the staging buffer memory

    // fast device-local vertex buffer for rendering (GPU-only access)
    VkBuffer        vbuf = VK_NULL_HANDLE;
    VkDeviceMemory  vmem = VK_NULL_HANDLE;
    if (!vk_create_buffer(bufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vbuf, &vmem)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_mesh_upload_from_data: failed to create device-local vertex buffer");
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }
    if (!vk_copy_buffer(staging, vbuf, bufSize)) { // its basically a memcpy staging->vbuf
        ftpp_logf(FTPP_LOG_ERROR, "vk_mesh_upload_from_data: failed to copy staging -> vertex buffer");
        vk_destroy_buffer(staging, staging_mem);
        vk_destroy_buffer(vbuf, vmem);
        return -1;
    }
    vk_destroy_buffer(staging, staging_mem);

    int handle                      = g_slot_count;
    g_slots[g_slot_count].buffer    = vbuf;
    g_slots[g_slot_count].mem       = vmem;
    g_slot_count++;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_mesh_upload_from_data: uploaded mesh (%u vertices, handle=%d)", vertex_count, handle);
    return handle;
}

VkBuffer vk_mesh_get_buffer(int handle)
{
    if (handle < 0 || handle >= g_slot_count) return VK_NULL_HANDLE;
    return g_slots[handle].buffer;
}

void vk_mesh_destroy_all(void)
{
    for (int i = 0; i < g_slot_count; ++i) {
        vk_destroy_buffer(g_slots[i].buffer, g_slots[i].mem);
    }
    g_slot_count = 0;
}
