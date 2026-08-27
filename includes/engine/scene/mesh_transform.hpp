#ifndef SCOP_MESH_TRANSFORM_HPP
#define SCOP_MESH_TRANSFORM_HPP

#include "mathematics/matrix.hpp"

/** One loaded mesh's placement in the world:
 * 
 * translation relative to its own bounding-box center,
 * plus rotation from two independent controls:
 *  
 *  - rot_x/rot_y/rot_z: arrow keys. Plain Euler totals about the mesh's
 *    base (world) axes, rebuilt fresh every frame via rot_z*rot_y*rot_x.
 * 
 *  - rot: mouse-drag. An accumulated matrix (see SceneManager::update)
 *    that composes each drag onto the current orientation in world/view
 *    space
 * 
 * The two combine at read time (see mesh_rotation()): drags spin the
 * object as already-rotated by the arrows, on top of the base pose. */
struct MeshTransform {
    float center[3]{0, 0, 0};
    
    float tx    = 0;
    float ty    = 0;
    float tz    = 0;
    
    float rot_x = 0;
    float rot_y = 0;
    float rot_z = 0;
    
    ftpp::Matrix rot = ftpp::Matrix({
        ftpp::Vector({1, 0, 0, 0}),
        ftpp::Vector({0, 1, 0, 0}),
        ftpp::Vector({0, 0, 1, 0}),
        ftpp::Vector({0, 0, 0, 1}),
    });
};

/** @brief Build a rotation matrix from the MeshTransform's Euler angles and accumulated rotation.
 * 
 * @param t The MeshTransform to use for building the rotation matrix.
 * @return A 4x4 rotation matrix representing the combined rotation.
 */
ftpp::Matrix mesh_rotation(const MeshTransform &t);

/** @brief Build a model matrix from the MeshTransform.
 * 
 * @param t The MeshTransform to use for building the model matrix.
 * @return A 4x4 model matrix representing the mesh's position and orientation.
 */
ftpp::Matrix model_matrix(const MeshTransform &t);

#endif /* SCOP_MESH_TRANSFORM_HPP */
