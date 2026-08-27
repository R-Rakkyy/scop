#include "utils/obj/mtl_loader.hpp"
#include "utils/obj/obj_parse_utils.hpp"

#include "utils/file_management.h"
#include "iostream/logger.hpp"
#include "stb_image.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace {

void load_material_texture(std::string_view rest, const std::string &mtl_dir, ObjTexture &tex)
{
    std::istringstream      ls{std::string(rest)};
    std::string             tok;
    
    while (ls >> tok) tex.path = tok;
    if (tex.path.empty()) return;

    decode_texture_file(mtl_dir + tex.path, tex);
}

void parse_floats(std::string_view rest, float *out, int n)
{
    std::istringstream      ls{std::string(rest)};
    std::string             tok;

    for (int i = 0; i < n && (ls >> tok); ++i)
        parse_float(tok, out[i]);
}

void parse_material_directive(std::string_view keyword, std::string_view rest, const std::string &mtl_dir, ObjMaterial &mat)
{
    if (keyword == "map_Kd")                        { load_material_texture(rest, mtl_dir, mat.map_Kd);     return; }
    if (keyword == "map_bump" || keyword == "bump") { load_material_texture(rest, mtl_dir, mat.map_bump);   return; }
    if (keyword == "Kd")                            { parse_floats(rest, mat.Kd, 3);                        return; }
    if (keyword == "Ks")                            { parse_floats(rest, mat.Ks, 3);                        return; }
    if (keyword == "Ns")                            { parse_floats(rest, &mat.Ns, 1);                       return; }
    // lazy ignore the rest of the .mtl spec for now, since we don't use it
}

} // namespace

bool load_mtl(const char *path, std::unordered_map<std::string, ObjMaterial> &out, std::string *out_first_name) noexcept
{
    void        *file_buf           = nullptr;
    size_t      size                = 0;
    if (!fm_read_file(path, &file_buf, &size)) return false;

    std::string_view    path_view(path);
    size_t              slash       = path_view.find_last_of('/');
    std::string         mtl_dir     = (slash == std::string_view::npos) ? std::string{} : std::string(path_view.substr(0, slash + 1));


    std::string_view    data(static_cast<const char *>(file_buf), size);
    ObjMaterial         *current    = nullptr;
    size_t              pos         = 0;
    while (pos < data.size()) {
        size_t              nl      = data.find('\n', pos);
        std::string_view    line    = (nl == std::string_view::npos) ? data.substr(pos) : data.substr(pos, nl - pos);
        pos                         = (nl == std::string_view::npos) ? data.size() : nl + 1;

        std::string_view    view    = trim(line); // view is now the current line we are in
        if (view.empty() || view[0] == '#') continue;

        size_t sp                   = view.find_first_of(" \t");
        std::string_view keyword    = (sp == std::string_view::npos) ? view : view.substr(0, sp);
        std::string_view rest       = (sp == std::string_view::npos) ? std::string_view{} : view.substr(sp + 1);

        if (keyword == "newmtl") {
            std::string name(trim(rest));
            if (out_first_name && out_first_name->empty())
                *out_first_name     = name;
            
            ObjMaterial mat{};
            mat.name                = name;
            current                 = &(out[name] = std::move(mat)); // trick to get a reference to the newly inserted material
            continue;
        }
        if (current) parse_material_directive(keyword, rest, mtl_dir, *current);
    }
    free(file_buf);
    return true;
}

bool decode_texture_file(const std::string &full_path, ObjTexture &tex) noexcept
{
    void    *file_buf      = nullptr;
    size_t  size           = 0;
    if (!fm_read_file(full_path.c_str(), &file_buf, &size)) {
        ftpp::log::warning("decode_texture_file: could not read '", full_path, "'");
        return false;
    }

    int w           = 0;
    int h           = 0;
    int channels    = 0;
    stbi_uc *pixels = stbi_load_from_memory(static_cast<const stbi_uc *>(file_buf), static_cast<int>(size), &w, &h, &channels, 4);
    free(file_buf);
    if (!pixels) {
        ftpp::log::warning("decode_texture_file: failed to decode '", full_path, "': ", stbi_failure_reason());
        return false;
    }

    tex.rgba    = std::shared_ptr<uint8_t[]>(pixels, [](uint8_t *p) { stbi_image_free(p); });
    tex.width   = static_cast<uint32_t>(w);
    tex.height  = static_cast<uint32_t>(h);
    tex.decoded = true;
    return true;
}
