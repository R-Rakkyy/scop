#include "engine/engine.hpp"
#include "config.h"
#include "engine/vk/context.h"
#include "iostream/logger.hpp"
#include "utils/math/matrix_utils.hpp"

Engine::Engine(GLFWwindow *window, std::vector<ObjMesh> &meshes): _renderer(window), _scheduler("ENG.") {

    ftpp::log::info("Engine: loading ", meshes.size(), " mesh(es)");

    _input.install(window);

    for (ObjMesh &mesh : meshes) {
        int mesh_index = _renderer.upload_mesh(mesh);
        if (mesh_index < 0) {
            ftpp::log::error("Engine: mesh upload failed for '", mesh.name, "'");
            continue;
        }
        
        std::string name = mesh.name.empty() ? ("mesh " + std::to_string(_scene.count())) : mesh.name;
        _scene.add_mesh(mesh_index, name, mesh.aabb_min, mesh.aabb_max);
        
        _total_vertices     += mesh.vertices.size();
        _total_materials    += mesh.materials.size();
    }

    ftpp::log::info("Engine: loaded ", _scene.count(), " mesh(es), ", _total_vertices, " vertices");
}

Engine::~Engine()
{
    stop();
}

void Engine::start()
{
    _scheduler.start();
    _chrono.start();

    _scheduler.schedule_forever([this] { _tick(); }, PROJECT_RENDER_FRAME_MS);

    ftpp::log::info("Engine: started");
}

void Engine::stop()
{
    _scheduler.stop();

    if (g_device != VK_NULL_HANDLE) vkDeviceWaitIdle(g_device);
    
    ftpp::log::info("Engine: stopped");
}

float Engine::_consume_dt()
{
    float dt = static_cast<float>(_chrono.elapsed_time()) / 1000.0f;
    _chrono.reset();
    _chrono.start();
    return dt;
}

std::vector<RenderInstance> Engine::_build_instances(const ftpp::Matrix &vp) const
{
    const std::vector<MeshEntry>    &meshes = _scene.meshes();
    std::vector<RenderInstance>     instances;
    instances.reserve(meshes.size());

    for (std::size_t i = 0; i < meshes.size(); ++i) {
        if (_scene.hide_others() && i != _scene.selected()) continue; /* H: only draw the selected mesh */

        const MeshEntry &entry  = meshes[i];
        ftpp::Matrix     model  = model_matrix(entry.transform);
        ftpp::Matrix     mvp    = vp * model;

        instances.push_back(RenderInstance{ entry.renderer_index, mvp, model });
    }
    return instances;
}

s_UiFrameInputs Engine::_build_ui_inputs() const
{
    s_UiFrameInputs               ui_inputs;
    ui_inputs.screen_w  =       static_cast<float>(g_swap_extent.width);
    ui_inputs.screen_h  =       static_cast<float>(g_swap_extent.height);

    const std::vector<MeshEntry> &meshes = _scene.meshes();
    if (_scene.count() > 0 && _scene.selected() < meshes.size()) {
        ui_inputs.selected_name     = meshes[_scene.selected()].name;       // name of selected model
        ui_inputs.has_selection     = true;
        ui_inputs.multiple_objects  = meshes.size() > 1;                    // true if more than one model is loaded

        /* used to get rotation axes for the indicator (top-right) */
        const MeshTransform &sel    = meshes[_scene.selected()].transform;
        rotated_axes(mesh_rotation(sel), ui_inputs.axis_x, ui_inputs.axis_y, ui_inputs.axis_z);
    }

    ui_inputs.mesh_count            = meshes.size();
    ui_inputs.total_vertices        = _total_vertices;
    ui_inputs.total_materials       = _total_materials;
    ui_inputs.device_info_lines     = _renderer.device_info_lines();
    ui_inputs.anisotropy_label      = g_max_sampler_anisotropy > 0.0f
        ? std::to_string(static_cast<int>(g_max_sampler_anisotropy)) + "x" : "n/a";
    ui_inputs.msaa_label = g_msaa_samples > VK_SAMPLE_COUNT_1_BIT
        ? std::to_string(static_cast<int>(g_msaa_samples)) + "x" : "off";

    ui_inputs.vsync_label           = g_vsync_enabled ? "on" : "off";

    return ui_inputs;
}

void Engine::_tick()
{
    if (!_renderer.is_ready()) { _renderer.try_recover(); return; } /* recovr from fail swapchain recreate */

    float           dt          =   _consume_dt();
    InputState      in          =   _input.snapshot();

    _ui.handle_input(in);
    if (in.toggle_vsync) _renderer.toggle_vsync();

    _scene.update(in, dt);
    _ui.update(dt);

    /* same vp for every vertex, so we compute it before. */
    float           aspect      =   _renderer.aspect_ratio();
    ftpp::Matrix    vp          =   _scene.projection_matrix(aspect) * _scene.view_matrix();

    std::vector<RenderInstance> instances   = _build_instances(vp);
    const UiBatch               &ui_batch   = _ui.build(_build_ui_inputs());

    _renderer.draw_frame(vp, instances, ui_batch.vertices(), _ui.texture_mix(), _ui.show_grid(), _scene.cam_pos());
}
