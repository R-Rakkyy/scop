#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include "engine/vk/pipeline/pipeline.h"
#include "engine/vk/pipeline/pipeline_common.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

/* /!\ IMPORTANT NOTE / KNOWN ISSUE /!\
 * mat4 mvp + mat4 model (lighting normals) + vec4 camPos (specular view
 * dir), all computed per-draw-call on the CPU ==> 144 bytes.
 * This is past Vulkan's guaranteed-minimum maxPushConstantsRange of 128
 * (every desktop GPU, reports >=256).
 * 
 * Note: query maxPushConstantsRange and fall back to a descriptor-bound UBO
 * if this ever has to run on unknown/mobile hardware. */
static int create_pipeline_layout_if_needed(void)
{
    if (g_pipeline_layout != VK_NULL_HANDLE) return 1;

    VkPushConstantRange pcr = {0};
    pcr.stageFlags          = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset              = 0;
    pcr.size                = sizeof(float) * 36; // 36 => sizeof(mat4 mvp + mat4 model + vec4 camPos)

    VkPipelineLayoutCreateInfo plci = {0};
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcr;
    if (g_descriptor_set_layout != VK_NULL_HANDLE) {
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &g_descriptor_set_layout;
    }
    if (vkCreatePipelineLayout(g_device, &plci, NULL, &g_pipeline_layout) != VK_SUCCESS) return 0;
    return 1;
}

static VkPipeline create_pipeline(VkShaderModule vert, VkShaderModule frag, const char *debug_name, int blend_enabled)
{
    if (vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE)   return VK_NULL_HANDLE;
    if (!create_pipeline_layout_if_needed())                return VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo vertStage = {0};
    vertStage.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage     = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module    = vert;
    vertStage.pName     = "main";

    VkPipelineShaderStageCreateInfo fragStage = {0};
    fragStage.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module    = frag;
    fragStage.pName     = "main";

    VkPipelineShaderStageCreateInfo stages[2] = { vertStage, fragStage };

    VkVertexInputBindingDescription binding = {0};
    binding.binding     = 0;
    binding.inputRate   = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride      = sizeof(float) * 9; /* interleaved: vec3 pos, vec3 normal, vec2 uv, float face_id = 9 floats */

    VkVertexInputAttributeDescription attrs[4]; // 4 because (pos + normal + uv + face_id)
    /* position */
    attrs[0].binding    = 0;
    attrs[0].location   = 0;
    attrs[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset     = 0;
    /* normal */
    attrs[1].binding    = 0;
    attrs[1].location   = 1;
    attrs[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset     = sizeof(float)*3;
    /* uv */
    attrs[2].binding    = 0;
    attrs[2].location   = 2;
    attrs[2].format     = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset     = sizeof(float)*6;
    /* face_id */
    attrs[3].binding    = 0;
    attrs[3].location   = 3;
    attrs[3].format     = VK_FORMAT_R32_SFLOAT;
    attrs[3].offset     = sizeof(float)*8;

    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.sType                               = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; // yes u read that right
    vis.vertexBindingDescriptionCount       = 1;
    vis.pVertexBindingDescriptions          = &binding;
    vis.vertexAttributeDescriptionCount     = 4; // 4 because (pos + normal + uv + face_id)
    vis.pVertexAttributeDescriptions        = attrs;

    VkPipelineInputAssemblyStateCreateInfo ias = {0};
    ias.sType       = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0};
    viewport.x      = 0.0f;
    viewport.y      = 0.0f;
    viewport.width  = (float)g_swap_extent.width;
    viewport.height = (float)g_swap_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor    = {0}; // scissor is used to clip pixels outside the viewport.
    scissor.offset.x    = 0;
    scissor.offset.y    = 0;
    scissor.extent      = g_swap_extent;

    VkPipelineViewportStateCreateInfo vp = {0};
    vp.sType            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount    = 1;
    vp.pViewports       = &viewport;
    vp.scissorCount     = 1;
    vp.pScissors        = &scissor;

    VkPipelineRasterizationStateCreateInfo rs = {0};
    rs.sType        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode  = VK_POLYGON_MODE_FILL;
    rs.cullMode     = VK_CULL_MODE_NONE;        // NOTE: no backface culling support. Known issue.
    rs.frontFace    = VK_FRONT_FACE_CLOCKWISE;
    rs.lineWidth    = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms = vk_pipeline_common_multisample();

    VkPipelineColorBlendAttachmentState cb = {0};
    cb.colorWriteMask           = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    if (blend_enabled) { // change between opaque and alpha blended pipeline (for palette debug view)
        cb.blendEnable          = VK_TRUE;
        cb.srcColorBlendFactor  = VK_BLEND_FACTOR_CONSTANT_ALPHA;
        cb.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        cb.colorBlendOp         = VK_BLEND_OP_ADD;
        cb.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
        cb.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
        cb.alphaBlendOp         = VK_BLEND_OP_ADD;
    } else {
        cb.blendEnable          = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo cbci = {0};
    cbci.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; // ....
    cbci.attachmentCount    = 1;
    cbci.pAttachments       = &cb;

    VkDynamicState dyn_state                = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
    VkPipelineDynamicStateCreateInfo dyn    = {0};
    dyn.sType                               = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount                   = 1;
    dyn.pDynamicStates                      = &dyn_state;

    VkPipelineDepthStencilStateCreateInfo ds = {0};
    ds.sType                = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; // ...
    ds.depthTestEnable      = VK_TRUE;
    ds.depthWriteEnable     = VK_TRUE;
    ds.depthCompareOp       = VK_COMPARE_OP_LESS_OR_EQUAL; // depth testing >= so the textured pass drawn second always win.

    VkPipelineRenderingCreateInfo rendering_ci = vk_pipeline_common_rendering_info();

    /* *** pipeline creation finalisation *** */
    VkGraphicsPipelineCreateInfo gpci = {0};
    gpci.sType                      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.pNext                      = &rendering_ci;
    gpci.stageCount                 = 2; // 2 = vert + frag
    gpci.pStages                    = stages;
    gpci.pVertexInputState          = &vis;
    gpci.pInputAssemblyState        = &ias;
    gpci.pViewportState             = &vp;
    gpci.pRasterizationState        = &rs;
    gpci.pMultisampleState          = &ms;
    gpci.pColorBlendState           = &cbci;
    gpci.pDepthStencilState         = &ds;
    if (blend_enabled) gpci.pDynamicState = &dyn;
    gpci.layout                     = g_pipeline_layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &gpci, NULL, &pipeline) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_pipeline: failed to create '%s' graphics pipeline", debug_name);
        return VK_NULL_HANDLE;
    }
    ftpp_logf(FTPP_LOG_DEBUG, "vk_pipeline: '%s' pipeline created (handle=%p)", debug_name, (void*)pipeline);
    if (g_dldi.vkSetDebugUtilsObjectNameEXT) { // debug tool
        VkDebugUtilsObjectNameInfoEXT on = {0};
        on.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        on.objectType   = VK_OBJECT_TYPE_PIPELINE;
        on.objectHandle = (uint64_t)pipeline;
        on.pObjectName  = debug_name;
        g_dldi.vkSetDebugUtilsObjectNameEXT(g_device, &on);
    }
    return pipeline;
}

