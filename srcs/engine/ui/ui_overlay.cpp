#include "engine/ui/ui_overlay.hpp"
#include "config.h"
#include "utils/stats/system_stats.h"

#include <algorithm>
#include <vector>

namespace {

// Exponential moving average: smooths a noisy sample stream (e.g. FPS) over time.
float ema(float prev, float sample, float alpha)
{
    return (prev <= 0.0f) ? sample : prev * (1.0f - alpha) + sample * alpha;
}

/* A single line can be wider than the screen on its own.
 * I try to trim it in the WORSE WAY POSSBILE. */
std::string truncate_to_width(const UiBatch &batch, const std::string &text, float scale, float max_w)
{
    if (batch.text_width(text, scale) <= max_w) return text;

    const std::string   ellipsis    =   "...";
    std::string         s           =   text;

    while (!s.empty() && batch.text_width(s + ellipsis, scale) > max_w) s.pop_back();
    
    return s.empty()    ? ellipsis  :   s + ellipsis;
}

} // namespace

void UiOverlay::handle_input(const InputState &in)
{
    if (in.toggle_info)    _show_debug    = !_show_debug;
    if (in.toggle_texture) _show_textured = !_show_textured;
    if (in.toggle_grid)    _show_grid     = !_show_grid;
}

void UiOverlay::update(float dt)
{
    /* Smooth crossfade instead of an instant cut: ramp texture_mix toward the target view over UI_CROSSFADE_SECONDS. */
    float target    = _show_textured ? 1.0f : 0.0f;
    float step      = dt / UI_CROSSFADE_SECONDS;
    
    if (_texture_mix < target)      
        _texture_mix= std::min(target, _texture_mix + step);
    else if (_texture_mix > target)
        _texture_mix= std::max(target, _texture_mix - step);

    _last_fps       = (dt > 0.0f) ? (1.0f / dt) : _last_fps;
    _smoothed_fps   = ema(_smoothed_fps, _last_fps, UI_STAT_SMOOTHING);
}

const UiBatch &UiOverlay::build(const s_UiFrameInputs &inputs)
{
    _batch.clear();

                                _draw_mode_label(inputs);
    if (inputs.has_selection)   _draw_compass(inputs);
    if (_show_debug)            _draw_debug_panel(inputs);

    return _batch;
}

bool    UiOverlay::show_textured() const    {   return _show_textured;  }
bool    UiOverlay::show_grid() const        {   return _show_grid;      }
float   UiOverlay::texture_mix() const      {   return _texture_mix;    }

void UiOverlay::_draw_mode_label(const s_UiFrameInputs &inputs)
{

    const float row_h       = _batch.glyph_advance(UI_TEXT_SCALE) + UI_ROW_GAP;             // height of a single text row.
    const std::string mode  = _show_textured ? "View: Textured (T)" : "View: Palette (T)";
    _batch.push_text(UI_MARGIN, UI_MARGIN, UI_TEXT_SCALE, mode, UI_TEXT_COLOR);

    if (!inputs.selected_name.empty()) {
        const std::string sel = inputs.multiple_objects ? "[" + inputs.selected_name + "] (Tab)" : "[" + inputs.selected_name + "]";
        
        _batch.push_text(UI_MARGIN, UI_MARGIN + row_h, UI_TEXT_SCALE, sel, UI_TEXT_COLOR);
    }
}

void UiOverlay::_draw_compass(const s_UiFrameInputs &inputs)
{
    const float cx  = inputs.screen_w - UI_MARGIN - 20.0f;  // center of the compass circle
    const float cy  = UI_MARGIN + 20.0f;                    // center of the compass circle
    const float r   = 26.0f;                                // radius of the compass circle

    // Draw the 3 lines
    _batch.push_thick_line(cx, cy, inputs.axis_x[0] * r, -inputs.axis_x[1] * r, UI_AXIS_X_COLOR);
    _batch.push_thick_line(cx, cy, inputs.axis_y[0] * r, -inputs.axis_y[1] * r, UI_AXIS_Y_COLOR);
    _batch.push_thick_line(cx, cy, inputs.axis_z[0] * r, -inputs.axis_z[1] * r, UI_AXIS_Z_COLOR);

    // Draw the labels (x, y, z) at the end of each line
    const float label_scale = UI_TEXT_SCALE * 0.75f;
    auto label = [&](const float dir[3], const char *text, const float color[4]) {
        float lx = cx + dir[0] * (r + 10.0f) - _batch.glyph_advance(label_scale) * 0.5f;
        float ly = cy - dir[1] * (r + 10.0f);
        _batch.push_text(lx, ly, label_scale, text, color);
    };

    label(inputs.axis_x, "X", UI_AXIS_X_COLOR);
    label(inputs.axis_y, "Y", UI_AXIS_Y_COLOR);
    label(inputs.axis_z, "Z", UI_AXIS_Z_COLOR);
}

