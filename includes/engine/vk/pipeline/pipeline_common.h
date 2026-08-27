#ifndef SCOP_VK_PIPELINE_COMMON_H
#define SCOP_VK_PIPELINE_COMMON_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared by every graphics pipeline (mesh, palette, grid, UI): samples at
 * g_msaa_samples, exactly what every pipeline in this project must match
 * (see vk/context.h). */
VkPipelineMultisampleStateCreateInfo vk_pipeline_common_multisample(void);

/* Shared by every graphics pipeline: dynamic-rendering color/depth
 * attachment formats, matching the swapchain image format and the shared
 * depth image format (g_swap_image_format/g_depth_format in vk/context.h). */
VkPipelineRenderingCreateInfo vk_pipeline_common_rendering_info(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_PIPELINE_COMMON_H */
