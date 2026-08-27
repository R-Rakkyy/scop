#ifndef SCOP_UTILS_OBJ_NORMAL_SMOOTHING_HPP
#define SCOP_UTILS_OBJ_NORMAL_SMOOTHING_HPP

#include "utils/obj/obj_parser_types.hpp"

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

struct NormalAccum {
    float   sum[3]  = {0.0f, 0.0f, 0.0f};
    int     count   = 0;
};
/* In the map, NormalAccum are keyed by (vertex index, smoothing group key) pairs. */
using NormalAccumMap = std::map<std::pair<int64_t, int64_t>, NormalAccum>;

/* Accumulates one flat per-triangle normal into every vertex of `faces`
 * that has no explicit vn.
 *
 * Note: faces who already carry a vn never enter the accumulator. */
void accumulate_face_normals(const std::vector<PendingFace> &faces, const std::vector<std::array<float, 3>> &positions, NormalAccumMap &accum);

void normalize_accumulated_normals(NormalAccumMap &accum);

/* Resolves the final normal for one face-vertex reference:
 * - its explicit vn if present
 * - otherwise the averaged (or flat, for ungrouped faces) result from `accum`. */
void resolve_vertex_normal(const FaceVertRef &ref, const PendingFace &face, const ParserState &state,
                            const NormalAccumMap &accum, float out[3]);

#endif /* SCOP_UTILS_OBJ_NORMAL_SMOOTHING_HPP */
