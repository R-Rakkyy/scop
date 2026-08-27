#ifndef SCOP_UTILS_OBJ_PARSER_TYPES_HPP
#define SCOP_UTILS_OBJ_PARSER_TYPES_HPP

#include "utils/obj/obj_loader.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/* "v[/vt][/vn]" token of an "f", "l" or "p" line.
 * 0-based indices. -1 when the token didn't specify them. */
struct FaceVertRef {
    int v;
    int vt;
    int vn;
};

/* "f" group: a list of vertex references, plus the material and smoothing group.
 * Note: they are kept alive until finalize. */
struct PendingFace {
    std::vector<FaceVertRef>    refs;
    std::string                 material_name;          /* empty if no "usemtl" is active; resolved to an index at finalize */
    int                         smoothing_group;        /* 0 means "off": never averaged with other faces */
    uint32_t                    face_id;
};

/* All mutable state threaded through the line-by-line parse of one .obj file.
 * Positions/normals/texcoords are file-global (OBJ indices never
 * reset at an object boundary); everything else describes the object
 * currently being built. */
struct ParserState {
    std::vector<std::array<float, 3>>               positions;
    std::vector<std::array<float, 3>>               normals;
    std::vector<std::array<float, 2>>               texcoords;

    std::unordered_map<std::string, ObjMaterial>    available_materials;            /* from mtllib */
    std::string                                     current_material_name;          /* empty if no "usemtl" is active; persists across "o" boundaries */
    int                                             current_smoothing_group = 0;

    std::string                                     name;                           /* in-progress object's name */
    std::vector<PendingFace>                        current_faces;
    std::vector<ObjPolyline>                        current_lines;
    std::vector<std::array<float, 3>>               current_points;
    uint32_t                                        next_face_id = 0;

    std::vector<ObjMesh>                            objects;                        /* finalized objects, in file order */
    std::string                                     obj_dir;                        /* directory containing the .obj file, for mtllib resolution */

    size_t                                          line_no = 0;                    /* 1-based line of the file currently being dispatched, for warnings */
};

inline bool parser_state_object_is_empty(const ParserState &state) {
    return state.current_faces.empty() && state.current_lines.empty() && state.current_points.empty();
}

#endif /* SCOP_UTILS_OBJ_PARSER_TYPES_HPP */
