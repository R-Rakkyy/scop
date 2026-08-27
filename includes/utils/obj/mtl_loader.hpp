#ifndef SCOP_UTILS_OBJ_MTL_LOADER_HPP
#define SCOP_UTILS_OBJ_MTL_LOADER_HPP

#include "utils/obj/obj_material.hpp"

#include <string>
#include <unordered_map>

bool load_mtl(const char *path, std::unordered_map<std::string, ObjMaterial> &out, std::string *out_first_name = nullptr) noexcept;
bool decode_texture_file(const std::string &full_path, ObjTexture &tex) noexcept;

#endif /* SCOP_UTILS_OBJ_MTL_LOADER_HPP */
