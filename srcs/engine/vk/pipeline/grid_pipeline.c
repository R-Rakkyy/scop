#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/pipeline/grid_pipeline.h"
#include "engine/vk/pipeline/pipeline_common.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/buffer.h"
#include "engine/vk/pipeline/shader.h"
#include "iostream/logger_bridge.h"
#include "config.h"

// Those are reserved for the UI pipeline. There is the same logic inside ui_pipeline.c
// pipeline.c however use shaders that come from the pipeline creation itself, stored in context.h.
static VkBuffer         g_vbuf = VK_NULL_HANDLE;
static VkDeviceMemory   g_vmem = VK_NULL_HANDLE;
static uint32_t         g_vcount = 0;

static VkShaderModule   g_vert = VK_NULL_HANDLE;
static VkShaderModule   g_frag = VK_NULL_HANDLE;
static VkPipelineLayout g_layout = VK_NULL_HANDLE;
static VkPipeline       g_pipeline = VK_NULL_HANDLE;

static int build_vertex_buffer(void)
{
    /* *** create vertex struct *** */
    const uint32_t  vertex_count    = GRID_VERTS_PER_PLANE * GRID_PLANE_COUNT;
    GridVertex      *verts          = malloc(sizeof(GridVertex) * vertex_count);
    if (!verts) return 0;

    const float axis_color[4] =     {GRID_AXIS_COLOR};
    const float minor_color[4] =    {GRID_MINOR_COLOR};
    /* Three axis-aligned reference planes through the origin:
     * XZ (ground, y=0), XY (z=0), YZ (x=0).
     * `u`/`w` are each plane's two in-plane axes
     * `fixed` is the one held at 0. */
    const int u_axis[GRID_PLANE_COUNT]     = {0, 0, 1};
    const int w_axis[GRID_PLANE_COUNT]     = {2, 1, 2};
    const int fixed_axis[GRID_PLANE_COUNT] = {1, 2, 0};

    uint32_t v = 0;
    for (int plane = 0; plane < GRID_PLANE_COUNT; ++plane) {
        int ua = u_axis[plane];     // ua = u_axis[plane]; // u-axis
        int wa = w_axis[plane];     // wa = w_axis[plane]; // w-axis
        int fa = fixed_axis[plane]; // fa = fixed_axis[plane]; // fixed axis
        for (int i = -GRID_HALF_EXTENT; i <= GRID_HALF_EXTENT; i += GRID_SPACING) {
            // basically, do a grid.
            const float *c = (i == 0) ? axis_color : minor_color;
            float p[3];

            p[fa] = 0.0f;
            p[ua] = -GRID_HALF_EXTENT;
            p[wa] = (float)i;
            memcpy(verts[v].pos, p, sizeof(p)); memcpy(verts[v].color, c, sizeof(verts[v].color)); v++;

            p[ua] = GRID_HALF_EXTENT;
            memcpy(verts[v].pos, p, sizeof(p)); memcpy(verts[v].color, c, sizeof(verts[v].color)); v++;

            p[ua] = (float)i;
            p[wa] = -GRID_HALF_EXTENT;
            memcpy(verts[v].pos, p, sizeof(p)); memcpy(verts[v].color, c, sizeof(verts[v].color)); v++;

            p[wa] = GRID_HALF_EXTENT;
            memcpy(verts[v].pos, p, sizeof(p)); memcpy(verts[v].color, c, sizeof(verts[v].color)); v++;
        }
    }

    /* *** map memory, copy verts, unmap, create device-local buffer, copy to it, free staging buffer *** */
    VkDeviceSize bufSize        = sizeof(GridVertex) * (VkDeviceSize)vertex_count;
    VkBuffer staging            = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem  = VK_NULL_HANDLE;
    int _ok = vk_create_buffer(bufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging, &staging_mem);
    if (_ok) {
        void *mapped    = NULL;
        _ok             = (vkMapMemory(g_device, staging_mem, 0, bufSize, 0, &mapped) == VK_SUCCESS && mapped != NULL);
        if (_ok) {
            memcpy(mapped, verts, (size_t)bufSize);
            vkUnmapMemory(g_device, staging_mem);
        }
    }
    if (_ok) {
        _ok = vk_create_buffer(bufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &g_vbuf, &g_vmem);
    }
    if (_ok)                            _ok = vk_copy_buffer(staging, g_vbuf, bufSize);
    if (staging != VK_NULL_HANDLE)      vk_destroy_buffer(staging, staging_mem); // dont need it anymore
    free(verts);

    if (!_ok) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_grid_init: failed to upload grid vertex buffer");
        return 0;
    }
    g_vcount = vertex_count;
    return 1;
}

