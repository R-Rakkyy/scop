#ifndef SCOP_VK_DESCRIPTORS_H
#define SCOP_VK_DESCRIPTORS_H

#include <vulkan/vulkan.h>
#include "config.h" /* VK_DESCRIPTOR_MAX */

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the shared descriptor set layout (binding 0 = diffuse map,
 * binding 1 = bump map, binding 2 = material scalars UBO) and a pool sized
 * for VK_DESCRIPTOR_MAX sets.
 * Note: Call once, before any vk_descriptors_create_for_textures(). */
int             vk_descriptors_init_layout(void);

/* One descriptor set bound to a pair of already-created textures,
 * plus, a small per-material UBO written once from
 * material_ubo: { Kd.xyz, pad; Ks.xyz, Ns }
 * Returns a handle >= 0, or -1 on failure. */
int             vk_descriptors_create_for_textures(int diffuse_texture_handle, int bump_texture_handle, const float material_ubo[8]);

int             vk_descriptors_create_for_ui(int texture_handle);
VkDescriptorSet vk_descriptors_get(int handle);
void            vk_descriptors_destroy_all(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_DESCRIPTORS_H */
