#ifndef SCOP_VK_INSTANCE_H
#define SCOP_VK_INSTANCE_H

#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

int     vk_instance_create(int enable_validation);
void    vk_instance_destroy(void);
int     vk_surface_create(GLFWwindow *win);
void    vk_surface_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_INSTANCE_H */
