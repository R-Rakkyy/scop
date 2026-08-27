#ifndef SCOP_CONFIG_H
#define SCOP_CONFIG_H

/* ------------------------- App Config ------------------------- */
#define APP_NAME "scop"
#define APP_VERSION_VARIANT 0
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 2
#define APP_VERSION_PATCH 0
#define APP_VERSION         ((((uint32_t)(APP_VERSION_VARIANT)) << 29U) | (((uint32_t)(APP_VERSION_MAJOR)) << 22U) | (((uint32_t)(APP_VERSION_MINOR)) << 12U) | ((uint32_t)(APP_VERSION_PATCH)))

#define ENGINE_NAME "ER54GER54G654G6R5EG46ZE5G46R5ZG46GZ5RG"
#define ENGINE_VERSION_VARIANT 0
#define ENGINE_VERSION_MAJOR 0
#define ENGINE_VERSION_MINOR 1
#define ENGINE_VERSION_PATCH 0
#define ENGINE_VERSION      ((((uint32_t)(ENGINE_VERSION_VARIANT)) << 29U) | (((uint32_t)(ENGINE_VERSION_MAJOR)) << 22U) | (((uint32_t)(ENGINE_VERSION_MINOR)) << 12U) | ((uint32_t)(ENGINE_VERSION_PATCH)))

#define APP_LOG_LEVEL ftpp::LogLevel::INFO

/* ------------------------ Window Config ----------------------- */
#define WINDOW_DEFAULT_WIDTH    800
#define WINDOW_DEFAULT_HEIGHT   600
#define WINDOW_DEFAULT_TITLE    "scop"
#define WINDOW_RESIZABLE        1

/* ------------------------ Vulkan Config ----------------------- */
#include <vulkan/vulkan.h>

/* Requested Vulkan API version. See vk/instance.c's vk_instance_create. */
#define VULKAN_API_VERSION              VK_API_VERSION_1_3 // (touhcing it will not change the actual Vulkan version)

/* Preferred swapchain surface format/color space; falls back to whatever
 * the surface offers first if this exact pair isn't supported.
 * > See vk/swapchain.c (choose_surface_format). */
#define VK_DESIRED_FORMAT               VK_FORMAT_B8G8R8A8_UNORM
#define VK_DESIRED_COLOR_SPACE          VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

/* Preferred texture format for all uploaded textures; falls back to whatever
 * the device supports first if this exact format isn't supported.
 * > See vk/texture.c (vk_texture_create). */
#define SCOP_TEXTURE_FORMAT             VK_FORMAT_R8G8B8A8_UNORM

/* Depth buffer format. See vk/depth.c (vk_depth_create). */
#define VK_DEPTH_FORMAT_DEFAULT         VK_FORMAT_D32_SFLOAT

/* 1 = request MAILBOX/FIFO (tear-free) by default, 0 = request IMMEDIATE
 * (uncapped, may tear) by default. Either way 'F5' flips g_vsync_enabled
 * > See vk/swapchain.c (choose_present_mode) and Renderer::toggle_vsync. */
#define VSYNC_DEFAULT_ENABLED           1

/* 1 = force VK_PRESENT_MODE_CHOICE below regardless of
 * VSYNC_DEFAULT_ENABLED/F5; 0 = let choose_present_mode() keep deriving it
 * from g_vsync_enabled instead (the normal case, VK_PRESENT_MODE_CHOICE
 * ignored). Forcing only takes effect if the surface actually reports the
 * mode as available; falls back to FIFO otherwise.
 * See vk/swapchain.c (choose_present_mode). */
#define VK_PRESENT_MODE_FORCE           0
#define VK_PRESENT_MODE_CHOICE          VK_PRESENT_MODE_FIFO_KHR

/* Requested swapchain image count, or 0 for auto
 * > See vk/swapchain.c (build_swapchain_create_info). */
#define VK_MIN_IMAGE_COUNT              0

/* Extra VkImageUsageFlags OR'd onto the swapchain images' mandatory
 * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, or 0 for none. E.g.
 * VK_IMAGE_USAGE_TRANSFER_DST_BIT to allow blitting into a swapchain image
 * (screenshot tooling, etc).
 * > See vk/swapchain.c (build_swapchain_create_info). */
#define VK_IMAGE_USAGE_EXTRA            0

/* 1 = enable MSAA (sample count chosen per-device, capped at
 * VK_MSAA_MAX_SAMPLES below), 0 = fully disable it (every pipeline and the
 * depth image run at VK_SAMPLE_COUNT_1_BIT, no multisample image is ever
 * allocated).
 * > See vk/logical_device.c's choose_msaa_samples. */
#define ENABLE_MSAA                     1

/* Highest MSAA sample count to ever request. choose_msaa_samples() is
 * already the "auto" picker: it queries what the device actually supports
 * and takes the highest value <= this cap.
 * this cap IS the auto behavior's ceiling.
 * 
 * Lower it for less GPU/bandwidth cost; VK_SAMPLE_COUNT_1_BIT is
 * equivalent to ENABLE_MSAA 0. Ignored when ENABLE_MSAA is 0.
 * > See vk/logical_device.c's choose_msaa_samples. */
#define VK_MSAA_MAX_SAMPLES             VK_SAMPLE_COUNT_64_BIT

