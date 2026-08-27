#ifndef SCOP_UTILS_OBJ_FACE_PARSER_HPP
#define SCOP_UTILS_OBJ_FACE_PARSER_HPP

#include "utils/obj/obj_parser_types.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

bool parse_face_token(std::string_view tok, size_t vcount, size_t vtcount, size_t vncount, FaceVertRef &out);

template <typename Fn>
void for_each_triangle_in_face(const std::vector<FaceVertRef> &refs, Fn &&fn)
{
    for (size_t t = 1; t + 1 < refs.size(); ++t)
        fn(refs[0], refs[t], refs[t + 1]);
}

/* Builds one ObjVertex from a resolved face-vertex reference. */
ObjVertex build_triangle_vertex(const FaceVertRef &ref, const ParserState &state, const float normal[3], int material_id, uint32_t face_id);

#endif /* SCOP_UTILS_OBJ_FACE_PARSER_HPP */
