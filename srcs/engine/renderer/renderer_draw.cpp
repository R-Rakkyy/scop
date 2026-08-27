#include "engine/renderer/renderer.hpp"
#include "utils/math/matrix_utils.hpp"

#include "engine/vk/context.h"
#include "engine/vk/render.h"
#include "engine/vk/swapchain/swapchain.h"

#include <algorithm>
#include <cstring>

namespace {

s_VkDrawCall build_draw_call(int mesh_handle, uint32_t first_vertex, uint32_t vertex_count, int descriptor_handle,
                            const float mvp[16], const float model[16], const float cam_pos[3])
{
    s_VkDrawCall              call{};

    call.mesh_handle        = mesh_handle;
    call.first_vertex       = first_vertex;
    call.vertex_count       = vertex_count;
    call.descriptor_handle  = descriptor_handle;

    // Copy the 4x4 matrices into the draw call's mvp and model arrays
    std::copy(mvp, mvp + 16, call.mvp);
    std::copy(model, model + 16, call.model);

    /* Note: std140 
     *  '.w' carries g_lighting_enabled through to the shader (see shaders/glsl/frag.glsl)
     *  I use this as a trick to reuses this otherwise-padding component
     *  instead of growing the push constant further. (i avoid needing descriptors) */
    call.cam_pos[0] = cam_pos[0];
    call.cam_pos[1] = cam_pos[1]; 
    call.cam_pos[2] = cam_pos[2];
    call.cam_pos[3] = g_lighting_enabled ? 1.0f : 0.0f;
    return call;
}

s_VkUiDrawCall build_ui_draw_call(const std::vector<float> &ui_vertices, uint32_t capacity,
                                 void *mapped, VkBuffer vbuf, int font_descriptor)
{
    s_VkUiDrawCall                  ui{};
    uint32_t ui_floats          =   static_cast<uint32_t>(std::min<size_t>(ui_vertices.size(), capacity * 8)); // 8 floats per vertex

    if (mapped && ui_floats > 0) {
        std::memcpy(mapped, ui_vertices.data(), ui_floats * sizeof(float));

        ui.buffer               =   vbuf;
        ui.vertex_count         =   ui_floats / 8;
        ui.descriptor_handle    =   font_descriptor;
    }
    return ui;
}

} // namespace

void Renderer::draw_frame(const ftpp::Matrix &view_proj, const std::vector<RenderInstance> &instances, const std::vector<float> &ui_vertices, float texture_mix, bool show_grid, const float cam_pos[3])
{
    if (!is_ready()) return;

    /* We have one drawcall per mesh instance, and each drawcall may have multiple material runs. */
    std::vector<s_VkDrawCall>     calls;
    for (const RenderInstance &inst : instances) {
        if (inst.mesh_index < 0 || static_cast<size_t>(inst.mesh_index) >= _meshes.size()) continue;

        const MeshGpu &gpu = _meshes[static_cast<size_t>(inst.mesh_index)];
        float mvp[16]; 
        float model[16];

        mat4_to_floats(inst.mvp, mvp);
        mat4_to_floats(inst.model, model);
        
        for (const MaterialRun &run : gpu.runs)
            calls.push_back(build_draw_call(gpu.mesh_handle, run.first, run.count, run.descriptor_handle, mvp, model, cam_pos));
    }

    float grid_mvp[16];
    mat4_to_floats(view_proj, grid_mvp);

    s_VkUiDrawCall ui     = build_ui_draw_call(ui_vertices, UI_VERTEX_CAPACITY, _ui_mapped, _ui_vbuf, _font_descriptor);

    vk_render_draw_frame(show_grid ? grid_mvp : nullptr,
                        calls.data(), static_cast<uint32_t>(calls.size()), 
                        ui.vertex_count > 0 ? &ui : nullptr, 
                        texture_mix);
}

float Renderer::aspect_ratio() const
{
    if (g_swap_extent.height == 0)
        return 1.0f;

    return static_cast<float>(g_swap_extent.width) / static_cast<float>(g_swap_extent.height);
}
