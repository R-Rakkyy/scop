#include "utils/obj/obj_finalize.hpp"
#include "utils/obj/obj_face_parser.hpp"
#include "utils/obj/obj_normal_smoothing.hpp"

#include <cmath>
#include <unordered_map>
#include <utility>

namespace {

int resolve_material_index(const std::string &name, const ParserState &state, ObjMesh &mesh, std::unordered_map<std::string, int> &name_to_index)
{
    if (name.empty()) return -1;
    if (auto it = name_to_index.find(name); it != name_to_index.end()) return it->second;

    auto mat_it = state.available_materials.find(name);
    if (mat_it == state.available_materials.end()) return -1;

    int index = static_cast<int>(mesh.materials.size());
    mesh.materials.push_back(mat_it->second);
    name_to_index.emplace(name, index);
    return index;
}

void build_object_vertices(const ParserState &state, ObjMesh &mesh)
{
    NormalAccumMap  accum;
    accumulate_face_normals(state.current_faces, state.positions, accum);
    normalize_accumulated_normals(accum);

    size_t vertex_count         = 0;
    for (const PendingFace &face : state.current_faces) {
        if (face.refs.size() >= 3)
            vertex_count += (face.refs.size() - 2) * 3;
    }
    mesh.vertices.reserve(vertex_count);

    std::unordered_map<std::string, int> name_to_index;
    for (const PendingFace &face : state.current_faces) {
        int material_id = resolve_material_index(face.material_name, state, mesh, name_to_index);

        for_each_triangle_in_face(face.refs, [&](const FaceVertRef &a, const FaceVertRef &b, const FaceVertRef &c) {
            for (const FaceVertRef *r : {&a, &b, &c}) {
                float n[3];
                resolve_vertex_normal(*r, face, state, accum, n);
                mesh.vertices.push_back(build_triangle_vertex(*r, state, n, material_id, face.face_id));
            }
        });
    }
}

void compute_aabb(ObjMesh &mesh)
{
    bool have_extent = false;
    auto extend = [&](const float p[3]) {
        if (!have_extent) {
            mesh.aabb_min[0] = mesh.aabb_max[0] = p[0];
            mesh.aabb_min[1] = mesh.aabb_max[1] = p[1];
            mesh.aabb_min[2] = mesh.aabb_max[2] = p[2];
            have_extent = true;
            return;
        }
        for (int i = 0; i < 3; ++i) {
            if (p[i] < mesh.aabb_min[i]) mesh.aabb_min[i] = p[i];
            if (p[i] > mesh.aabb_max[i]) mesh.aabb_max[i] = p[i];
        }
    };
    for (const ObjVertex &v : mesh.vertices) 
        extend(v.pos);
    for (const ObjPolyline &l : mesh.lines) 
        for (const auto &p : l.points) 
            extend(p.data());
    for (const auto &p : mesh.points)
        extend(p.data());
}

/* Box/triplanar projection: pick the plane most perpendicular to each vertex's normal */
void apply_uv_fallback(ObjMesh &mesh)
{
    float dx = mesh.aabb_max[0] - mesh.aabb_min[0] + 1e-6f;
    float dy = mesh.aabb_max[1] - mesh.aabb_min[1] + 1e-6f;
    float dz = mesh.aabb_max[2] - mesh.aabb_min[2] + 1e-6f;

    for (ObjVertex &v : mesh.vertices) {
        if (v.uv[0] >= -0.5f && v.uv[1] >= -0.5f) continue; // already has an explicit vt, skip fallback

        float ax = std::fabs(v.normal[0]);
        float ay = std::fabs(v.normal[1]);
        float az = std::fabs(v.normal[2]);

        // note: flip the v coordinate here:
        if (az >= ax && az >= ay) { // |nz| is largest, project onto XY plane
            v.uv[0] = (v.pos[0] - mesh.aabb_min[0]) / dx;
            v.uv[1] = 1.0f - (v.pos[1] - mesh.aabb_min[1]) / dy;
        } else if (ax >= ay) {      // |nx| is largest, project onto YZ plane
            v.uv[0] = (v.pos[2] - mesh.aabb_min[2]) / dz;
            v.uv[1] = 1.0f - (v.pos[1] - mesh.aabb_min[1]) / dy;
        } else {                    // |ny| is largest, project onto XZ plane
            v.uv[0] = (v.pos[0] - mesh.aabb_min[0]) / dx;
            v.uv[1] = 1.0f - (v.pos[2] - mesh.aabb_min[2]) / dz;
        }
    }
}

} // namespace

void finalize_object(ParserState &state)
{
    if (parser_state_object_is_empty(state)) return;

    ObjMesh mesh{};
    mesh.name           = std::move(state.name);
    mesh.lines          = std::move(state.current_lines);
    mesh.points         = std::move(state.current_points);
    
    build_object_vertices(state, mesh);
    compute_aabb(mesh);
    apply_uv_fallback(mesh);

    state.objects.push_back(std::move(mesh)); // store the finalized object in the parser state

    // clear
    state.current_faces.clear();
    state.current_lines.clear();
    state.current_points.clear();
    state.next_face_id = 0;
    state.name.clear();
}