static int build_grid_pipeline(void)
{
    if (!vk_create_shader_module_from_file("./shaders/spv/grid/vert.spv", &g_vert)) return 0;
    if (!vk_create_shader_module_from_file("./shaders/spv/grid/frag.spv", &g_frag)) return 0;

    VkPushConstantRange pcr = {0};
    pcr.stageFlags  = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset      = 0;
    pcr.size        = sizeof(float) * 16;
    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount     = 1;
    plci.pPushConstantRanges        = &pcr;
    if (vkCreatePipelineLayout(g_device, &plci, NULL, &g_layout) != VK_SUCCESS) return 0; // pip layout

    VkPipelineShaderStageCreateInfo stages[2] = {0}; // 2 stage because vertex + fragment
    stages[0].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage     = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module    = g_vert;
    stages[0].pName     = "main";
    stages[1].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module    = g_frag;
    stages[1].pName     = "main";

    VkVertexInputBindingDescription binding = {0};
    binding.binding     = 0;
    binding.inputRate   = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride      = sizeof(GridVertex);

    VkVertexInputAttributeDescription attrs[2];
    attrs[0].binding = 0; attrs[0].location = 0; attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;      attrs[0].offset = 0;
    attrs[1].binding = 0; attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;   attrs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount   = 1;    // 1 binding
    vis.pVertexBindingDescriptions      = &binding;
    vis.vertexAttributeDescriptionCount = 2;    // 2: pos + color
    vis.pVertexAttributeDescriptions    = attrs;

    VkPipelineInputAssemblyStateCreateInfo ias = {0};
    ias.sType =         VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology =      VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkViewport viewport = {0};
    viewport.width      = (float)g_swap_extent.width;
    viewport.height     = (float)g_swap_extent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    VkRect2D scissor = {0};
    scissor.extent = g_swap_extent;
    VkPipelineViewportStateCreateInfo vp = {0};
    vp.sType            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount    = 1;
    vp.pViewports       = &viewport;
    vp.scissorCount     = 1;
    vp.pScissors        = &scissor;

    VkPipelineRasterizationStateCreateInfo rs = {0};
    rs.sType        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode  = VK_POLYGON_MODE_FILL;
    rs.cullMode     = VK_CULL_MODE_NONE;
    rs.lineWidth    = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms = vk_pipeline_common_multisample();

    /* Alpha blended, not opaque */
    VkPipelineColorBlendAttachmentState cb = {0};
    cb.colorWriteMask   = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    cb.blendEnable      = VK_TRUE;
    cb.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.colorBlendOp     = VK_BLEND_OP_ADD;
    cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.alphaBlendOp     = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo cbci = {0};
    cbci.sType          = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbci.attachmentCount= 1;
    cbci.pAttachments   = &cb;

    /* grid also have depth/stencil */
    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = VK_TRUE;
    ds.depthWriteEnable = VK_FALSE; /* grid never occludes the model */
    ds.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineRenderingCreateInfo rendering_ci = vk_pipeline_common_rendering_info();

    /* *** pipeline creation finalisation *** */
    VkGraphicsPipelineCreateInfo gpci = {0};
    gpci.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.pNext              = &rendering_ci;
    gpci.stageCount         = 2;
    gpci.pStages            = stages;
    gpci.pVertexInputState  = &vis;
    gpci.pInputAssemblyState= &ias;
    gpci.pViewportState     = &vp;
    gpci.pRasterizationState= &rs;
    gpci.pMultisampleState  = &ms;
    gpci.pColorBlendState   = &cbci;
    gpci.pDepthStencilState = &ds;
    gpci.layout             = g_layout;

    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &gpci, NULL, &g_pipeline) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_grid_init: failed to create grid pipeline");
        return 0;
    }
    return 1;
}

int vk_grid_init(void)
{
    if (!build_vertex_buffer()) return 0;
    if (!build_grid_pipeline())      return 0;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_grid_init: grid ready (%u vertices, %d planes)", g_vcount, GRID_PLANE_COUNT);
    return 1;
}

void vk_grid_draw(VkCommandBuffer cb, const float mvp[16])
{
    if (g_pipeline == VK_NULL_HANDLE || g_vbuf == VK_NULL_HANDLE) return;
    
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);
    vkCmdPushConstants(cb, g_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16, mvp);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cb, 0, 1, &g_vbuf, offsets);
    vkCmdDraw(cb, g_vcount, 1, 0, 0);
}

void vk_grid_destroy(void)
{
    if (g_pipeline != VK_NULL_HANDLE)   { vkDestroyPipeline(g_device, g_pipeline, NULL); g_pipeline = VK_NULL_HANDLE;       }
    if (g_layout != VK_NULL_HANDLE)     { vkDestroyPipelineLayout(g_device, g_layout, NULL); g_layout = VK_NULL_HANDLE;     }
    if (g_vert != VK_NULL_HANDLE)       { vk_destroy_shader_module(g_vert); g_vert = VK_NULL_HANDLE;                        }
    if (g_frag != VK_NULL_HANDLE)       { vk_destroy_shader_module(g_frag); g_frag = VK_NULL_HANDLE;                        }
    if (g_vbuf != VK_NULL_HANDLE)       { vk_destroy_buffer(g_vbuf, g_vmem); g_vbuf = VK_NULL_HANDLE; g_vmem = VK_NULL_HANDLE;}
    g_vcount = 0;
}
