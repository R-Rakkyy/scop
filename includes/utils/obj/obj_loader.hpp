#ifndef SCOP_OBJ_LOADER_HPP
#define SCOP_OBJ_LOADER_HPP

#include "utils/obj/obj_material.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/* One triangle-corner vertex, produced by fan-triangulating an OBJ "f"
 * line. Non-indexed: shared corners are duplicated. */
struct ObjVertex {
    uint32_t    face_id;
    int         material_id;    /* index into the owning ObjMesh::materials, -1 if none */

    float       pos[3];
    float       normal[3];      /* explicit vn, smoothing-group average, or flat fallback */
    float       uv[2];          /* explicit vt, or planar fallback */
};

/** One "l" statement: an ordered polyline of baked positions. */
struct ObjPolyline {
    std::vector<std::array<float, 3>> points;
};

/** One drawable object: everything in a .obj between
 * an "o" line and the next one (or EOF).
 * A file with no "o" lines still produces exactly one,
 * unnamed, ObjMesh holding all of its geometry.
 * 
 * @note mesh->lines & mesh->points are not used nor rendered.
 */
struct ObjMesh {
    std::string                         name;

    float                               aabb_min[3];        /* basically the bounding boxes of the mesh, used to get the center and bounds */
    float                               aabb_max[3];        /* basically the bounding boxes of the mesh, used to get the center and bounds */

    std::vector<ObjVertex>              vertices;           /* flat, triangulated, non-indexed (from "f") */
    std::vector<ObjMaterial>            materials;          /* materials referenced by this object, in first-use order */
    std::vector<ObjPolyline>            lines;              /* from "l" statements */
    std::vector<std::array<float, 3>>   points;             /* from "p" statements */
};

/* Every object found in one .obj file, in file order. */
struct ObjFile {
    std::vector<ObjMesh> objects;
};

/* Parses a full .obj file (v/vn/vt/f/o/s/l/p/usemtl/mtllib), plus every referenced .mtl file. */
bool load_obj(const char *path, ObjFile &out) noexcept;
std::vector<std::string> list_obj_paths(const std::string &path);

#endif /* SCOP_OBJ_LOADER_HPP */
