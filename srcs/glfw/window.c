#include "glfw/window.h"
#include "iostream/logger_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

static GLFWwindow   *s_window       = NULL;
static atomic_int   s_fb_width      = 0;
static atomic_int   s_fb_height     = 0;

static void callback_error_glfw(int e, const char *desc)
{
    ftpp_logf(FTPP_LOG_ERROR, "GLFW error %d: %s", e, desc);
}

static void framebuffer_size_cb(GLFWwindow *window, int width, int height)
{
    (void)window;
    atomic_store(&s_fb_width, width);
    atomic_store(&s_fb_height, height);
}

int window_init(int width, int height, const char *title, GLFWwindow **out_window)
{
    glfwSetErrorCallback(callback_error_glfw);

    if (!glfwInit())    return 0;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // NO_API means no OpenGL XD
    s_window        =   glfwCreateWindow(width, height, title, NULL, NULL);
    *out_window     =   s_window;

    if (!s_window) return 0;

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(s_window, &fbw, &fbh);
    atomic_store(&s_fb_width, fbw);
    atomic_store(&s_fb_height, fbh);
    glfwSetFramebufferSizeCallback(s_window, framebuffer_size_cb);
    return 1;
}

void window_poll_events(void)
{
    /* glfwPollEvents() but with a timeout. */
    glfwWaitEventsTimeout(1.0 / 60.0);
}

int window_should_close(void)
{
    if (!s_window)  return 1;
    return          glfwWindowShouldClose(s_window);
}

GLFWwindow *window_get(void)
{
    return s_window;
}

void window_get_framebuffer_size(int *out_w, int *out_h)
{
    if (out_w) *out_w = atomic_load(&s_fb_width);
    if (out_h) *out_h = atomic_load(&s_fb_height);
}

void window_destroy(void)
{
    if (!s_window) return;
    
    glfwDestroyWindow(s_window);
    glfwTerminate();
    s_window = NULL;
}
