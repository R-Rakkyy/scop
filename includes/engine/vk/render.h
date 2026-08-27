#ifndef SCOP_VK_RENDER_H
#define SCOP_VK_RENDER_H

#include <vulkan/vulkan.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One draw: a contiguous vertex range of one uploaded mesh (see vk/mesh.h),
 * rendered with one material's descriptor set (see vk/descriptors.h) and
 * its own model-view-projection matrix. */
typedef struct {
    int      mesh_handle;

    uint32_t first_vertex;
    uint32_t vertex_count;
    int      descriptor_handle;

    float    mvp[16];
    float    model[16];     /** model-only matrix, for transforming vertex normals into world space. */
    float    cam_pos[4];    /** world-space camera position (.xyz; .w unused/pad), for specular. */
} s_VkDrawCall;

typedef struct {
    VkBuffer buffer;

    uint32_t vertex_count;

    int      descriptor_handle;
} s_VkUiDrawCall;

int     vk_render_init(int graphics_family);

/* Note: grid_mvp/ui may be NULL to skip drawing the floor grid / overlay (e.g. before ready).
 * Note: texture_mix [0,1] crossfades from the palette debug view (0, default) */
void    vk_render_draw_frame(const float *grid_mvp, const s_VkDrawCall *calls, uint32_t call_count, const s_VkUiDrawCall *ui, float texture_mix);
void    vk_render_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_RENDER_H */
