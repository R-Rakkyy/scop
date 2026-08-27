#ifndef SCOP_SCENE_HPP
#define SCOP_SCENE_HPP

#include "config.h"
#include "engine/input/input_manager.hpp"
#include "engine/scene/mesh_entry.hpp"
#include "mathematics/matrix.hpp"

#include <cstddef>
#include <string>
#include <vector>

class SceneManager {
    public:
        void add_mesh(int renderer_index, const std::string &name, const float aabb_min[3], const float aabb_max[3]);
        void update(const InputState &in, float dt);

        ftpp::Matrix view_matrix() const;
        ftpp::Matrix projection_matrix(float aspect) const;

        std::size_t count() const;
        std::size_t selected() const;
        const std::vector<MeshEntry> &meshes() const noexcept;
        bool hide_others() const noexcept; /** true: only the selected mesh should be drawn (H toggles this) */

        /* World-space camera position, for the
         * fragment shader's specular (Blinn-Phong) view direction.
         * > See Engine::_tick
         * */
        const float *cam_pos() const;

    private:
        void                            _update_camera(const InputState &in, float dt);
        std::vector<MeshTransform *>    _select_targets(bool shift_held);
        void                            _move_targets(const InputState &in, float dt, const std::vector<MeshTransform *> &targets) const;
        void                            _pivot_position(float out[3]) const;
        void                            _rotate_targets_arrow(const InputState &in, float dt, const std::vector<MeshTransform *> &targets, const float pivot_pos[3]) const;
        void                            _rotate_targets_mouse(const InputState &in, const std::vector<MeshTransform *> &targets, const float pivot_pos[3]) const;

        std::vector<MeshEntry>      _meshes;                                /** List of all loaded meshes, each with its renderer handle, name, and transform */
        std::size_t                 _selected       = 0;                    /** Index of the currently selected mesh */
        bool                        _hide_others    = false;                /** true: only the selected mesh should be drawn (H toggles this) */
        float                       _cam_pos[3]     = {0.0f, 0.0f, 6.0f};   /** World-space camera position */
        float                       _near_plane     = 0.1f;                 /** Near clipping plane, updated to fit the largest loaded mesh (see add_mesh()) */
        float                       _far_plane      = 1000.0f;              /** Far clipping plane, updated to fit the largest loaded mesh (see add_mesh()) */
        float                       _move_speed     = SCENE_MOVE_SPEED;     /** Camera WASD speed (units/sec), scaled to fit the largest loaded mesh (see add_mesh()) */
        float                       _max_radius     = 0.0f;                 /** Largest mesh "radius" (max abs AABB coordinate) seen so far, drives cam_pos/far_plane/move_speed */
};

#endif /* SCOP_SCENE_HPP */
