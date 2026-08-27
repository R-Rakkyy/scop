#include "app/application.hpp"
#include "glfw/window.h"
#include "config.h"
#include "utils/obj/obj_loader.hpp"

volatile std::sig_atomic_t Application::s_quit_requested = 0;

void Application::_handle_quit_signal(int /*signum*/) {
    s_quit_requested = 1;
}

Application::Application(ftpp::Logger *logger): _window(nullptr), _logger(logger), _inited(false) {}

Application::~Application() {   cleanup();  }

bool Application::init() {
    if (_inited) return true;

    if (!window_init(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, WINDOW_DEFAULT_TITLE, &_window)) {
        ftpp::log::error("Application: window_init failed");
        return false;
    }

    struct sigaction sa = {};
    sa.sa_handler = _handle_quit_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    ftpp::log::debug("Application: window created successfully");
    ftpp::log::info("Application: initialized");

    _inited = true;
    return true;
}

ftpp::Result<std::vector<ObjMesh>> Application::load_obj_paths(const std::vector<std::string> &paths) const noexcept {
    using MeshResult = ftpp::Result<std::vector<ObjMesh>>;
    std::vector<ObjMesh> meshes;

    for (const std::string &path : paths) {
        for (const std::string &obj_path : list_obj_paths(path)) {
            MeshResult result = this->load_obj_file(obj_path.c_str());
            if (!result) {
                ftpp::log::error("An error occured loading '", obj_path, "': ", result.errorMessage());
                return result;
            }
            for (ObjMesh &mesh : result.value()) meshes.push_back(std::move(mesh));
        }
    }

    ftpp::log::info("Application: loaded ", meshes.size(), " mesh object(s) total");
    return MeshResult::success(std::move(meshes));
}

ftpp::Result<std::vector<ObjMesh>> Application::load_obj_file(const char *path) const noexcept {
    using MeshResult = ftpp::Result<std::vector<ObjMesh>>;
    if (!path) return MeshResult::failure(std::invalid_argument("Application: load_obj_file called with null path"));

    // std::error_code error;
    // uintmax_t       size = std::filesystem::file_size(path, error);
    // if (error) return MeshResult::failure(std::runtime_error("Application: cannot stat " + std::string(path) + ": " + error.message()));

    ObjFile file;
    if (!load_obj(path, file))
        return MeshResult::failure(std::runtime_error("Application: load_obj_file failed for " + std::string(path)));

    if (file.objects.empty())
        return MeshResult::failure(std::runtime_error("Application: no mesh objects found in " + std::string(path)));

    ftpp::log::debug("Application: loaded ", file.objects.size(), " object(s) from file '", path, "'");
    return MeshResult::success(std::move(file.objects));
}

ftpp::Logger& Application::Logger() {
    return *_logger;
}

GLFWwindow* Application::get_window() const {
    return _window;
}

void Application::run() {
    
    /* Main application loop */
    /* Since scop is about honestly only rendering things, we do not really have any logic to update.
     * The only candidate would be inputs, but passing inputs informations to the engine would require
     * to work a lot around thread-safety. My other thought is:
     *  - I wouldn't want my logic that updates my game state (e.g. physics, AI, etc.) to also work on inputs (unrelated imo).
    */
    while (!window_should_close() && !s_quit_requested) {

        window_poll_events();

    }

    ftpp::log::debug("Application: window close requested.");
}

void Application::cleanup()
{
    if (!_inited) return;
    ftpp::log::info("Application: shutting down");
    window_destroy();
    _inited = false;
}
