#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stddef.h>
#include "engine/vk/pipeline/ui_pipeline.h"
#include "engine/vk/pipeline/pipeline_common.h"
#include "engine/vk/context.h"
#include "engine/vk/pipeline/shader.h"
#include "engine/vk/resources/descriptors.h"
#include "iostream/logger_bridge.h"

// Those are reserved for the UI pipeline. There is the same logic inside grid_pipeline.c
// pipeline.c however use shaders that come from the pipeline creation itself, stored in context.h.
static VkShaderModule   g_vert      = VK_NULL_HANDLE;
static VkShaderModule   g_frag      = VK_NULL_HANDLE;
static VkPipelineLayout g_layout    = VK_NULL_HANDLE;
static VkPipeline       g_pipeline  = VK_NULL_HANDLE;

int vk_ui_pipeline_init(void)
{
    if (!vk_create_shader_module_from_file("./shaders/spv/ui/vert.spv", &g_vert)) return 0;
    if (!vk_create_shader_module_from_file("./shaders/spv/ui/frag.spv", &g_frag)) return 0;

    VkPushConstantRange pcr = {0};                  // push constants for screen size
    pcr.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset              = 0;
    pcr.size                = sizeof(float) * 2;    /* vec2 screen size */

    VkPipelineLayoutCreateInfo plci = {0};          // pipeline layout for the UI pipeline
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcr;
    plci.setLayoutCount         = 1;
    plci.pSetLayouts            = &g_descriptor_set_layout; /* same 2-binding layout as materials; binding 1 unused here */
    if (vkCreatePipelineLayout(g_device, &plci, NULL, &g_layout) != VK_SUCCESS) return 0;

    VkPipelineShaderStageCreateInfo stages[2] = {0};// 0 = vertex, 1 = fragment
    stages[0].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage     = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module    = g_vert;
    stages[0].pName     = "main"; // entry point
    stages[1].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module    = g_frag;
    stages[1].pName     = "main"; // entry point

    VkVertexInputBindingDescription binding = {0};  // binding for the vertex buffer (UiVertex)
    binding.binding     = 0;
    binding.inputRate   = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride      = sizeof(UiVertex);

    VkVertexInputAttributeDescription attrs[3];     // 0 = pos, 1 = uv, 2 = color
    attrs[0].binding = 0; attrs[0].location = 0; attrs[0].format = VK_FORMAT_R32G32_SFLOAT;         attrs[0].offset = offsetof(UiVertex, pos);
    attrs[1].binding = 0; attrs[1].location = 1; attrs[1].format = VK_FORMAT_R32G32_SFLOAT;         attrs[1].offset = offsetof(UiVertex, uv);
    attrs[2].binding = 0; attrs[2].location = 2; attrs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;   attrs[2].offset = offsetof(UiVertex, color);

    VkPipelineVertexInputStateCreateInfo vis = {0}; // vertex input state for the UI pipeline
    vis.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount   = 1;
    vis.pVertexBindingDescriptions      = &binding;
    vis.vertexAttributeDescriptionCount = 3;    // 3 attributes: pos, uv, color
    vis.pVertexAttributeDescriptions    = attrs;

    VkPipelineInputAssemblyStateCreateInfo ias = {0};   // input assembly state for the UI pipeline
    ias.sType                           = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology                        = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp = {0};
    vp.sType            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount    = 1;
    vp.scissorCount     = 1;

    VkDynamicState dyn_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; // dynamic viewport/scissor so we don't have to recreate the pipeline on resize
    VkPipelineDynamicStateCreateInfo dyn = {0};
    dyn.sType               = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount   = 2; // 2 because we have viewport and scissor as dynamic states
    dyn.pDynamicStates      = dyn_states;

    VkPipelineRasterizationStateCreateInfo rs = {0};
    rs.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode      = VK_POLYGON_MODE_FILL; // fill polygons (no wireframe)
    rs.cullMode         = VK_CULL_MODE_NONE;    // no culling for UI (i have no culling at all anyway)
    rs.lineWidth        = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms = vk_pipeline_common_multisample(); // epic creation of multisample state for the UI pipeline (uses g_msaa_samples)

    VkPipelineColorBlendAttachmentState cb  = {0};
    cb.colorWriteMask                       = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    cb.blendEnable                          = VK_TRUE;
    cb.srcColorBlendFactor                  = VK_BLEND_FACTOR_SRC_ALPHA;
    cb.dstColorBlendFactor                  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.colorBlendOp                         = VK_BLEND_OP_ADD;
    cb.srcAlphaBlendFactor                  = VK_BLEND_FACTOR_ONE;
    cb.dstAlphaBlendFactor                  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb.alphaBlendOp                         = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo cbci = {0};
    cbci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbci.attachmentCount        = 1;
    cbci.pAttachments           = &cb;

    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType                = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable      = VK_FALSE;
    ds.depthWriteEnable     = VK_FALSE;

    VkPipelineRenderingCreateInfo rendering_ci = vk_pipeline_common_rendering_info();
    /* ************************************************************* */
    VkGraphicsPipelineCreateInfo gpci = {0};        // Creation of the graphics pipeline for the UI overlay (orthographic textured quad)
    gpci.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.pNext                  = &rendering_ci;
    gpci.stageCount             = 2;        // 2 stage because we have vertex and fragment shaders
    gpci.pStages                = stages;
    gpci.pVertexInputState      = &vis;
    gpci.pInputAssemblyState    = &ias;
    gpci.pViewportState         = &vp;
    gpci.pDynamicState          = &dyn;
    gpci.pRasterizationState    = &rs;
    gpci.pMultisampleState      = &ms;
    gpci.pColorBlendState       = &cbci;
    gpci.pDepthStencilState     = &ds;
    gpci.layout                 = g_layout;

    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &gpci, NULL, &g_pipeline) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_ui_pipeline_init: failed to create UI pipeline");
        return 0;
    }
    return 1;
}

