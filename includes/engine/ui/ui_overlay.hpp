#ifndef SCOP_UI_OVERLAY_HPP
#define SCOP_UI_OVERLAY_HPP

#include "engine/ui/ui_batch.hpp"
#include "engine/input/input_manager.hpp"

#include <string>
#include <vector>

/** Scene/renderer facts the overlay needs to draw one frame.
 * built by Engine, which owns the scene and renderer. */
struct s_UiFrameInputs {
    float screen_w = 0;
    float screen_h = 0;

    std::string selected_name;                  /** name of the currently selected mesh, or empty */
    bool        has_selection       = false;
    bool        multiple_objects    = false;    /** only show the "(Tab)" hint when there's something to cycle to */

    float axis_x[3] = {1, 0, 0};
    float axis_y[3] = {0, 1, 0};
    float axis_z[3] = {0, 0, 1};
    
    std::size_t mesh_count          = 0;
    std::size_t total_vertices      = 0;
    std::size_t total_materials     = 0;

    std::vector<std::string> device_info_lines; /** Vulkan device/setup info, one per line */
    
    std::string anisotropy_label;
    std::string msaa_label;
    std::string vsync_label;
};

/**
 * Hand-rolled immediate-mode debug/GUI overlay: 
 * an always-visible view-mode/selection label and compass,
 * and a F3-toggled debug panel.
 * 
 * Owns every bit of UI-only state (toggles, texture crossfade, smoothed
 * F3 stats) and builds one UiBatch per frame from a s_UiFrameInputs
 * snapshot of the scene/renderer.
 * */
class UiOverlay {
    public:
        UiOverlay()                             = default;
        UiOverlay(const UiOverlay &)            = default;
        UiOverlay &operator=(const UiOverlay &) = default;
        UiOverlay(UiOverlay &&)                 = default;
        ~UiOverlay()                            = default;

        void            handle_input(const InputState &in);
        void            update(float dt);

        const UiBatch   &build(const s_UiFrameInputs &inputs);

        bool            show_textured() const;
        bool            show_grid() const;
        float           texture_mix() const;

    private:
        void                        _draw_mode_label(const s_UiFrameInputs &inputs);
        void                        _draw_compass(const s_UiFrameInputs &inputs);
        void                        _draw_debug_panel(const s_UiFrameInputs &inputs);
        std::vector<std::string>    _build_debug_lines(const s_UiFrameInputs &inputs);

        UiBatch _batch;

        bool    _show_debug             = false;    /** F3 toggles the debug panel, off by default */
        bool    _show_grid              = false;     /** F4 toggles the reference grid, off by default */
        bool    _show_textured          = false; /** 'T' toggles palette (default) <-> real textures */
        float   _texture_mix            = 0.0f;    /** animates toward _show_textured over ~0.3s (smooth crossfade) */

        /* Smoothed stats for the F3 debug panel, updated every frame. */

        float   _smoothed_fps           = 0.0f;
        float   _smoothed_sys_cpu       = 0.0f;
        float   _smoothed_mem_used_mb   = 0.0f;
        float   _smoothed_mem_total_mb  = 0.0f;
        float   _smoothed_proc_cpu      = 0.0f;
        float   _smoothed_rss_mb        = 0.0f;
        float   _smoothed_gpu_used_mb   = 0.0f;
        float   _smoothed_gpu_budget_mb = 0.0f;
        float   _last_fps               = 0.0f;
};

#endif /* SCOP_UI_OVERLAY_HPP */
