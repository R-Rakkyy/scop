#ifndef SCOP_ENGINE_ENGINE_H
#define SCOP_ENGINE_ENGINE_H

#include "glfw/window.h"
#include "engine/input/input_manager.hpp"
#include "engine/scene/mesh_entry.hpp"
#include "engine/scene/scene.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/ui/ui_overlay.hpp"
#include "utils/obj/obj_loader.hpp"
#include "time/chronometer.hpp"
#include "time/scheduler.hpp"

#include <vector>

/**
 * @class Engine
 * @brief The Engine class is responsible for managing the rendering loop, input handling, and scene updates.
 */
class Engine {
    public:
        explicit Engine(GLFWwindow *window, std::vector<ObjMesh> &meshes);
        ~Engine();
        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;

        void start();
        void stop();

    private:
        void                            _tick();            /* main tick */
        float                           _consume_dt();
        std::vector<RenderInstance>     _build_instances(const ftpp::Matrix &vp) const;
        s_UiFrameInputs                 _build_ui_inputs() const;

        InputManager                    _input;
        SceneManager                    _scene;
        Renderer                        _renderer;
        UiOverlay                       _ui;

        std::size_t                     _total_vertices = 0;
        std::size_t                     _total_materials = 0;

        ftpp::Chronometer               _chrono;
        ftpp::Scheduler                 _scheduler;
};

#endif /* SCOP_ENGINE_ENGINE_H */
