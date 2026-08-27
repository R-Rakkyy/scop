#ifndef SCOP_RENDERER_HPP
#define SCOP_RENDERER_HPP

#include "glfw/window.h"
#include "utils/obj/obj_loader.hpp"
#include "mathematics/matrix.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

/** One mesh instance to draw this frame. */
struct RenderInstance {
    int             mesh_index;

    ftpp::Matrix    mvp;
    ftpp::Matrix    model; /* model-only transform, keep it for lighting normals */
};

/**
 * @class Renderer
 * @brief Owns the Vulkan context and the per-material texture/descriptor registries.
 * Uploads ObjMesh geometry (grouped into per-material draw
 * ranges) and draws a list of mesh instances each frame.
 */
class Renderer {
    public:
        explicit Renderer(GLFWwindow *window);
        ~Renderer();
        Renderer(Renderer &&) = delete;
        Renderer &operator=(Renderer &&) = delete;

        Renderer(const Renderer &) = delete;
        Renderer &operator=(const Renderer &) = delete;

        /* Returns a mesh index (>= 0) to pass in RenderInstance::mesh_index, or -1 on failure. */
        int   upload_mesh(ObjMesh &mesh);

        /**
         * @brief Draws a frame with the given view-projection matrix, mesh instances, and UI vertex buffer.
         * @param view_proj The view-projection matrix to use for rendering.
         * @param instances The list of mesh instances to draw.
         * @param ui_vertices The vertex buffer for the UI overlay.
         * @param texture_mix The mix factor for textures (0.0 to 1.0).
         * @param show_grid Whether to show the grid in the scene.
         * @param cam_pos The camera position in world space.
         */
        void  draw_frame(const ftpp::Matrix &view_proj, const std::vector<RenderInstance> &instances,
                        const std::vector<float> &ui_vertices, float texture_mix, bool show_grid,
                        const float cam_pos[3]);
    
        float aspect_ratio() const;

        bool  is_ready() const;             /** @brief Returns true if the renderer is ready to draw frames (swapchain and pipelines are valid). */
        void  try_recover();                /** @brief Attempts to recover the renderer if it is not ready (e.g., after a swapchain recreation). */
        void  toggle_vsync();               /** @brief Toggles VSync on or off. */

        /** @brief Verbose, cached-once Vulkan/device setup summary for the F3 panel.
         *  @return GPU name, driver/API version, chosen extensions, swapchain/present
         *  settings, frames-in-flight, etc. */
        const std::vector<std::string> &device_info_lines() const;

    private:
        /** @brief Represents a range of vertices that use the same material. */
        struct MaterialRun {
            uint32_t    first; /* first vertex index of this material's draw range */
            uint32_t    count; /* number of vertices in this material's draw range */
            int         descriptor_handle;
        };

        /** @brief Represents a mesh that has been uploaded to the GPU. */
        struct MeshGpu {
            int                         mesh_handle;
            std::vector<MaterialRun>    runs;
        };

        bool _init_core(int &out_graphics_family, int &out_present_family);
        bool _init_fallback_textures();
        bool _init_ui_resources();
        bool _init_shaders_and_pipelines(int graphics_family);

        GLFWwindow                  *_window;                               /* The GLFW window associated with this renderer. */
        std::vector<MeshGpu>        _meshes;                                /* The list of meshes that have been uploaded to the GPU (handles-only). */
        int                         _base_texture       = -1;               /* fallback texture handle for missing diffuse maps */
        int                         _flat_bump_texture  = -1;               /* flat bump texture handle */


        static constexpr uint32_t   UI_VERTEX_CAPACITY  = 16384;            /* fixed cap, add growth if the overlay ever needs more */
        int                         _font_texture       = -1;               /* font atlas texture handle */
        int                         _font_descriptor    = -1;               /* font atlas descriptor set handle */
        VkBuffer                    _ui_vbuf            = VK_NULL_HANDLE;   /* vertex buffer for the UI overlay */
        VkDeviceMemory              _ui_vmem            = VK_NULL_HANDLE;   /* device memory for the UI vertex buffer */
        void                        *_ui_mapped         = nullptr;          /* mapped pointer to the UI vertex buffer memory */
        std::vector<std::string>    _device_info_lines;
        void                        _build_device_info_lines(int graphics_family, int present_family);
};

#endif /* SCOP_RENDERER_HPP */
