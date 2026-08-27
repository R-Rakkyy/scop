#include "iostream/logger.hpp"
#include "iostream/thread_safe_iostream.hpp"
#include "design_patterns/result.hpp"
#include "config.h"

#include "app/application.hpp"
#include "engine/engine.hpp"

#include <malloc.h>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
    /* ARGUMENTS */
    std::vector<std::string> paths;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) paths.emplace_back(argv[i]);
    } else {
        paths.emplace_back("./resources/mandatory/42.obj");
    }

    /* LOGGER */
    ftpp::threadSafeCout.setPrefix("[MAIN] ");
    ftpp::Singleton<ftpp::Logger>::instantiate(APP_LOG_LEVEL);
    ftpp::Logger    *logger = ftpp::Singleton<ftpp::Logger>::instance();

    /* APP */
    Application     app(logger);                                        /* App loop runs on the main thread */
    if (!app.init())
        return 1;
    window_poll_events();                                               /* Mark window responsive on some provider */

    ftpp::Result<std::vector<ObjMesh>> meshes_result = app.load_obj_paths(paths);
    if (!meshes_result) return 1;

    {   // Engine scope

        Engine engine(app.get_window(), meshes_result.value());         /* Engine runs on possibly separated threads */
        meshes_result.value().clear();                                  /* uploaded to GPU already, drop the CPU copy */
        meshes_result.value().shrink_to_fit();
        malloc_trim(0);
        engine.start();

        app.run();                                                      /* Main application loop, stops when the window is closed */
    
    }   // ensure Engine destructor runs before we destroy the window & app.

    app.cleanup();
    return 0;
}
