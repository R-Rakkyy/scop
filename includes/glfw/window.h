#ifndef SCOP_GLFW_WINDOW_H
#define SCOP_GLFW_WINDOW_H

#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

int         window_init(int width, int height, const char *title, GLFWwindow **out_window);
void        window_poll_events(void);
int         window_should_close(void);
GLFWwindow  *window_get(void);
void        window_destroy(void);

/* Thread-safe framebuffer size, refreshed only by GLFW's own resize
 * callback (fired on the main thread during window_poll_events).
 *
 * Use this instead of glfwGetFramebufferSize() from any thread other than main.
 * 
 * GLFW functions are mainly main-thread-only. */
void        window_get_framebuffer_size(int *out_w, int *out_h);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_GLFW_WINDOW_H */
