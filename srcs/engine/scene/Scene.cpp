#include "engine/scene/scene.hpp"
#include "config.h"
#include "utils/math/matrix_utils.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

void orbit_position(MeshTransform &t, const float pivot_pos[3], const ftpp::Matrix &d_world)
{
    ftpp::Vector local({ // local-space position of the mesh relative to the pivot
        t.center[0] + t.tx - pivot_pos[0],
        t.center[1] + t.ty - pivot_pos[1],
        t.center[2] + t.tz - pivot_pos[2],
        0,
    });

    ftpp::Vector v  = d_world.multiply(local);
    t.tx            = pivot_pos[0] + (static_cast<float>(v.at(0))) - t.center[0];
    t.ty            = pivot_pos[1] + (static_cast<float>(v.at(1))) - t.center[1];
    t.tz            = pivot_pos[2] + (static_cast<float>(v.at(2))) - t.center[2];
}

} // namespace

void SceneManager::add_mesh(int renderer_index, const std::string &name, const float aabb_min[3], const float aabb_max[3])
{
    MeshEntry entry{renderer_index, name, MeshTransform{}};
    for (int i = 0; i < 3; ++i)     entry.transform.center[i] = (aabb_min[i] + aabb_max[i]) * 0.5f;
    _meshes.push_back(entry);

    /* Change the speed and pos of the camera based on the largest mesh */
    float radius = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float extent = std::max(std::fabs(aabb_min[i]), std::fabs(aabb_max[i]));
        radius = std::max(radius, extent);
    }
    _max_radius     = std::max(_max_radius, radius);
    _near_plane     = std::max(_max_radius * static_cast<float>(SCENE_NEAR_PLANE_FACTOR), static_cast<float>(SCENE_NEAR_PLANE_MIN));
    _cam_pos[2]     = std::max(_max_radius * 2.2f, _near_plane * 4.0f);
    _move_speed     = _cam_pos[2] * (SCENE_MOVE_SPEED / 6.0f);
    _far_plane      = std::max((_cam_pos[2] + _max_radius) * 3.0f, 1000.0f);
}

void SceneManager::update(const InputState &in, float dt)
{
    if (in.select_next && !_meshes.empty())     _selected = (_selected + 1) % _meshes.size();
    if (in.toggle_hide_others)                  _hide_others = !_hide_others;

    this->_update_camera(in, dt);

    if (_meshes.empty()) return; // leave if no meshes to manipulate (shouldn't happen)
    std::vector<MeshTransform *> targets = this->_select_targets(in.shift_held);

    if (in.reset_selected) {
        for (MeshTransform *t : targets) {
            float center[3] = {t->center[0], t->center[1], t->center[2]};   // keep center
            *t = MeshTransform{};                                           // reset by recreating in case we add new fields to MeshTransform.
            for (int i = 0; i < 3; ++i) t->center[i] = center[i];           // keep center
        }
        return;
    }

    this->_move_targets(in, dt, targets);

    float pivot_pos[3];
    this->_pivot_position(pivot_pos);

    this->_rotate_targets_arrow(in, dt, targets, pivot_pos);
    this->_rotate_targets_mouse(in, targets, pivot_pos);
}

void SceneManager::_update_camera(const InputState &in, float dt)
{
    /* Camera: WASD (+Space/Ctrl) */
    float cam_right = (in.cam_move_right ? 1.0f : 0.0f) - (in.cam_move_left ? 1.0f : 0.0f);
    float cam_fwd   = (in.cam_move_fwd   ? 1.0f : 0.0f) - (in.cam_move_back ? 1.0f : 0.0f);
    float cam_up    = (in.cam_move_up    ? 1.0f : 0.0f) - (in.cam_move_down ? 1.0f : 0.0f);
    _cam_pos[0]     += cam_right * _move_speed * dt; // x
    _cam_pos[1]     += cam_up    * _move_speed * dt; // y
    _cam_pos[2]     -= cam_fwd   * _move_speed * dt; /* z: -Z is "into the scene" */
}

std::vector<MeshTransform *> SceneManager::_select_targets(bool shift_held)
{
    // If the user is holding shift, we apply the transform to all meshes at once.
    std::vector<MeshTransform *> targets;
    
    if (shift_held) {
        for (MeshEntry &entry : _meshes) targets.push_back(&entry.transform);
    } else {
        targets.push_back(&_meshes[_selected].transform);
    }

    return targets;
}

void SceneManager::_move_targets(const InputState &in, float dt, const std::vector<MeshTransform *> &targets) const
{
    float obj_right = (in.obj_move_right ? 1.0f : 0.0f) - (in.obj_move_left ? 1.0f : 0.0f);
    float obj_fwd   = (in.obj_move_fwd   ? 1.0f : 0.0f) - (in.obj_move_back ? 1.0f : 0.0f);
    float obj_up    = (in.obj_move_up    ? 1.0f : 0.0f) - (in.obj_move_down ? 1.0f : 0.0f);

    for (MeshTransform *t : targets) {
        t->tx       += obj_right * SCENE_MOVE_SPEED * dt;
        t->tz       += obj_fwd   * SCENE_MOVE_SPEED * dt;
        t->ty       += obj_up    * SCENE_MOVE_SPEED * dt;
    }
}