int vk_pipeline_create_from_shaders(VkShaderModule vert, VkShaderModule frag, int graphics_family)
{
    (void)graphics_family;
    g_graphics_pipeline = create_pipeline(vert, frag, "mesh-textured", 1);
    return g_graphics_pipeline != VK_NULL_HANDLE;
}

int vk_pipeline_create_palette(VkShaderModule vert, VkShaderModule frag)
{
    g_graphics_pipeline_palette = create_pipeline(vert, frag, "mesh-palette", 0);
    return g_graphics_pipeline_palette != VK_NULL_HANDLE;
}

void vk_pipeline_destroy(void)
{
    if (g_graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_graphics_pipeline, NULL);
        ftpp_logf(FTPP_LOG_DEBUG, "vk_pipeline_destroy: destroyed textured pipeline");
    }
    g_graphics_pipeline = VK_NULL_HANDLE;

    if (g_graphics_pipeline_palette != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, g_graphics_pipeline_palette, NULL);
        ftpp_logf(FTPP_LOG_DEBUG, "vk_pipeline_destroy: destroyed palette pipeline");
    }
    g_graphics_pipeline_palette = VK_NULL_HANDLE;
    
    if (g_pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(g_device, g_pipeline_layout, NULL);
        ftpp_logf(FTPP_LOG_DEBUG, "vk_pipeline_destroy: destroyed pipeline layout");
    }
    g_pipeline_layout = VK_NULL_HANDLE;
}
