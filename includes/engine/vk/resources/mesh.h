#ifndef SCOP_VK_MESH_H
#define SCOP_VK_MESH_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct s_VkMeshVertexIn
 * @brief Represents a single vertex input for the Vulkan mesh.
 *
 * It is a mirror of the @ref ObjVertex structure.
 */
typedef struct {
    float pos[3];           /** The position of the vertex in 3D space. */
    float normal[3];        /** The vertex normal (object space), used for lighting. */
    float uv[2];            /** The texture coordinates of the vertex. */

    float face_id;          /** Source face index, used by the palette debug view to color each face. */
} s_VkMeshVertexIn;

/** 
 * @brief Uploads a mesh's vertices to a device-local buffer.
 * @return a handle >= 0 (used by vk_render_draw_frame's s_VkDrawCall::mesh_handle), or -1
 * on failure.
 * @note Meshes live until vk_mesh_destroy_all(). */
int      vk_mesh_upload_from_data(const s_VkMeshVertexIn *vertices, uint32_t vertex_count);
VkBuffer vk_mesh_get_buffer(int handle);
void     vk_mesh_destroy_all(void);

#ifdef __cplusplus
}
#endif

#endif
