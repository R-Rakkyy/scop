#ifndef SCOP_VK_GRID_H
#define SCOP_VK_GRID_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float pos[3];
    float color[4];
} GridVertex;

/* Builds the floor-grid vertex buffer and its own line-topology pipeline.
 * Must be called after vk_render_init() (needs g_depth_format/g_swap_image_format)
 * and before the first vk_grid_draw(). */
int  vk_grid_init(void);

void vk_grid_draw(VkCommandBuffer cb, const float mvp[16]);
void vk_grid_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_GRID_H */
