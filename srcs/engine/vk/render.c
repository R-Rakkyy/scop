#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "engine/vk/render.h"
#include "engine/vk/context.h"
#include "engine/vk/pipeline/pipeline.h"
#include "engine/vk/pipeline/shader.h"
#include "engine/vk/swapchain/depth.h"
#include "iostream/logger_bridge.h"
#include "engine/vk/swapchain/swapchain_recreate.h"
#include "engine/vk/swapchain/frames.h"
#include "engine/vk/resources/mesh.h"
#include "engine/vk/resources/descriptors.h"
#include "engine/vk/pipeline/grid_pipeline.h"
#include "engine/vk/pipeline/ui_pipeline.h"
#include "engine/vk/swapchain/msaa.h"
#include "glfw/window.h"

// note: resolved a bug using this because comparing with the get extent function was unprecise and caused infinite loop recreation
static int s_last_fbw = 0;
static int s_last_fbh = 0;

/* Helpers to simplify vk_render_draw_frame and improve readability */
static int wait_for_frame_fence(uint32_t frame)
{
    VkResult wf = vkWaitForFences(g_device, 1, &g_fences[frame], VK_TRUE, 1000000000ULL);
    if (wf == VK_TIMEOUT) return 0; /* caller should return and try later */
    return 1;
}

/* Returns: 1 = acquired, 0 = timeout/other (try later), -1 = out-of-date/suboptimal (shall recreate swapchain) */
static int acquire_next_image(uint32_t frame, uint32_t *out_img_idx)
{
    VkResult acq    = VK_SUCCESS;
    if (g_dldi.vkAcquireNextImageKHR) 
        acq         = g_dldi.vkAcquireNextImageKHR(g_device, g_swap, 100000000ULL, g_img_avail[frame], VK_NULL_HANDLE, out_img_idx);
    else
        acq         = vkAcquireNextImageKHR(g_device, g_swap, 100000000ULL, g_img_avail[frame], VK_NULL_HANDLE, out_img_idx);
    
    if (acq == VK_TIMEOUT) return 0; // timeout
    if (acq == VK_ERROR_OUT_OF_DATE_KHR || acq == VK_SUBOPTIMAL_KHR) return -1; // dead swapchain, recreate plz
    if (acq != VK_SUCCESS) return 0; // other error, try later
    return 1;
}

static int ensure_image_not_in_flight(uint32_t img_idx)
{
    if (g_image_in_flight && g_image_in_flight[img_idx] != VK_NULL_HANDLE) {
        VkResult wf2 = vkWaitForFences(g_device, 1, &g_image_in_flight[img_idx], VK_TRUE, 100000000ULL);
        
        if (wf2 == VK_TIMEOUT) {
            ftpp_logf(FTPP_LOG_WARNING, "vk_render_draw_frame: image %u still in flight, skipping frame", img_idx);
            return 0; /* image still in use; caller should skip frame */
        }
    }
    return 1;
}

static int begin_command_buffer_for_frame(VkCommandBuffer *out_cb, uint32_t frame)
{
    VkCommandBufferBeginInfo cbbi   = {0};
    cbbi.sType                      = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCommandBuffer command_buffer  = g_cmdbuf[frame];

    VkResult _r = vkBeginCommandBuffer(command_buffer, &cbbi);
    if (_r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_render_draw_frame: vkBeginCommandBuffer failed (VkResult=%d)", (int)_r);
        return 0;
    }

    *out_cb = command_buffer;
    return 1;
}

