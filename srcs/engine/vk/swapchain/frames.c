#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "config.h"
#include "engine/vk/swapchain/frames.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

static void free_fence_array(VkFence **arr_ptr, uint32_t count)
{
    if (!arr_ptr || !*arr_ptr) return;

    for (uint32_t k = 0; k < count; ++k) {
        if ((*arr_ptr)[k] != VK_NULL_HANDLE) vkDestroyFence(g_device, (*arr_ptr)[k], NULL);
    }

    free(*arr_ptr);
    *arr_ptr = NULL;
}

static void free_semaphore_array(VkSemaphore **arr_ptr, uint32_t count)
{
    if (!arr_ptr || !*arr_ptr) return;

    for (uint32_t k = 0; k < count; ++k) {
        if ((*arr_ptr)[k] != VK_NULL_HANDLE) vkDestroySemaphore(g_device, (*arr_ptr)[k], NULL);
    }

    free(*arr_ptr);
    *arr_ptr = NULL;
}

int vk_frames_create_sync_objects(void)
{
    uint32_t frames = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;

    g_img_avail     = calloc(frames, sizeof(VkSemaphore));
    g_render_fin    = calloc(g_image_count, sizeof(VkSemaphore));
    g_fences        = calloc(frames, sizeof(VkFence));
    if (!g_img_avail || !g_render_fin || !g_fences) {
        free_semaphore_array(&g_img_avail, frames);
        free_semaphore_array(&g_render_fin, g_image_count);
        free_fence_array(&g_fences, frames);
        ftpp_logf(FTPP_LOG_ERROR, "vk_frames_create_sync_objects: allocation failure for per-frame sync arrays");
        return 0;
    }

    VkSemaphoreCreateInfo sci   = {0};
    sci.sType                   =  VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fci   = {0};
    fci.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < g_image_count; ++i) {
        if (vkCreateSemaphore(g_device, &sci, NULL, &g_render_fin[i]) != VK_SUCCESS) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_frames_create_sync_objects: vkCreateSemaphore(render_fin) failed for image %u", i);
            free_semaphore_array(&g_render_fin, i);
            free_semaphore_array(&g_img_avail, frames);
            free_fence_array(&g_fences, frames);
            return 0;
        }
    }

    for (uint32_t i = 0; i < frames; ++i) {
        if (vkCreateSemaphore(g_device, &sci, NULL, &g_img_avail[i]) != VK_SUCCESS) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_frames_create_sync_objects: vkCreateSemaphore(img_avail) failed for frame %u", i);
            free_fence_array(&g_fences, i);
            free_semaphore_array(&g_img_avail, i);
            free_semaphore_array(&g_render_fin, g_image_count);
            return 0;
        }
        if (vkCreateFence(g_device, &fci, NULL, &g_fences[i]) != VK_SUCCESS) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_frames_create_sync_objects: vkCreateFence failed for frame %u", i);
            free_fence_array(&g_fences, i + 1);
            free_semaphore_array(&g_img_avail, i + 1);
            free_semaphore_array(&g_render_fin, g_image_count);
            return 0;
        }
        ftpp_logf(FTPP_LOG_DEBUG, "vk_frames_create_sync_objects: created semaphores/fence for frame %u", i);
    }

    // Create array to track which fence is associated with which swapchain image (g_image_in_flight)
    g_image_in_flight = malloc(sizeof(VkFence) * g_image_count);
    if (!g_image_in_flight) {
        free_fence_array(&g_fences, frames);
        free_semaphore_array(&g_img_avail, frames);
        free_semaphore_array(&g_render_fin, g_image_count);
        ftpp_logf(FTPP_LOG_ERROR, "vk_frames_create_sync_objects: allocation failure for image-in-flight array");
        return 0;
    }
    for (uint32_t i = 0; i < g_image_count; ++i) g_image_in_flight[i] = VK_NULL_HANDLE;
    g_frame_index = 0;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_frames_create_sync_objects: initialized %u frames-in-flight and %u image-in-flight slots", frames, g_image_count);
    return 1;
}

void vk_frames_destroy_sync_objects(void)
{
    uint32_t frames_count_inflight     = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;

    if (g_fences) {
        for (uint32_t i = 0; i < frames_count_inflight; ++i) {
            if (g_fences[i] != VK_NULL_HANDLE) vkDestroyFence(g_device, g_fences[i], NULL);
        }
        free(g_fences);
        g_fences = NULL;
    }

    if (g_render_fin) {
        for (uint32_t i = 0; i < g_image_count; ++i) {
            if (g_render_fin[i] != VK_NULL_HANDLE) vkDestroySemaphore(g_device, g_render_fin[i], NULL);
        }
        free(g_render_fin);
        g_render_fin = NULL;
    }

    if (g_img_avail) {
        for (uint32_t i = 0; i < frames_count_inflight; ++i) {
            if (g_img_avail[i] != VK_NULL_HANDLE) vkDestroySemaphore(g_device, g_img_avail[i], NULL);
        }
        free(g_img_avail);
        g_img_avail = NULL;
    }

    if (g_image_in_flight) { free(g_image_in_flight); g_image_in_flight = NULL; }
}

void vk_frames_teardown_all(void)
{
    vk_frames_destroy_sync_objects();
}
