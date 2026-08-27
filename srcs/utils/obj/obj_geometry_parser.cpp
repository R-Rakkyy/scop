#include "utils/obj/obj_geometry_parser.hpp"
#include "utils/obj/obj_face_parser.hpp"
#include "utils/obj/obj_parse_utils.hpp"

#include "iostream/logger.hpp"

#include <cmath>
#include <sstream>
#include <string>

void handle_vertex_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    std::string         sx, sy, sz, sw;
    float               x,  y,  z,  w           = 1.0f;

    if (!(ls >> sx >> sy >> sz) || !parse_float(sx, x) || !parse_float(sy, y) || !parse_float(sz, z)) {
        ftpp::log::warning("load_obj: line ", state.line_no, ": malformed 'v' line, skipped");
        return;
    }
    
    if ((ls >> sw) && parse_float(sw, w) && std::fabs(w) > 1e-9f && std::fabs(w - 1.0f) > 1e-9f) {
        x /= w;
        y /= w;
        z /= w;
    }
    state.positions.push_back({x, y, z});
}

void handle_normal_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    std::string         sx, sy, sz;
    float               x,  y,  z;

    if (!(ls >> sx >> sy >> sz) || !parse_float(sx, x) || !parse_float(sy, y) || !parse_float(sz, z)) {
        ftpp::log::warning("load_obj: line ", state.line_no, ": malformed 'vn' line, skipped");
        return;
    }

    /* Explicit normals aren't guaranteed unit-length by the spec. */
    float len           = std::sqrt(x * x + y * y + z * z);
    if (len > 1e-9f) { 
        x /= len;
        y /= len; 
        z /= len;
    }
    state.normals.push_back({x, y, z});
}

void handle_texcoord_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    std::string         su,         sv;
    float               u = 0.0f,   v = 0.0f;

    if (!(ls >> su) || !parse_float(su, u)) {
        ftpp::log::warning("load_obj: line ", state.line_no, ": malformed 'vt' line, skipped");
        return;
    }
    if (ls >> sv)
        parse_float(sv, v);
    state.texcoords.push_back({u, v});
}

void handle_face_line(std::string_view rest, ParserState &state)
{
    std::istringstream          ls{std::string(rest)};
    std::vector<FaceVertRef>    refs;
    std::string                 tok;

    while (ls >> tok) {
        FaceVertRef ref{};

        if (!parse_face_token(tok, state.positions.size(), state.texcoords.size(), state.normals.size(), ref)) {
            ftpp::log::warning("load_obj: line ", state.line_no, ": face token '", tok, "' has no valid vertex index, corner dropped");
            continue;
        }
        if (static_cast<size_t>(ref.v) >= state.positions.size()) {
            ftpp::log::warning("load_obj: line ", state.line_no, ": face token '", tok, "' references out-of-range vertex, corner dropped");
            continue;
        }
        refs.push_back(ref);
    }

    if (refs.size() < 3) {
        if (!refs.empty())
            ftpp::log::warning("load_obj: line ", state.line_no, ": face left with ", refs.size(), " valid corner(s) after drops, whole face skipped");
        return; /* degenerate face: skip, not a hard failure */
    }

    // dont ask why i felt like it this time
    state.current_faces.push_back(
                                PendingFace{std::move(refs), 
                                            state.current_material_name,
                                            state.current_smoothing_group,
                                            state.next_face_id++
                                            }
                                );
}
