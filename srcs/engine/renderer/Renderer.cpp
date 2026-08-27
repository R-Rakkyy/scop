#include "engine/renderer/renderer.hpp"
#include "config.h"
#include "utils/obj/obj_material.hpp"
#include "utils/obj/mtl_loader.hpp"
#include "iostream/logger.hpp"

#include "engine/vk/device/instance.h"
#include "engine/vk/device/device.h"
#include "engine/vk/swapchain/swapchain.h"
#include "engine/vk/swapchain/command.h"
#include "engine/vk/swapchain/images.h"
#include "engine/vk/pipeline/shader.h"
#include "engine/vk/pipeline/pipeline.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/mesh.h"
#include "engine/vk/render.h"
#include "engine/vk/resources/texture.h"
#include "engine/vk/resources/descriptors.h"
#include "engine/vk/pipeline/grid_pipeline.h"
#include "engine/vk/pipeline/ui_pipeline.h"
#include "engine/vk/resources/buffer.h"
#include "engine/ui/ui_font.hpp"

#include <array>
#include <vulkan/vulkan.h>

bool Renderer::_init_core(int &out_gf, int &out_pf)
{
    ftpp::log::info("Renderer: initializing Vulkan instance and device");

    if (!vk_instance_create(ENABLE_VALIDATION_LAYERS)) {                ftpp::log::error("Renderer: vk_instance_create failed");            return false; }
    if (!vk_surface_create(_window)) {                                  ftpp::log::error("Renderer: vk_surface_create failed");             return false; }
    if (!vk_pick_physical_and_queues(&out_gf, &out_pf)) {               ftpp::log::error("Renderer: vk_pick_physical_and_queues failed");   return false;}

    if (g_physical != VK_NULL_HANDLE) { // should be always true
        VkPhysicalDeviceProperties  props;
        vkGetPhysicalDeviceProperties(g_physical, &props);
        ftpp::log::debug("Renderer: Physical device: ", props.deviceName, " (driver=", props.driverVersion, " apiVersion=", props.apiVersion, ")");
    }

    if (!vk_create_device_and_queues(out_gf, out_pf)) {                 ftpp::log::error("Renderer: vk_create_device_and_queues failed");   return false; }
    ftpp::log::debug("Renderer: logical device and queues created");

    if (!vk_create_swapchain(out_gf, out_pf)) {                         ftpp::log::error("Renderer: vk_create_swapchain failed");           return false; }
    if (!vk_images_create_from_swapchain()) {                           ftpp::log::error("Renderer: vk_images_create_from_swapchain failed"); return false; }
    if (!vk_command_init(out_gf)) {                                     ftpp::log::error("Renderer: vk_command_init failed");               return false; }
    if (!vk_render_init(out_gf)) {                                      ftpp::log::error("Renderer: vk_render_init failed");                return false; }
    if (!vk_grid_init()) {                                              ftpp::log::error("Renderer: vk_grid_init failed");                  return false; }

    if (!vk_descriptors_init_layout()) {                                ftpp::log::error("Renderer: vk_descriptors_init_layout failed");    return false; }
    return true;
}

/* Fallback textures used by materials that don't define (or fail to
 * decode) a diffuse/bump map:
 * - 'resources/base_texture.png' stands in for a missing diffuse map,
 * and a neutral "flat" bump makes the bump modulation term in the fragment shader a no-op.
 * If the base texture is missing, fall back further to a flat white pixel. */
bool Renderer::_init_fallback_textures()
{
    ftpp::log::info("Renderer: creating fallback textures");

    /* base texture case */
    ObjTexture base_tex{};
    if (decode_texture_file("resources/base_texture.png", base_tex)) {
        _base_texture = vk_texture_create(base_tex.rgba.get(), base_tex.width, base_tex.height);
    }

    /* no base texture */
    if (_base_texture < 0) {
        ftpp::log::error("Renderer: resources/base_texture.png missing/undecodable, using flat white fallback");
        const std::array<uint8_t, 4> white_px = {255, 255, 255, 255};
        _base_texture = vk_texture_create(white_px.data(), 1, 1);
    }

    /* flat bump texture: neutral bump map (no bump effect) */
    const std::array<uint8_t, 4> flat_bump_px = {128, 128, 255, 255}; // normal pointing straight out of the surface
    _flat_bump_texture = vk_texture_create(flat_bump_px.data(), 1, 1);


    if (_base_texture < 0 || _flat_bump_texture < 0) {                  ftpp::log::error("Renderer: failed to create fallback textures");           return false; }
    return true;
}

