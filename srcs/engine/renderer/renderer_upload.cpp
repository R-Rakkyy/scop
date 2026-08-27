#include "engine/renderer/renderer.hpp"
#include "utils/obj/obj_material.hpp"
#include "iostream/logger.hpp"

#include "engine/vk/resources/mesh.h"
#include "engine/vk/resources/descriptors.h"
#include "engine/vk/resources/texture.h"

#include <algorithm>
#include <vector>

namespace {

int upload_vertices(const std::vector<ObjVertex> &sorted)
{
    std::vector<s_VkMeshVertexIn> vk_verts;
    vk_verts.reserve(sorted.size());

    for (const ObjVertex &v : sorted) {
        s_VkMeshVertexIn  out{};

        out.pos[0]      =       v.pos[0];           
        out.pos[1]      =       v.pos[1];           
        out.pos[2]      =       v.pos[2];
        out.normal[0]   =       v.normal[0];        
        out.normal[1]   =       v.normal[1];        
        out.normal[2]   =       v.normal[2];
        out.uv[0]       =       v.uv[0];            
        out.uv[1]       =       v.uv[1];
        out.face_id     =       static_cast<float>(v.face_id);
        vk_verts.push_back(out);
    }
    return vk_mesh_upload_from_data(vk_verts.data(), static_cast<uint32_t>(vk_verts.size()));
}

int texture_for(ObjTexture &tex, int fallback_handle)
{
    if (!tex.decoded || !tex.rgba)      return fallback_handle;

    int handle = vk_texture_create(tex.rgba.get(), tex.width, tex.height);


    tex.rgba.reset();
    return handle >= 0 ? handle : fallback_handle;
}

} // namespace

int Renderer::upload_mesh(ObjMesh &mesh)
{
    if (mesh.vertices.empty()) return -1;

    /* Group vertices by material_id (stable so order is kept)
     * Sorted in place: mesh.vertices is otherwise unused after upload. */
    std::vector<ObjVertex> &sorted  =   mesh.vertices;
    std::stable_sort(sorted.begin(), sorted.end(), [](const ObjVertex &a, const ObjVertex &b) { return a.material_id < b.material_id; });

    int mesh_handle                 =   upload_vertices(sorted);
    if (mesh_handle < 0) return -1;

    MeshGpu gpu{};
    gpu.mesh_handle                 =   mesh_handle;

    size_t i = 0;
    while (i < sorted.size()) {
        // Group consecutive vertices with the same material_id into a MaterialRun
        int material_id     = sorted[i].material_id;
        size_t start        = i;
        while (i < sorted.size() && sorted[i].material_id == material_id) ++i;
        uint32_t count = static_cast<uint32_t>(i - start);
        int diffuse         = _base_texture;
        int bump            = _flat_bump_texture;

        /* std140-safe as two vec4s: {Kd.xyz, pad}, {Ks.xyz, Ns}.
         * matches shaders/glsl/frag.glsl's Material block:
         *    material_ubo[8] = {kd.x, kd.y, kd.z,  0.0f,     ks.x, ks.y, ks.z,  ns}; */
        float material_ubo[8] = {1.0f, 1.0f, 1.0f,  0.0f,     0.5f, 0.5f, 0.5f,  32.0f};
        if (material_id >= 0 && static_cast<size_t>(material_id) < mesh.materials.size()) {
            ObjMaterial &mat    = mesh.materials[static_cast<size_t>(material_id)];
            diffuse             = texture_for(mat.map_Kd,       _base_texture);
            bump                = texture_for(mat.map_bump,     _flat_bump_texture);
            material_ubo[0]     = mat.Kd[0];    material_ubo[1] = mat.Kd[1];    material_ubo[2] = mat.Kd[2];
            material_ubo[4]     = mat.Ks[0];    material_ubo[5] = mat.Ks[1];    material_ubo[6] = mat.Ks[2];
            material_ubo[7]     = mat.Ns;
        }

        int descriptor = vk_descriptors_create_for_textures(diffuse, bump, material_ubo);
        if (descriptor < 0) {
            ftpp::log::error("Renderer::upload_mesh: failed to create descriptor set for material ", material_id);
            continue;
        }
        gpu.runs.push_back(MaterialRun{static_cast<uint32_t>(start), count, descriptor});
    }

    int mesh_index = static_cast<int>(_meshes.size());
    _meshes.push_back(std::move(gpu));
    return mesh_index;
}
