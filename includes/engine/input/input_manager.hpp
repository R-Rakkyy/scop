/**
 * @file input_manager.hpp
 * @note This implementation is what actively makes this world a worse place to live in.
 */
#ifndef SCOP_INPUT_MANAGER_HPP
#define SCOP_INPUT_MANAGER_HPP

#include "glfw/window.h"
#include <atomic>

/** Per-frame snapshot of held keys / mouse-drag.
 *  Scene uses those booleans to update camera and selected object transforms using dt. */
struct InputState {
    /** Shift: let user select all objects at once */
    bool shift_held         = false;

    /** WASD + Space/Ctrl: move the camera. */
    bool cam_move_fwd       = false;
    bool cam_move_back      = false;
    bool cam_move_left      = false;
    bool cam_move_right     = false;
    bool cam_move_up        = false;
    bool cam_move_down      = false;

    /** ARROW KEYS + RSPACE/Ctrl: move the selected object. **/
    bool obj_move_fwd       = false;
    bool obj_move_back      = false;
    bool obj_move_left      = false;
    bool obj_move_right     = false;
    bool obj_move_up        = false;
    bool obj_move_down      = false;

    /** Numpad 7/9, 4/6, 1/3: rotate the selected object around its base (world) X/Y/Z axes. */
    bool obj_rot_pitch_add  = false; /** pitch+, rotate around base X */
    bool obj_rot_pitch_sub  = false; /** pitch- */
    bool obj_rot_yaw_sub    = false; /** yaw-, rotate around base Y */
    bool obj_rot_yaw_add    = false; /** yaw+ */
    bool obj_rot_roll_add   = false; /** roll+, rotate around base Z */
    bool obj_rot_roll_sub   = false; /** roll- */

    bool select_next        = false; /** Tab: one-shot, advance mesh selection */
    bool toggle_info        = false; /** F3: one-shot, toggle debug overlay */
    bool toggle_texture     = false; /** T: one-shot, toggle palette/textured view */
    bool toggle_grid        = false; /** F4: one-shot, toggle the reference grid */
    bool reset_selected     = false; /** R: one-shot, reset selected mesh's transform */
    bool toggle_vsync       = false; /** F5: one-shot, toggle vsync (present mode) */
    bool toggle_hide_others = false; /** H: one-shot, toggle hiding all but the selected mesh */

    /** Mouse input. **/
    /** Note: on screen there is no z-coordinate */
    float mouse_dx          = 0;      /** Cursor delta while left button held, since last snapshot */
    float mouse_dy          = 0;
    float mouse_x           = 0;      /** Current cursor position (for UI hit-testing) */
    float mouse_y           = 0;
    bool  mouse_click       = false; /** Left button pressed edge (for UI hit-testing) */
};

/**
 * @class InputManager
 * @brief Owns the GLFW key/mouse callbacks and the raw input state they
 * mutate.
 * Held keys are booleans (set true on GLFW_PRESS, false on
 * GLFW_RELEASE) rather than accumulated deltas, so movement is smooth
 * instead of tied to OS key-repeat. Each field is read once per frame
 * via snapshot() from the render/update thread.
 */
class InputManager {
    public:
        explicit InputManager() = default;
        ~InputManager()         = default;

        InputManager(const InputManager &)              = delete;
        InputManager &operator=(const InputManager &)   = delete;

        void install(GLFWwindow *window);

        InputState snapshot();
    private:
        static void _key_callback(              GLFWwindow *w, int key, int scancode,   int action, int mods);
        static void _mouse_button_callback(     GLFWwindow *w, int button,              int action, int mods);
        static void _cursor_pos_callback(       GLFWwindow *w, double x, double y);

        /** Yes, i am aware. But sometimes you have to do what you have to do. */
        std::atomic<bool> _shift_held{false};

        std::atomic<bool> _cam_move_fwd{false};
        std::atomic<bool> _cam_move_back{false};
        std::atomic<bool> _cam_move_left{false};
        std::atomic<bool> _cam_move_right{false};
        std::atomic<bool> _cam_move_up{false};
        std::atomic<bool> _cam_move_down{false};

        std::atomic<bool> _obj_move_fwd{false};
        std::atomic<bool> _obj_move_back{false};
        std::atomic<bool> _obj_move_left{false};
        std::atomic<bool> _obj_move_right{false};
        std::atomic<bool> _obj_move_up{false};
        std::atomic<bool> _obj_move_down{false};

        std::atomic<bool> _obj_rot_pitch_add{false};
        std::atomic<bool> _obj_rot_pitch_sub{false};
        std::atomic<bool> _obj_rot_yaw_sub{false};
        std::atomic<bool> _obj_rot_yaw_add{false};
        std::atomic<bool> _obj_rot_roll_add{false};
        std::atomic<bool> _obj_rot_roll_sub{false};

        std::atomic<bool> _select_next_pending{false};
        std::atomic<bool> _toggle_info_pending{false};
        std::atomic<bool> _toggle_texture_pending{false};
        std::atomic<bool> _toggle_grid_pending{false};
        std::atomic<bool> _reset_selected_pending{false};
        std::atomic<bool> _toggle_vsync_pending{false};
        std::atomic<bool> _toggle_hide_others_pending{false};
        std::atomic<bool> _click_pending{false};

        std::atomic<bool>  _dragging{false};
        std::atomic<float> _mouse_dx{0};
        std::atomic<float> _mouse_dy{0};
        std::atomic<float> _last_mouse_x{0};
        std::atomic<float> _last_mouse_y{0};
        std::atomic<float> _cur_mouse_x{0};
        std::atomic<float> _cur_mouse_y{0};
};

#endif /* SCOP_INPUT_MANAGER_HPP */
