#include "utils/obj/obj_loader.hpp"
#include "utils/obj/obj_parser_types.hpp"

#include "utils/obj/obj_parse_utils.hpp"
#include "utils/obj/obj_geometry_parser.hpp"
#include "utils/obj/obj_directive_parser.hpp"
#include "utils/obj/obj_primitive_parser.hpp"
#include "utils/obj/obj_finalize.hpp"

#include "utils/file_management.h"
#include "iostream/logger.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

namespace {

std::string obj_directory_of(const char *path)
{
    std::string_view    p(path);
    size_t              slash   = p.find_last_of('/');
    
    return (slash == std::string_view::npos) ? std::string{} : std::string(p.substr(0, slash + 1));
}

void dispatch_line(std::string_view line, ParserState &state)
{
    std::string_view view = trim(line);
    if (view.empty() || view[0] == '#') return;

    size_t sp                   = view.find_first_of(" \t");
    std::string_view keyword    = (sp == std::string_view::npos) ? view : view.substr(0, sp);
    std::string_view rest       = (sp == std::string_view::npos) ? std::string_view{} : view.substr(sp + 1);

    if (keyword == "v")         { handle_vertex_line(rest, state);      return; }
    if (keyword == "vn")        { handle_normal_line(rest, state);      return; }
    if (keyword == "vt")        { handle_texcoord_line(rest, state);    return; }
    if (keyword == "f")         { handle_face_line(rest, state);        return; }
    if (keyword == "l")         { handle_line_line(rest, state);        return; }
    if (keyword == "p")         { handle_point_line(rest, state);       return; }
    if (keyword == "o")         { handle_object_line(rest, state);      return; }
    if (keyword == "s")         { handle_smoothing_line(rest, state);   return; }
    if (keyword == "usemtl")    { handle_usemtl_line(rest, state);      return; }
    if (keyword == "mtllib")    { handle_mtllib_line(rest, state);      return; }

    /* unrecognized keyword: skipped */
    ftpp::log::debug("load_obj: line ", state.line_no, ": unrecognized keyword '", keyword, "', skipped");
}

} // namespace

bool load_obj(const char *path, ObjFile &out) noexcept
{
    out.objects.clear();

    void    *file_buf   = nullptr;
    size_t  size        = 0;
    if (!fm_read_file(path, &file_buf, &size)) {
        ftpp::log::error("load_obj: failed to read ", path);
        return false;
    }

    ParserState state;
    state.obj_dir = obj_directory_of(path);

    /* Line-split the raw buffer directly with string_views. */
    std::string_view    data(static_cast<const char *>(file_buf), size);
    size_t              pos = 0;
    
    while (pos < data.size()) {
        size_t              nl      = data.find('\n', pos);
        std::string_view    line    = (nl == std::string_view::npos) ? data.substr(pos) : data.substr(pos, nl - pos);
        
        ++state.line_no;
        dispatch_line(line, state);

        if (nl == std::string_view::npos) break;
        pos = nl + 1;
    }
    free(file_buf);
    finalize_object(state);

    out.objects = std::move(state.objects);
    ftpp::log::info("load_obj: parsed ", out.objects.size(), " object(s) from ", path);
    return true;
}

std::vector<std::string> list_obj_paths(const std::string &path)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec) || ec) return {path};

    std::vector<std::string> found;
    for (const auto &entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) break;
        if (entry.is_regular_file() && entry.path().extension() == ".obj")
            found.push_back(entry.path().string());
    }

    std::sort(found.begin(), found.end());
    if (found.empty()) ftpp::log::warning("list_obj_paths: no .obj files found in directory '", path, "'");
    
    return found;
}
