#ifndef SCOP_VK_PHYSICAL_DEVICE_H
#define SCOP_VK_PHYSICAL_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    uint32_t    index;
    int         graphics_family;
    int         present_family;
} t_gpu_candidate;

int vk_pick_physical_and_queues(int *graphics_family, int *present_family);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_PHYSICAL_DEVICE_H */
