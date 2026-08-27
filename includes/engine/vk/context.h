#ifndef SCOP_VK_CONTEXT_H
#define SCOP_VK_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

/*

 * Vulkan context, side note: (known issue)
 *
 * I am aware that this is not an elegant way to handle those kind of things.
 * At first i was very naive and thought it would only be a matter
 * of small amount of variables, and that i would change it in the future.
 * We are, in the future, and i won't change it.
 * 
 * Explanation of every variable are in the .c file.

*/

extern      VkInstance          g_instance;
extern      VkSurfaceKHR        g_surface;
extern      VkPhysicalDevice    g_physical;
extern      VkDevice            g_device;
extern      VkQueue             g_graphics;
extern      VkQueue             g_present;
extern      VkSwapchainKHR      g_swap;
extern      VkImage             *g_images;
extern      VkImageView         *g_image_views;
extern      uint32_t            g_image_count;
extern      VkFormat            g_swap_image_format;
extern      VkColorSpaceKHR     g_swap_color_space;
extern      VkExtent2D          g_swap_extent;
extern      VkPresentModeKHR    g_swap_present_mode;
extern      VkCommandPool       g_cmdpool;
extern      VkCommandBuffer     *g_cmdbuf;
extern      VkSemaphore         *g_img_avail;
extern      VkSemaphore         *g_render_fin;
extern      VkFence             *g_fences;
extern      VkFence             *g_image_in_flight;
extern      uint32_t            g_frames_in_flight;
extern      uint32_t            g_frame_index;
extern      int                 g_graphics_family;
extern      int                 g_present_family;
extern      VkPipeline          g_graphics_pipeline;
extern      VkPipeline          g_graphics_pipeline_palette;
extern      VkPipelineLayout    g_pipeline_layout;
extern      VkShaderModule      g_vert_shader;
extern      VkShaderModule      g_frag_shader;
extern      VkShaderModule      g_frag_shader_palette;

extern      VkImage             g_depth_image;
extern      VkDeviceMemory      g_depth_image_mem;
extern      VkImageView         g_depth_image_view;
extern      VkFormat            g_depth_format;

extern      VkDescriptorSetLayout g_descriptor_set_layout;
extern      VkDescriptorPool      g_descriptor_pool;

extern      int                   g_memory_budget_supported;
extern      float                 g_max_sampler_anisotropy;
extern      VkSampleCountFlagBits g_msaa_samples;
extern      VkImage               g_msaa_color_image;
extern      VkDeviceMemory        g_msaa_color_mem;
extern      VkImageView           g_msaa_color_view;
extern      int                   g_vsync_enabled;
extern      int                   g_lighting_enabled;

typedef struct {
	PFN_vkCreateDebugUtilsMessengerEXT    vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT   vkDestroyDebugUtilsMessengerEXT;
	PFN_vkSetDebugUtilsObjectNameEXT      vkSetDebugUtilsObjectNameEXT;
	PFN_vkCmdBeginDebugUtilsLabelEXT      vkCmdBeginDebugUtilsLabelEXT;
	PFN_vkCmdEndDebugUtilsLabelEXT        vkCmdEndDebugUtilsLabelEXT;
	PFN_vkCreateSwapchainKHR              vkCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR             vkDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR           vkGetSwapchainImagesKHR;
	PFN_vkAcquireNextImageKHR             vkAcquireNextImageKHR;
	PFN_vkQueuePresentKHR                 vkQueuePresentKHR;
} vk_dispatch_t;

extern vk_dispatch_t 			g_dldi;
extern VkDebugUtilsMessengerEXT g_debug_messenger;

#endif /* SCOP_VK_CONTEXT_H */