// Transitions an image from one layout to another in the worst way possible.
static void transition_image(
    VkCommandBuffer         cb,             VkImage image,              VkImageAspectFlags aspect,
    VkImageLayout           old_layout,     VkImageLayout new_layout,
    VkPipelineStageFlags    src_stage,      VkAccessFlags src_access,   
    VkPipelineStageFlags    dst_stage,      VkAccessFlags dst_access)
{
    VkImageMemoryBarrier barrier        = {0};
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                   = old_layout;
    barrier.newLayout                   = new_layout;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                       = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask               = src_access;
    barrier.dstAccessMask               = dst_access;

    vkCmdPipelineBarrier(cb, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static void draw_mesh_calls(VkCommandBuffer command_buffer, VkPipeline pipeline, const s_VkDrawCall *calls, uint32_t call_count)
{
    if (pipeline == VK_NULL_HANDLE) return;
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    for (uint32_t i = 0; i < call_count; ++i) {
        const s_VkDrawCall  *call   = &calls[i];
        VkBuffer            vbuf    = vk_mesh_get_buffer(call->mesh_handle);
        if (vbuf == VK_NULL_HANDLE || call->vertex_count == 0) continue;

        VkDescriptorSet     set     = vk_descriptors_get(call->descriptor_handle);
        if (set != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline_layout, 0, 1, &set, 0, NULL);
        }

        /* mvp, model and cam_pos are adjacent same-typed array members of
         * s_VkDrawCall (no padding between them), so one push covers all three. */
        vkCmdPushConstants(command_buffer, g_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(call->mvp) + sizeof(call->model) + sizeof(call->cam_pos), call->mvp);

        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vbuf, offsets);
        vkCmdDraw(command_buffer, call->vertex_count, 1, call->first_vertex, 0);
    }
}