void SceneManager::_pivot_position(float out[3]) const
{
    /* Shared pivot: the selected mesh's own world position. */
    const MeshTransform &pivot  =   _meshes[_selected].transform;
    out[0]                      =   pivot.center[0] + pivot.tx;
    out[1]                      =   pivot.center[1] + pivot.ty;
    out[2]                      =   pivot.center[2] + pivot.tz;
}

/**
 * @note The rotation used to be easier to understand (t->rot_x/y/z = rot_button_x/y/z * speed * dt).
 *       I however needed the math to pass through a Matrix delta so the object could rotate around a pivot.
 *       This specific case only happens when the user is holding shift.
 *       In a normal case, orbit_position is a no-op onto 'MeshTransform &t' which just rotate around its own center.
 */
void SceneManager::_rotate_targets_arrow(const InputState &in, float dt, const std::vector<MeshTransform *> &targets, const float pivot_pos[3]) const
{
    int pitch_dir       = (in.obj_rot_pitch_add ? 1 : 0) - (in.obj_rot_pitch_sub ? 1 : 0);
    int yaw_dir         = (in.obj_rot_yaw_add   ? 1 : 0) - (in.obj_rot_yaw_sub   ? 1 : 0);
    int roll_dir        = (in.obj_rot_roll_add  ? 1 : 0) - (in.obj_rot_roll_sub  ? 1 : 0);

    if (pitch_dir == 0 && yaw_dir == 0 && roll_dir == 0) return;

    float rot_pitch     = static_cast<float>(pitch_dir);
    float rot_yaw       = static_cast<float>(yaw_dir);
    float rot_roll      = static_cast<float>(roll_dir);

    for (MeshTransform *t : targets) {
        ftpp::Matrix base_old       = rotate_z(t->rot_z) * rotate_y(t->rot_y) * rotate_x(t->rot_x);
        t->rot_x                    += rot_pitch * SCENE_ARROW_ROT_SPEED * dt;
        t->rot_y                    += rot_yaw   * SCENE_ARROW_ROT_SPEED * dt;
        t->rot_z                    += rot_roll  * SCENE_ARROW_ROT_SPEED * dt;
        ftpp::Matrix base_new       = rotate_z(t->rot_z) * rotate_y(t->rot_y) * rotate_x(t->rot_x);
        
        /* A delta is base_new * base_old⁻¹.
         * Since rotation matrices are orthonormal (R * Rᵀ = I),
         * the inverse of a rotation matrix is just its transpose
         */
        ftpp::Matrix local_delta    = base_new * base_old.transpose();
        ftpp::Matrix d_world        = t->rot * local_delta * t->rot.transpose();
        orbit_position(*t, pivot_pos, d_world);
    }
}

void SceneManager::_rotate_targets_mouse(const InputState &in, const std::vector<MeshTransform *> &targets, const float pivot_pos[3]) const
{
    if (in.mouse_dx == 0.0f && in.mouse_dy == 0.0f) return;

    /* compute new drag increment onto the LEFT of the current
     * rotation (world/view-space axes) from the accumulated mouse movement.
     *
     * Note: dx rotate around Y axis, dy rotate around X axis */
    ftpp::Matrix delta  = rotate_y(-in.mouse_dx * SCENE_MOUSE_ROT_SPEED) * rotate_x(in.mouse_dy * SCENE_MOUSE_ROT_SPEED);

    for (MeshTransform *t : targets) {
        t->rot          = delta * t->rot;
        orbit_position(*t, pivot_pos, delta);
    }
}

ftpp::Matrix SceneManager::view_matrix() const
{
    return translate_matrix(-_cam_pos[0], -_cam_pos[1], -_cam_pos[2]);
}

/**
 * This is a different version from ftpp::Matrix::projection() because Vulkan and OpenGL
 * Have different implementation:
 * 
 *      - Row 2 is negated: Vulkan's clip space has +Y pointing down (unlike
 *      OpenGL's convention this formula was originally written for), so
 *      without the flip every frame rendered upside down.
 *
 *      - Row 3 also differs from the textbook OpenGL formula: OpenGL maps
 *      view-space z in [-near,-far] to NDC z in [-1,1], but Vulkan's depth
 *      range is [0,1]. 
 * 
 * Maths references:
 *      - https://arxiv.org/pdf/2105.09476
 *      - https://gamemath.com/book/matrixmore.html#perspective_projection
 *      - https://johannesugb.github.io/gpu-programming/setting-up-a-proper-vulkan-projection-matrix/
 *      - https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
 * */
ftpp::Matrix SceneManager::projection_matrix(float aspect) const
{
    double factor       = 1.0 / std::tan(SCENE_FOV_RADIANS * 0.5);
    double near         = _near_plane;
    double far          = _far_plane;
    double aspect_d     = static_cast<double>(aspect);

    return ftpp::Matrix({
ftpp::Vector(   {factor / aspect_d, 0      , 0                 , 0                            }),
ftpp::Vector(   {0              , -factor, 0                 , 0                            }),
ftpp::Vector(   {0              , 0      , far / (near - far), (far * near) / (near - far)  }),
ftpp::Vector(   {0              , 0      , -1                , 0                            }),
    });
}
const std::vector<MeshEntry> &SceneManager::meshes() const noexcept     { return _meshes;               }
std::size_t SceneManager::count()       const                           { return _meshes.size();        }
std::size_t SceneManager::selected()    const                           { return _selected;             }
bool        SceneManager::hide_others() const noexcept                  { return _hide_others;          }
const float *SceneManager::cam_pos()    const                           { return _cam_pos;              }
