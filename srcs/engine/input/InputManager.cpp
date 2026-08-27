#include "engine/input/input_manager.hpp"
#include "iostream/logger.hpp"

void InputManager::install(GLFWwindow *window)
{
    if (!window) return;

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, &InputManager::_key_callback);
    glfwSetCursorPosCallback(window, &InputManager::_cursor_pos_callback);
    glfwSetMouseButtonCallback(window, &InputManager::_mouse_button_callback);
}

InputState InputManager::snapshot()
{
    InputState s;

    s.shift_held        = _shift_held.load();

    s.cam_move_fwd      = _cam_move_fwd.load();
    s.cam_move_back     = _cam_move_back.load();
    s.cam_move_left     = _cam_move_left.load();
    s.cam_move_right    = _cam_move_right.load();
    s.cam_move_up       = _cam_move_up.load();
    s.cam_move_down     = _cam_move_down.load();

    s.obj_move_fwd      = _obj_move_fwd.load();
    s.obj_move_back     = _obj_move_back.load();
    s.obj_move_left     = _obj_move_left.load();
    s.obj_move_right    = _obj_move_right.load();
    s.obj_move_up       = _obj_move_up.load();
    s.obj_move_down     = _obj_move_down.load();

    s.obj_rot_pitch_add = _obj_rot_pitch_add.load();
    s.obj_rot_pitch_sub = _obj_rot_pitch_sub.load();
    s.obj_rot_yaw_sub   = _obj_rot_yaw_sub.load();
    s.obj_rot_yaw_add   = _obj_rot_yaw_add.load();
    s.obj_rot_roll_add  = _obj_rot_roll_add.load();
    s.obj_rot_roll_sub  = _obj_rot_roll_sub.load();

    s.select_next       = _select_next_pending.exchange(false);
    s.toggle_info       = _toggle_info_pending.exchange(false);
    s.toggle_texture    = _toggle_texture_pending.exchange(false);
    s.toggle_grid       = _toggle_grid_pending.exchange(false);
    s.reset_selected    = _reset_selected_pending.exchange(false);
    s.toggle_vsync      = _toggle_vsync_pending.exchange(false);
    s.toggle_hide_others = _toggle_hide_others_pending.exchange(false);
    s.mouse_click       = _click_pending.exchange(false);

    s.mouse_dx          = _mouse_dx.exchange(0.0f);
    s.mouse_dy          = _mouse_dy.exchange(0.0f);
    s.mouse_x           = _cur_mouse_x.load();
    s.mouse_y           = _cur_mouse_y.load();

    return s;
}

void InputManager::_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(w));
    if  (!self) { ftpp::log::warning("InputManager::_key_callback: window user pointer is null"); return; }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_TAB)    { self->_select_next_pending.store(true); return; }
        if (key == GLFW_KEY_F3)     { self->_toggle_info_pending.store(true); return; }
        if (key == GLFW_KEY_T)      { self->_toggle_texture_pending.store(true); return; }
        if (key == GLFW_KEY_F4)     { self->_toggle_grid_pending.store(true); return; }
        if (key == GLFW_KEY_R)      { self->_reset_selected_pending.store(true); return; }
        if (key == GLFW_KEY_F5)     { self->_toggle_vsync_pending.store(true); return; }
        if (key == GLFW_KEY_H)      { self->_toggle_hide_others_pending.store(true); return; }
        if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(w, GLFW_TRUE); return; }
    }
    if (action != GLFW_PRESS && action != GLFW_RELEASE) return; /* no OS key-repeat: held-state only */

    bool held = (action == GLFW_PRESS); // true if key is pressed, false if released
    switch (key) {
        /** Shift: let user select all objects at once */
        case GLFW_KEY_LEFT_SHIFT:   self->_shift_held.store(held);          break;

        /* WASD moves the camera. */
        case GLFW_KEY_W:            self->_cam_move_fwd.store(held);        break;
        case GLFW_KEY_S:            self->_cam_move_back.store(held);       break;
        case GLFW_KEY_A:            self->_cam_move_left.store(held);       break;
        case GLFW_KEY_D:            self->_cam_move_right.store(held);      break;
        case GLFW_KEY_SPACE:        self->_cam_move_up.store(held);         break;
        case GLFW_KEY_LEFT_CONTROL: self->_cam_move_down.store(held);       break;
        /* ARROW KEYS + RSPACE/Ctrl moves the selected object. */
        case GLFW_KEY_UP:           self->_obj_move_fwd.store(held);        break;
        case GLFW_KEY_DOWN:         self->_obj_move_back.store(held);       break;
        case GLFW_KEY_LEFT:         self->_obj_move_left.store(held);       break;
        case GLFW_KEY_RIGHT:        self->_obj_move_right.store(held);      break;
        case GLFW_KEY_RIGHT_CONTROL:self->_obj_move_down.store(held);       break;
        case GLFW_KEY_RIGHT_SHIFT:  self->_obj_move_up.store(held);         break;
        /* Numpad 7/9, 4/6, 1/3 rotates the selected object around its base (world) X/Y/Z axes. */
        case GLFW_KEY_KP_7:         self->_obj_rot_pitch_add.store(held);   break;
        case GLFW_KEY_KP_9:         self->_obj_rot_pitch_sub.store(held);   break;
        case GLFW_KEY_KP_4:         self->_obj_rot_yaw_sub.store(held);     break;
        case GLFW_KEY_KP_6:         self->_obj_rot_yaw_add.store(held);     break;
        case GLFW_KEY_KP_1:         self->_obj_rot_roll_sub.store(held);    break;
        case GLFW_KEY_KP_3:         self->_obj_rot_roll_add.store(held);    break;
        default: break;
    }
}

void InputManager::_cursor_pos_callback(GLFWwindow *w, double x, double y)
{
    auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(w));
    if (!self) { ftpp::log::warning("InputManager::_cursor_pos_callback: window user pointer is null"); return; }

    float fx        = static_cast<float>(x);
    float fy        = static_cast<float>(y);
    float last_x    = self->_last_mouse_x.exchange(fx);
    float last_y    = self->_last_mouse_y.exchange(fy);

    // If the left mouse button is held down, accumulate the cursor delta since the last snapshot.
    if (self->_dragging.load()) {
        self->_mouse_dx.fetch_add(fx - last_x);
        self->_mouse_dy.fetch_add(fy - last_y);
    }
    self->_cur_mouse_x.store(fx);
    self->_cur_mouse_y.store(fy);
}

void InputManager::_mouse_button_callback(GLFWwindow *w, int button, int action, int mods)
{
    (void)mods;
    auto *self = static_cast<InputManager *>(glfwGetWindowUserPointer(w));
    if (!self) { ftpp::log::warning("InputManager::_mouse_button_callback: window user pointer is null"); return; }
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS) {
        self->_click_pending.store(true);

        double x = 0, y = 0;
        glfwGetCursorPos(w, &x, &y);
        
        self->_last_mouse_x.store(static_cast<float>(x));
        self->_last_mouse_y.store(static_cast<float>(y));

        self->_dragging.store(true);
    } else if (action == GLFW_RELEASE) {
        self->_dragging.store(false);
    }
}
