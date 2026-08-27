#ifndef SCOP_UTILS_OBJ_MATERIAL_HPP
#define SCOP_UTILS_OBJ_MATERIAL_HPP

#include <cstdint>
#include <memory>
#include <string>

/* One texture reference from a .mtl "map_*" line, decoded via stb_image
 * (PNG/JPG/BMP/...). `decoded` is false if the path was empty or the
 * decode failed; `rgba` is always 4 bytes/pixel, row-major, top row first.
 * 
 * Note: rgba is a shared_ptr which should be freed with a custom deleter that calls stbi_image_free().
 */
struct ObjTexture {
    std::string     path;

    bool            decoded = false;
    
    uint32_t        width = 0;
    uint32_t        height = 0;
    
    std::shared_ptr<uint8_t[]> rgba;
};

/* One "newmtl" block from a .mtl file */
struct ObjMaterial {
    std::string     name;

    ObjTexture      map_Kd;
    ObjTexture      map_bump;

    float Kd[3]     = {1.0f, 1.0f, 1.0f};
    float Ks[3]     = {0.5f, 0.5f, 0.5f};
    float Ns        = 32.0f;
};

#endif /* SCOP_UTILS_OBJ_MATERIAL_HPP */