static int record_draw_commands(VkCommandBuffer command_buffer, uint32_t img_idx, const float *grid_mvp,
                                 const s_VkDrawCall *calls, uint32_t call_count, const s_VkUiDrawCall *ui, float texture_mix)
{
    VkImage color_image = g_images ? g_images[img_idx] : VK_NULL_HANDLE;
    int use_msaa = (g_msaa_samples > VK_SAMPLE_COUNT_1_BIT) && g_msaa_color_view != VK_NULL_HANDLE;
    // we have 3 potential images to transition: the swapchain image, the MSAA color image (if used), and the depth image. All must be transitioned to their respective attachment layouts before rendering.
    transition_image(   command_buffer,                                 color_image,                                
                        VK_IMAGE_ASPECT_COLOR_BIT,                      VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,              0,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    if (use_msaa) {
        transition_image(command_buffer,                                g_msaa_color_image, 
                        VK_IMAGE_ASPECT_COLOR_BIT,                      VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,              0,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    }
    transition_image(   command_buffer,                                 g_depth_image,
                        VK_IMAGE_ASPECT_DEPTH_BIT,                      VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,              0,
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

    if (g_dldi.vkCmdBeginDebugUtilsLabelEXT) { // debug utils
        VkDebugUtilsLabelEXT lbl    = {0};
        lbl.sType                   = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        lbl.pLabelName              = "frame-render";
        lbl.color[0] = 0.0f; lbl.color[1] = 0.6f; lbl.color[2] = 0.6f; lbl.color[3] = 1.0f;
        g_dldi.vkCmdBeginDebugUtilsLabelEXT(command_buffer, &lbl);
    }

    /* *** Begin dynamic rendering (VK_KHR_dynamic_rendering) *** */
    VkRenderingAttachmentInfo color_att     = {0};
    color_att.sType                         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_att.imageView                     = use_msaa ? g_msaa_color_view : g_image_views[img_idx];
    color_att.imageLayout                   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_att.loadOp                        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    /* Without MSAA the swapchain image itself is the attachment and must be stored. */
    color_att.storeOp                       = use_msaa ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    if (use_msaa) {
        color_att.resolveMode           = VK_RESOLVE_MODE_AVERAGE_BIT;
        color_att.resolveImageView      = g_image_views[img_idx];
        color_att.resolveImageLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    /* #2a389b */
    color_att.clearValue.color = (VkClearColorValue){ {SCOP_BACKGROUND_COLOR} };

    VkRenderingAttachmentInfo   depth_att   = {0};
    depth_att.sType                         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_att.imageView                     = g_depth_image_view;
    depth_att.imageLayout                   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_att.loadOp                        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att.storeOp                       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.clearValue.depthStencil       = (VkClearDepthStencilValue){ 1.0f, 0 };

    VkRenderingInfo ri                      = {0};
    ri.sType                                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea.offset.x                  = 0;
    ri.renderArea.offset.y                  = 0;
    ri.renderArea.extent                    = g_swap_extent;
    ri.layerCount                           = 1;
    ri.colorAttachmentCount                 = 1;
    ri.pColorAttachments                    = &color_att;
    ri.pDepthAttachment                     = &depth_att;

    vkCmdBeginRendering(command_buffer, &ri);
    if (grid_mvp) vk_grid_draw(command_buffer, grid_mvp);

    if (texture_mix < 1.0f && g_graphics_pipeline_palette != VK_NULL_HANDLE) {      // draw palette pass only if texture_mix < 1.0f
        draw_mesh_calls(command_buffer, g_graphics_pipeline_palette, calls, call_count);
    }
    if (texture_mix > 0.0f && g_graphics_pipeline != VK_NULL_HANDLE) {              // draw textured pass only if texture_mix > 0.0f
        float alpha                 = texture_mix > 1.0f ? 1.0f : texture_mix;
        float blend_constants[4]    = { 1.0f, 1.0f, 1.0f, alpha };
        vkCmdSetBlendConstants(command_buffer, blend_constants);
        draw_mesh_calls(command_buffer, g_graphics_pipeline, calls, call_count);
    }

    if (ui) {
        vk_ui_draw(command_buffer, ui->buffer, ui->vertex_count, ui->descriptor_handle, (float)g_swap_extent.width, (float)g_swap_extent.height);
    }
    /* *** end dynamic rendering *** */
    vkCmdEndRendering(command_buffer);

    // transition the swapchain image to present layout for presentation
    transition_image(command_buffer,                                color_image,
                    VK_IMAGE_ASPECT_COLOR_BIT,                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           0);

    if (g_dldi.vkCmdEndDebugUtilsLabelEXT) g_dldi.vkCmdEndDebugUtilsLabelEXT(command_buffer);
    VkResult _r = vkEndCommandBuffer(command_buffer);
    if (_r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_render_draw_frame: vkEndCommandBuffer failed (VkResult=%d)", (int)_r);
        return 0;
    }
    return 1;
}

static int submit_and_present(VkCommandBuffer command_buffer, uint32_t frame, uint32_t img_idx)
{
    /* *** submit *** */
    VkPipelineStageFlags waitStages[]   = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submit                 = {0};
    submit.sType                        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount           = 1;
    submit.pWaitSemaphores              = &g_img_avail[frame];
    submit.pWaitDstStageMask            = waitStages;
    submit.commandBufferCount           = 1;
    submit.pCommandBuffers              = &command_buffer;
    submit.signalSemaphoreCount         = 1;
    submit.pSignalSemaphores            = &g_render_fin[img_idx];
    
    VkResult _submit_queue_res = vkQueueSubmit(g_graphics, 1, &submit, g_fences[frame]);
    if (_submit_queue_res != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_render_draw_frame: vkQueueSubmit failed (VkResult=%d)", (int)_submit_queue_res);
        return 0;
    }
    /* *** present *** */
    if (g_image_in_flight) g_image_in_flight[img_idx] = g_fences[frame];

    VkPresentInfoKHR pres   = {0};
    pres.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pres.waitSemaphoreCount = 1;
    pres.pWaitSemaphores    = &g_render_fin[img_idx];
    pres.swapchainCount     = 1;
    pres.pSwapchains        = &g_swap;
    pres.pImageIndices      = &img_idx;
    VkResult pres_res       = VK_SUCCESS;

    if (g_dldi.vkQueuePresentKHR)   pres_res = g_dldi.vkQueuePresentKHR(g_present, &pres);
    else                            pres_res = vkQueuePresentKHR(g_present, &pres);
    
    if (pres_res == VK_ERROR_OUT_OF_DATE_KHR || pres_res == VK_SUBOPTIMAL_KHR) {
        ftpp_logf(FTPP_LOG_WARNING, "vk_render_draw_frame: swapchain out-of-date/suboptimal (VkResult=%d), recreating", (int)pres_res);
        if (!vk_recreate_swapchain())
            ftpp_logf(FTPP_LOG_ERROR, "vk_render_draw_frame: recreate after out-of-date present failed, will retry via Renderer::try_recover");
    }
    return 1;
}


int vk_render_init(int graphics_family)
{
    (void)graphics_family;
    if (!vk_depth_create(g_swap_extent)) return 0;
    if (!vk_msaa_create(g_swap_extent)) return 0;
    if (!vk_frames_create_sync_objects()) return 0;
    return 1;
}

void vk_render_draw_frame(const float *grid_mvp, const s_VkDrawCall *calls, uint32_t call_count, const s_VkUiDrawCall *ui, float texture_mix)
{
    /* VK_ERROR_OUT_OF_DATE_KHR/VK_SUBOPTIMAL_KHR aren't guaranteed
     * to fire promptly on every platform (i.e Wayland).
     * Vulkan doc says that polling the live framebuffer size every frame is the
     * portable way to catch a resize, matching the standard Vulkan
     * swapchain-recreation pattern. */
    int fbw = 0, fbh = 0;
    window_get_framebuffer_size(&fbw, &fbh); // get the current framebuffer size from GLFW, thread-safe and platform-agnostic
    if (fbw > 0 && fbh > 0) {
        if (s_last_fbw == 0 && s_last_fbh == 0) { // first frame, initialize last known framebuffer size
            s_last_fbw = fbw;
            s_last_fbh = fbh;
        } else if (fbw != s_last_fbw || fbh != s_last_fbh) {
            ftpp_logf(FTPP_LOG_INFO, "vk_render_draw_frame: framebuffer resized %dx%d -> %dx%d, recreating swapchain", s_last_fbw, s_last_fbh, fbw, fbh);
            s_last_fbw = fbw;
            s_last_fbh = fbh;
            vk_recreate_swapchain();
            return;
        }
    }

    /* Logic behind the draw */
    uint32_t img_idx        = 0;
    uint32_t frames_count   = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;
    uint32_t frame          = g_frame_index % frames_count;

    if (!wait_for_frame_fence(frame)) return;

    int acq = acquire_next_image(frame, &img_idx);
    if (acq == 0) return; /* try later */
    if (acq < 0) {
        ftpp_logf(FTPP_LOG_INFO, "vk_render_draw_frame: swapchain out-of-date on acquire, recreating");
        vk_recreate_swapchain();
        return;
    } /* dead swapchain */

    if (!ensure_image_not_in_flight(img_idx)) return;

    vkResetFences(g_device, 1, &g_fences[frame]);

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    if (!begin_command_buffer_for_frame(&command_buffer, frame)) return;

    if (!record_draw_commands(command_buffer, img_idx, grid_mvp, calls, call_count, ui, texture_mix)) return;

    if (!submit_and_present(command_buffer, frame, img_idx)) return;

    g_frame_index++;
}

void vk_render_cleanup(void)
{
    /* wait for things to be idle before destroying resources */
    if (g_graphics != VK_NULL_HANDLE)   vkQueueWaitIdle(g_graphics);
    if (g_present != VK_NULL_HANDLE)    vkQueueWaitIdle(g_present);
    if (g_device != VK_NULL_HANDLE)     vkDeviceWaitIdle(g_device);
    /* destroy pipeline and shader modules */
    vk_pipeline_destroy();
    if (g_vert_shader != VK_NULL_HANDLE)            vk_destroy_shader_module(g_vert_shader);
    if (g_frag_shader != VK_NULL_HANDLE)            vk_destroy_shader_module(g_frag_shader);
    if (g_frag_shader_palette != VK_NULL_HANDLE)    vk_destroy_shader_module(g_frag_shader_palette);

    /* destroy frame-related resources via frames module */
    uint32_t frames_count = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;
    vk_frames_destroy_sync_objects();
    if (g_cmdbuf) {
        vkFreeCommandBuffers(g_device, g_cmdpool, frames_count, g_cmdbuf);
        free(g_cmdbuf);
        g_cmdbuf = NULL;
    }
    /* destroy descriptor sets and pool */
    if (g_cmdpool != VK_NULL_HANDLE) vkDestroyCommandPool(g_device, g_cmdpool, NULL);
    vk_depth_destroy();
    vk_msaa_destroy();
}