void vk_ui_draw(VkCommandBuffer cb, VkBuffer vbuf, uint32_t vertex_count, int descriptor_handle, float screen_w, float screen_h)
{
    if (g_pipeline == VK_NULL_HANDLE || vbuf == VK_NULL_HANDLE || vertex_count == 0) return;
    VkDescriptorSet set = vk_descriptors_get(descriptor_handle);
    if (set == VK_NULL_HANDLE) return;

    VkViewport viewport     = {0};
    viewport.width          = screen_w;
    viewport.height         = screen_h;
    viewport.minDepth       = 0.0f;
    viewport.maxDepth       = 1.0f;
    VkRect2D scissor        = {0};      // scissor rect for the UI overlay (full screen)
    scissor.extent.width    = (uint32_t)screen_w;
    scissor.extent.height   = (uint32_t)screen_h;

    vkCmdSetViewport(cb, 0, 1, &viewport);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_layout, 0, 1, &set, 0, NULL);
    
    float screen_size[2] = { screen_w, screen_h };
    vkCmdPushConstants(cb, g_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screen_size), screen_size); // only constant is the screen size (vec2) for the vertex shader to convert from pixel coordinates to NDC
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cb, 0, 1, &vbuf, offsets);
    vkCmdDraw(cb, vertex_count, 1, 0, 0);
}

void vk_ui_pipeline_destroy(void)
{
    if (g_pipeline != VK_NULL_HANDLE)   { vkDestroyPipeline(g_device, g_pipeline, NULL); g_pipeline = VK_NULL_HANDLE; }
    if (g_layout != VK_NULL_HANDLE)     { vkDestroyPipelineLayout(g_device, g_layout, NULL); g_layout = VK_NULL_HANDLE; }
    if (g_vert != VK_NULL_HANDLE)       { vk_destroy_shader_module(g_vert); g_vert = VK_NULL_HANDLE; }
    if (g_frag != VK_NULL_HANDLE)       { vk_destroy_shader_module(g_frag); g_frag = VK_NULL_HANDLE; }
}
