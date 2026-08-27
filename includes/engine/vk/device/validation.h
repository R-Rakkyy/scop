#ifndef SCOP_VK_VALIDATION_H
#define SCOP_VK_VALIDATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char** vk_validation_get_layers(uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_VALIDATION_H */