/* 1 = request anisotropic texture filtering when the device supports it
 * > (See vk/logical_device (detect_and_fill_supported_features))
 * 0 = never request the feature, so every sampler falls back to plain
 * trilinear filtering 
 * > See vk/texture.c (ensure_sampler). */
#define ENABLE_ANISOTROPIC_FILTERING    1

/* 0 = off, 1 = on.
 * Enables VK_LAYER_KHRONOS_validation (if present on the
 * system) plus VK_EXT_debug_utils message logging.
 * > See vk/instance.c (vk_instance_create).
 * */
#define ENABLE_VALIDATION_LAYERS        (0 || APP_LOG_LEVEL == ftpp::LogLevel::DEBUG)

/* 1 = the textured pipeline's fragment shader applies the ambient/
 * diffuse/specular lighting terms as usual, 
 * 0 = it skips all of them and outputs
 * the material's raw (Kd-tinted) diffuse texture untouched
 * > See shaders/glsl/frag.glsl. */
#define ENABLE_LIGHTING                 1

/* Fixed-size registries for uploaded GPU resources */
#define VK_MESH_MAX                     256 /* one per mesh in the .obj file. */
#define VK_TEXTURE_MAX                  512 /* one per texture in the .obj file */
#define VK_DESCRIPTOR_MAX               512 /* one set per material run. */

/* Mip-chain depth cap: 0 = uncapped (full chain down to 1x1), 1 = mipmaps
 * disabled, N = cap at N levels.
 * > See engine/vk/resources/texture.c (vk_texture_create). */
#define VK_TEXTURE_MIP_LEVELS_CAP       0

/* ----------------------- Project Config ---------------------- */

/* Render timing: how often the scheduler ATTEMPTS a tick, not a target FPS. */
#define PROJECT_RENDER_FRAME_MS         1

#define SCOP_BACKGROUND_COLOR           0.1647f, 0.2196f, 0.6078f, 1.0f     /* #2a389b */

/* ------------------------- Grid Config ------------------------- */
/* Floor-grid extent/spacing (world units). See vk/pipeline/grid_pipeline.c. */
#define GRID_HALF_EXTENT                20
#define GRID_SPACING                    1
#define GRID_LINES_PER_AXIS             ((2 * GRID_HALF_EXTENT / GRID_SPACING) + 1)
#define GRID_VERTS_PER_PLANE            ((uint32_t)GRID_LINES_PER_AXIS * 4)
#define GRID_PLANE_COUNT                3
#define GRID_AXIS_COLOR                 0.55f, 0.55f, 0.62f, 0.5f           /* #8c8ca0 */
#define GRID_MINOR_COLOR                0.220f, 0.239f, 0.302f, 0.18f       /* #383d4d */

/* ------------------------ Scene Config ------------------------ */
/* C++-only (SceneManager/mesh_transform's tuning) */
#ifdef __cplusplus
#include <cstddef>

constexpr double SCENE_NEAR_PLANE_FACTOR= 0.01;
constexpr double SCENE_NEAR_PLANE_MIN   = 0.0001;
constexpr double SCENE_FOV_RADIANS      = (60 * 3.14159265358979323846 / 180.0);
constexpr float  SCENE_MOVE_SPEED       = 3.0f;    /* units/sec,        held-key translation */
constexpr float  SCENE_MOUSE_ROT_SPEED  = 0.008f;  /* radians/pixel,    drag rotation */
constexpr float  SCENE_ARROW_ROT_SPEED  = 1.5f;    /* radians/sec,      arrow-key base-axis rotation */

/* ---------------------- UI Overlay Config ---------------------- */
/* C++-only (UiOverlay's layout/color/timing tuning) */
constexpr float        UI_STAT_SMOOTHING            = 0.0035f;                          /* exponential moving average alpha for smoothing the FPS and other stats in the F3 debug panel */

constexpr float        UI_TEXT_SCALE                = 1.5f;
constexpr float        UI_TEXT_COLOR[4]             = {0.902f, 0.918f, 0.925f, 1.0f};   /* #e6eaec */
constexpr float        UI_AXIS_X_COLOR[4]           = {0.90f, 0.25f, 0.25f, 1.0f};      /* #e64040 */
constexpr float        UI_AXIS_Y_COLOR[4]           = {0.30f, 0.85f, 0.35f, 1.0f};      /* #4ca858 */
constexpr float        UI_AXIS_Z_COLOR[4]           = {0.30f, 0.55f, 0.95f, 1.0f};      /* #4c8cf2 */

constexpr float        UI_MARGIN                    = 20.0f;                            /* distance from any screen edge */
constexpr float        UI_ROW_GAP                   = 10.0f;                            /* extra space added to each text row's height */
constexpr float        UI_COLUMN_GAP                = 24.0f;                            /* horizontal gap between debug-panel columns */
constexpr std::size_t  UI_DEBUG_ROWS_PER_COLUMN     = 3;                                /* wrap into a new column instead of growing past this many rows */

// Note: this thing has NOTHING to do in the UI, what am i even doing ?
constexpr float        UI_CROSSFADE_SECONDS         = 0.5f;                             /* texture_mix ramp duration for the palette<->textured crossfade */

#endif /* __cplusplus */

#endif /* SCOP_CONFIG_H */
