#ifndef SCOP_MESH_ENTRY_HPP
#define SCOP_MESH_ENTRY_HPP

#include "engine/scene/mesh_transform.hpp"

#include <string>

/** One loaded mesh in the scene */
struct MeshEntry {
    int             renderer_index;
    std::string     name;

    MeshTransform   transform;
};

#endif /* SCOP_MESH_ENTRY_HPP */
