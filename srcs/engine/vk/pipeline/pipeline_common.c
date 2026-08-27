#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include "engine/vk/pipeline/pipeline_common.h"
#include "engine/vk/context.h"

VkPipelineMultisampleStateCreateInfo vk_pipeline_common_multisample(void)
{
    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType                    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples     = g_msaa_samples;
    return ms;
}

VkPipelineRenderingCreateInfo vk_pipeline_common_rendering_info(void)
{
    VkPipelineRenderingCreateInfo rendering_ci = {0};
    rendering_ci.sType                      = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_ci.colorAttachmentCount       = 1;
    rendering_ci.pColorAttachmentFormats    = &g_swap_image_format;
    rendering_ci.depthAttachmentFormat      = g_depth_format;
    return rendering_ci;
}
