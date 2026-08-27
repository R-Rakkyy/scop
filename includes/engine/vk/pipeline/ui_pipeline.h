#ifndef SCOP_VK_UI_PIPELINE_H
#define SCOP_VK_UI_PIPELINE_H

#include <vulkan/vulkan.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { 
    float pos[2];
    float uv[2];
    float color[4];
} UiVertex;

/* Orthographic textured-quad pipeline for the debug/GUI overlay: alpha
 * blend on, depth test off, dynamic viewport/scissor (so it never needs
 * recreating on resize, unlike the main 3D pipeline). */
int  vk_ui_pipeline_init(void);
void vk_ui_draw(VkCommandBuffer cb, VkBuffer vbuf, uint32_t vertex_count, int descriptor_handle, float screen_w, float screen_h);
void vk_ui_pipeline_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_UI_PIPELINE_H */
