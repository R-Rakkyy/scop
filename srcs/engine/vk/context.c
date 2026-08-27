#include <GLFW/glfw3.h>
#include "engine/vk/context.h"
#include "config.h"

VkInstance          g_instance          = VK_NULL_HANDLE; // Vulkan instance
VkSurfaceKHR        g_surface           = VK_NULL_HANDLE; // Window surface for presentation
VkPhysicalDevice    g_physical          = VK_NULL_HANDLE; // Physical GPU device
VkDevice            g_device            = VK_NULL_HANDLE; // Logical device for interfacing with the GPU
VkQueue             g_graphics          = VK_NULL_HANDLE; // Graphics queue for rendering commands
VkQueue             g_present           = VK_NULL_HANDLE; // Presentation queue for presenting images to the surface
VkSwapchainKHR      g_swap              = VK_NULL_HANDLE; // Swapchain for managing the images to be presented
VkImage             *g_images           = NULL;           // Raw swapchain images (for layout transitions, dynamic rendering)
VkImageView         *g_image_views      = NULL;           // Image views for each swapchain image
uint32_t            g_image_count       = 0;              // Number of images in the swapchain
VkFormat            g_swap_image_format = VK_FORMAT_UNDEFINED; 
                                                          // Format of the swapchain images (set during swapchain creation)
VkColorSpaceKHR     g_swap_color_space  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; 
                                                          // Color space of the swapchain images (set during swapchain creation)
VkExtent2D          g_swap_extent       = {0,0};
VkPresentModeKHR    g_swap_present_mode = VK_PRESENT_MODE_FIFO_KHR;
                                                          // Extent (resolution) and present mode of the swapchain (set during swapchain creation)
VkCommandPool       g_cmdpool           = VK_NULL_HANDLE; // Command pool for allocating command buffers
VkCommandBuffer     *g_cmdbuf           = NULL;           // Command buffers for recording rendering commands (per-frame)
VkSemaphore         *g_img_avail        = NULL;           // Semaphores signaled when an image is available for rendering (per-frame)
VkSemaphore         *g_render_fin       = NULL;           // Semaphores signaled when rendering is finished (per-frame)
VkFence             *g_fences           = NULL;           // Fences for synchronizing CPU-GPU operations (per-frame)
VkFence             *g_image_in_flight  = NULL;           // Per-swapchain-image fence indicating which frame is using the image
uint32_t            g_frames_in_flight  = 0;              // Number of frames we allow to be in flight (set dynamically)
uint32_t            g_frame_index       = 0;              // Monotonic frame index used by frame/sync logic
VkPipeline          g_graphics_pipeline = VK_NULL_HANDLE; // Graphics pipeline defining the fixed-function and programmable stages of rendering
VkPipeline          g_graphics_pipeline_palette = VK_NULL_HANDLE; // Palette (debug, untextured) variant of the mesh pipeline
VkPipelineLayout    g_pipeline_layout   = VK_NULL_HANDLE; // Pipeline layout defining resource bindings for the graphics pipeline
VkShaderModule      g_vert_shader       = VK_NULL_HANDLE; // Vertex shader module for rendering
VkShaderModule      g_frag_shader       = VK_NULL_HANDLE; // Fragment shader module for rendering
VkShaderModule      g_frag_shader_palette = VK_NULL_HANDLE; 
                                                          // Fragment shader module for the palette debug view
int                 g_graphics_family   = -1;             // queue family index for graphics
int                 g_present_family    = -1;             // queue family index for present
VkImage             g_depth_image       = VK_NULL_HANDLE; // Depth image for depth testing
VkDeviceMemory      g_depth_image_mem   = VK_NULL_HANDLE; // Memory backing the depth image
VkImageView         g_depth_image_view  = VK_NULL_HANDLE; // Image view for the depth image
VkFormat            g_depth_format      = VK_FORMAT_UNDEFINED;
                                                          // Depth format used for the depth image (set during depth image creation)



VkDescriptorSetLayout   g_descriptor_set_layout     = VK_NULL_HANDLE; 
                                                          // Descriptor set layout defining the structure of descriptor sets (resources bound to shaders)
VkDescriptorPool        g_descriptor_pool           = VK_NULL_HANDLE; 
                                                          // Descriptor pool for allocating descriptor sets

int                     g_memory_budget_supported   = 0;  // Flag indicating if the VK_EXT_memory_budget extension is supported (for querying GPU memory usage)
float                   g_max_sampler_anisotropy    = 0.0f;
                                                          // Maximum supported anisotropy level for texture sampling (queried from physical device properties)


// Number of samples for multisampling
VkSampleCountFlagBits   g_msaa_samples              = VK_SAMPLE_COUNT_1_BIT;
// Multisampled color image for MSAA
VkImage                 g_msaa_color_image          = VK_NULL_HANDLE;
// Memory backing the multisampled color image
VkDeviceMemory          g_msaa_color_mem            = VK_NULL_HANDLE;
// Image view for the multisampled color image
VkImageView             g_msaa_color_view           = VK_NULL_HANDLE;

int                     g_vsync_enabled             = VSYNC_DEFAULT_ENABLED;
int                     g_lighting_enabled          = ENABLE_LIGHTING;

vk_dispatch_t           g_dldi                      = {0};                         
                                                            // dispatch table for dynamically loaded device-level functions (extension functions that may not be present on all devices or platforms)
VkDebugUtilsMessengerEXT g_debug_messenger = VK_NULL_HANDLE;
                                                            // Debug messenger for receiving validation layer messages (if debug utils extension is available and enabled)
