#include "utils/obj/obj_normal_smoothing.hpp"
#include "utils/obj/obj_face_parser.hpp"

#include <cmath>

namespace {

int64_t group_key_for(const PendingFace &face)
{
    if (face.smoothing_group != 0) 
        return face.smoothing_group;

    return -(1 + static_cast<int64_t>(face.face_id));           /* unique per ungrouped face: no averaging */
}

void flat_normal(const std::array<float, 3> &a, const std::array<float, 3> &b, const std::array<float, 3> &c, float out[3])
{
    float ux = b[0] - a[0];
    float uy = b[1] - a[1];
    float uz = b[2] - a[2];

    float vx = c[0] - a[0];
    float vy = c[1] - a[1];
    float vz = c[2] - a[2];
    
    out[0] = uy * vz - uz * vy;
    out[1] = uz * vx - ux * vz;
    out[2] = ux * vy - uy * vx;
}

} // namespace






void accumulate_face_normals(const std::vector<PendingFace> &faces, const std::vector<std::array<float, 3>> &positions, NormalAccumMap &accum)
{
    for (const PendingFace &face : faces) {
        int64_t group_key = group_key_for(face);

        for_each_triangle_in_face(face.refs, [&](const FaceVertRef &a, const FaceVertRef &b, const FaceVertRef &c) {
            // compute the flat normal
            float n[3];
            flat_normal(positions[static_cast<size_t>(a.v)], positions[static_cast<size_t>(b.v)], positions[static_cast<size_t>(c.v)], n);
            
            // accumulate the normal for each vertex of the triangle
            for (const FaceVertRef *r : {&a, &b, &c}) {
                if (r->vn >= 0) continue; // no explicit

                NormalAccum &acc    = accum[{r->v, group_key}]; // get the normal accumulator for this vertex and smoothing group from the map
                acc.sum[0]          += n[0];
                acc.sum[1]          += n[1];
                acc.sum[2]          += n[2];
                acc.count++;
            }
        });
    }
}

void normalize_accumulated_normals(NormalAccumMap &accum)
{
    for (auto &[key, acc] : accum) {
        float len = std::sqrt(acc.sum[0] * acc.sum[0] + acc.sum[1] * acc.sum[1] + acc.sum[2] * acc.sum[2]);
        if (len < 1e-9f) continue; // avoid division by zero for degenerate normals
        
        acc.sum[0] /= len;
        acc.sum[1] /= len;
        acc.sum[2] /= len;
    }
}

void resolve_vertex_normal(const FaceVertRef &ref_vert, const PendingFace &face, const ParserState &state, const NormalAccumMap &accum, float out[3])
{
    if (ref_vert.vn >= 0 && static_cast<size_t>(ref_vert.vn) < state.normals.size()) {
        // vertex has an explicit normal
        const auto &n = state.normals[static_cast<size_t>(ref_vert.vn)];
        out[0] = n[0];
        out[1] = n[1]; 
        out[2] = n[2];
        return;
    }

    // vertex does not have an explicit normal, look up the accumulated normal
    auto it = accum.find({ref_vert.v, group_key_for(face)});
    if (it == accum.end()) { out[0] = out[1] = out[2] = 0.0f; return; } // no accumulated normal found, return zero vector

    out[0] = it->second.sum[0];
    out[1] = it->second.sum[1]; 
    out[2] = it->second.sum[2];
}
