#include "engine/scene/mesh_transform.hpp"
#include "utils/math/matrix_utils.hpp"

ftpp::Matrix mesh_rotation(const MeshTransform &t)
{
    ftpp::Matrix base = (rotate_z(t.rot_z) * rotate_y(t.rot_y) * rotate_x(t.rot_x));
    
    return t.rot * base;
}

ftpp::Matrix model_matrix(const MeshTransform &t)
{
    /*
     * The model matrix is built as follows:
     *   1. Translate the mesh to its position in world space, which is the sum of its center and translation offsets.
     *   2. Apply the combined rotation from the MeshTransform.
     *   3. Translate the mesh back to its original position by negating the center translation.
     * 
     * This ensures that the mesh rotates around its own center while being positioned correctly in world space.
     * Note: those matrices are 4x4.
    */
    ftpp::Matrix t_pos = translate_matrix(  t.center[0] + t.tx,
                                            t.center[1] + t.ty,
                                            t.center[2] + t.tz);

    ftpp::Matrix t_neg = translate_matrix(  -t.center[0],
                                            -t.center[1],
                                            -t.center[2]);
    
    return t_pos * mesh_rotation(t) * t_neg;
}