bool Renderer::_init_ui_resources()
{
    ftpp::log::info("Renderer: creating UI font atlas");

    std::vector<uint8_t> font_rgba  =   build_ui_solid_texture();
    _font_texture                   =   vk_texture_create(font_rgba.data(), UI_SOLID_TEXTURE_WIDTH, UI_SOLID_TEXTURE_HEIGHT);
    _font_descriptor                =   (_font_texture >= 0) ? vk_descriptors_create_for_ui(_font_texture) : -1;
    
    if (_font_texture < 0 || _font_descriptor < 0) {                    ftpp::log::error("Renderer: failed to create UI font atlas texture/descriptor");    return false; }
    if (!vk_ui_pipeline_init()) {                                       ftpp::log::error("Renderer: vk_ui_pipeline_init failed");                   return false; }

    VkDeviceSize ui_buf_size = sizeof(float) * UI_VERTEX_CAPACITY * 8; /* 8 floats/vertex */
    if (!vk_create_buffer(ui_buf_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           &_ui_vbuf, &_ui_vmem)) {
        ftpp::log::error("Renderer: failed to create UI vertex buffer");
        return false;
    }
    vkMapMemory(g_device, _ui_vmem, 0, ui_buf_size, 0, &_ui_mapped);
    return true;
}

bool Renderer::_init_shaders_and_pipelines(int graphics_family)
{
    ftpp::log::info("Renderer: loading shaders and creating graphics pipeline");

    VkShaderModule vert             =   VK_NULL_HANDLE;
    VkShaderModule frag             =   VK_NULL_HANDLE;
    VkShaderModule palette_frag     =   VK_NULL_HANDLE;
    
    /* vertex and (textured | palette) fragment shaders */
    if (vk_create_shader_module_from_file("./shaders/spv/vert.spv", &vert)) {
        g_vert_shader       =   vert;
        ftpp::log::debug("Renderer: vertex shader loaded");
    } else { ftpp::log::error("Renderer: vertex shader not loaded"); }

    if (vk_create_shader_module_from_file("./shaders/spv/frag.spv", &frag)) {
        g_frag_shader       =   frag;
        ftpp::log::debug("Renderer: fragment shader loaded");
    } else { ftpp::log::error("Renderer: fragment shader not loaded"); }
    
    if (vk_create_shader_module_from_file("./shaders/spv/palette/frag.spv", &palette_frag)) {
        g_frag_shader_palette = palette_frag;
        if (!vk_pipeline_create_palette(g_vert_shader, g_frag_shader_palette)) {
            ftpp::log::error("Renderer: vk_pipeline_create_palette failed");
            return false;
        }
    } else { ftpp::log::error("Renderer: palette fragment shader not loaded"); }

    /* graphics pipeline: vertex + textured fragment shader */
    if (g_vert_shader != VK_NULL_HANDLE && g_frag_shader != VK_NULL_HANDLE) {
        if (!vk_pipeline_create_from_shaders(g_vert_shader, g_frag_shader, graphics_family)) {
            ftpp::log::error("Renderer: vk_pipeline_create_from_shaders failed");
            return false;
        }
        ftpp::log::debug("Renderer: pipeline created (handle=", static_cast<void*>(g_graphics_pipeline), ")");
    } else { ftpp::log::error("Renderer: pipeline not created, missing shader modules"); }

    return true;
}

Renderer::Renderer(GLFWwindow *window) : _window(window)
{
    int gf = -1, pf = -1;
    if (!_init_core(gf, pf))                return;
    if (!_init_fallback_textures())         return;
    if (!_init_ui_resources())              return;
    if (!_init_shaders_and_pipelines(gf))   return;

    ftpp::log::debug("Renderer: swapchain format=", static_cast<int>(g_swap_image_format),
                     " extent=", g_swap_extent.width, "x", g_swap_extent.height,
                     " images=", g_image_count, " presentMode=", static_cast<int>(g_swap_present_mode));

    _build_device_info_lines(gf, pf);
    ftpp::log::info("Renderer: Vulkan core initialized, ready to draw frames");
}

Renderer::~Renderer()
{
    ftpp::log::info("Renderer: cleaning up Vulkan core");

    if (_ui_mapped)                 { vkUnmapMemory(g_device, _ui_vmem);        _ui_mapped = nullptr; }
    if (_ui_vbuf != VK_NULL_HANDLE) { vk_destroy_buffer(_ui_vbuf, _ui_vmem);    _ui_vbuf = VK_NULL_HANDLE; }

    vk_ui_pipeline_destroy();
    vk_grid_destroy();
    vk_render_cleanup();
    vk_descriptors_destroy_all();
    vk_texture_destroy_all();
    vk_images_destroy();
    vk_destroy_swapchain();
    vk_mesh_destroy_all();
    vk_destroy_device();
    vk_surface_destroy();
    vk_instance_destroy();
}