std::vector<std::string> UiOverlay::_build_debug_lines(const s_UiFrameInputs &inputs)
{
    t_SystemStats stats{};
    system_stats_sample(&stats); // sample the system stats once per frame.

    if (stats.sys_cpu_available) _smoothed_sys_cpu      = ema(_smoothed_sys_cpu, stats.sys_cpu_percent, UI_STAT_SMOOTHING);
    if (stats.mem_available) {
        _smoothed_mem_used_mb                           = ema(_smoothed_mem_used_mb, static_cast<float>(stats.mem_used_kb) / 1024.0f, UI_STAT_SMOOTHING);
        _smoothed_mem_total_mb                          = ema(_smoothed_mem_total_mb, static_cast<float>(stats.mem_total_kb) / 1024.0f, UI_STAT_SMOOTHING);
    }
    if (stats.cpu_available) _smoothed_proc_cpu         = ema(_smoothed_proc_cpu, stats.cpu_percent, UI_STAT_SMOOTHING);
    if (stats.rss_available) _smoothed_rss_mb           = ema(_smoothed_rss_mb, static_cast<float>(stats.rss_kb) / 1024.0f, UI_STAT_SMOOTHING);
    if (stats.gpu_mem_available) {
        _smoothed_gpu_used_mb                           = ema(_smoothed_gpu_used_mb, static_cast<float>(stats.gpu_mem_used_bytes) / (1024.0f * 1024.0f), UI_STAT_SMOOTHING);
        _smoothed_gpu_budget_mb                         = ema(_smoothed_gpu_budget_mb, static_cast<float>(stats.gpu_mem_budget_bytes) / (1024.0f * 1024.0f), UI_STAT_SMOOTHING);
    }

    std::vector<std::string> lines;
    lines.push_back("FPS: " + std::to_string(static_cast<int>(_smoothed_fps)));
    lines.push_back("CPU: " + (stats.sys_cpu_available ? std::to_string(static_cast<int>(_smoothed_sys_cpu)) + "%" : "n/a"));
    lines.push_back("MEM: " + (stats.mem_available
        ? std::to_string(static_cast<long>(_smoothed_mem_used_mb)) + "/" + std::to_string(static_cast<long>(_smoothed_mem_total_mb)) + " MB"
        : std::string("n/a")));
    lines.push_back("PROC CPU: " + (stats.cpu_available ? std::to_string(static_cast<int>(_smoothed_proc_cpu)) + "%" : std::string("n/a")));
    lines.push_back("PROC MEM: " + (stats.rss_available ? std::to_string(static_cast<long>(_smoothed_rss_mb)) + " MB" : std::string("n/a")));
    lines.push_back("GPU: " + (stats.gpu_mem_available
        ? std::to_string(static_cast<long>(_smoothed_gpu_used_mb)) + "/" + std::to_string(static_cast<long>(_smoothed_gpu_budget_mb)) + " MB"
        : std::string("n/a")));
    lines.push_back("AF: " + inputs.anisotropy_label);
    lines.push_back("MSAA: " + inputs.msaa_label);
    lines.push_back("VSync: " + inputs.vsync_label);
    lines.push_back(std::to_string(inputs.mesh_count) + " obj loaded");
    lines.push_back(std::to_string(inputs.total_vertices) + " verts, " + std::to_string(inputs.total_materials) + " mats");
    
    /** @attention device_info_lines contains the present mode (FIFO/MAILBOX) but isn't updated (should with F5). That is a known bug/issue. */
    for (const std::string &line : inputs.device_info_lines)
        lines.push_back(line);
    
    return lines;
}

void UiOverlay::_draw_debug_panel(const s_UiFrameInputs &inputs)
{
    std::vector<std::string> raw_lines = _build_debug_lines(inputs);
    if (raw_lines.empty()) return;

    /* Try as best as possible to fit the debug lines into a band at the bottom of the screen. */
    const float row_h           =   _batch.glyph_advance(UI_TEXT_SCALE) + UI_ROW_GAP;           // height of a single text row.
    const std::size_t rows      =   std::min(raw_lines.size(), UI_DEBUG_ROWS_PER_COLUMN);       // number of rows in a single column (wrap into a new column after this many rows).
    const float band_h          =   row_h * static_cast<float>(rows);                           // height of the debug panel band at the bottom of the screen.
    const float max_line_w      =   inputs.screen_w - (UI_MARGIN * 2.0f);                       // maximum width of a single line before it must be truncated to fit the screen.

    std::vector<std::string> lines;
    lines.reserve(raw_lines.size());
    for (const std::string &line : raw_lines)
        lines.push_back(truncate_to_width(_batch, line, UI_TEXT_SCALE, max_line_w));

    float x     = UI_MARGIN;
    float y0    = inputs.screen_h - band_h - UI_MARGIN; // bottom of the debug panel band (y coordinate of the first row).
    for (std::size_t col_start = 0; col_start < lines.size(); col_start += UI_DEBUG_ROWS_PER_COLUMN) {
        std::size_t col_end     =   std::min(col_start + UI_DEBUG_ROWS_PER_COLUMN, lines.size());       // index of the first line in the next column (or lines.size() if this is the last column).
        float col_w             =   0.0f;                                                               // width of the current column (max width of all lines in this column).
        for (std::size_t i = col_start; i < col_end; ++i)
            col_w = std::max(col_w, _batch.text_width(lines[i], UI_TEXT_SCALE));

        /* If the next column would go past the right edge of the screen, wrap to a new row. */
        if (x > UI_MARGIN && x + col_w > inputs.screen_w - UI_MARGIN) {
            x = UI_MARGIN;
            y0 -= band_h + UI_ROW_GAP;
        }

        for (std::size_t i = col_start; i < col_end; ++i) {
            float row = static_cast<float>(i - col_start);
            _batch.push_text(x, y0 + row_h * row, UI_TEXT_SCALE, lines[i], UI_TEXT_COLOR);
        }
        x += col_w + UI_COLUMN_GAP;
    }
}
