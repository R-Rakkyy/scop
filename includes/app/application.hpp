#ifndef SCOP_APP_HPP
#define SCOP_APP_HPP

#include "glfw/window.h"
#include "iostream/logger.hpp"
#include "utils/obj/obj_loader.hpp"
#include "design_patterns/result.hpp"

#include <csignal>
#include <vector>

class Application {
    public:
        explicit Application(ftpp::Logger *logger);
        ~Application();

        Application(const Application &) = delete;
        Application &operator=(const Application &) = delete;

        bool init();
        void run();         /** @brief Main loop, blocks main thread until exit */
        void cleanup();
        
        ftpp::Logger&   Logger();
        GLFWwindow*     get_window() const;

        [[nodiscard]] ftpp::Result<std::vector<ObjMesh>> load_obj_paths(const std::vector<std::string> &paths) const noexcept;
        [[nodiscard]] ftpp::Result<std::vector<ObjMesh>> load_obj_file(const char *path) const noexcept;

    private:
        static volatile std::sig_atomic_t  s_quit_requested;
        static void                        _handle_quit_signal(int signum);

        GLFWwindow     *_window;
        ftpp::Logger   *_logger;
        bool            _inited;
};

#endif /* SCOP_APP_HPP */
