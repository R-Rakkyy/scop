#ifndef SCOP_VK_LOGICAL_DEVICE_H
#define SCOP_VK_LOGICAL_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

int vk_create_device_and_queues(int graphics_family, int present_family);
void vk_destroy_device(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_LOGICAL_DEVICE_H */
