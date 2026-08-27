#include "utils/obj/obj_primitive_parser.hpp"
#include "utils/obj/obj_parse_utils.hpp"

#include "iostream/logger.hpp"

#include <sstream>
#include <string>

bool handle_line_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    ObjPolyline         polyline;
    std::string         tok;

    while (ls >> tok) {
        std::string_view    tok_view = tok;
        std::string_view    v_tok    = tok_view.substr(0, tok_view.find('/'));
        int                 idx      = 0;

        if (!resolve_index(v_tok, state.positions.size(), idx)) continue;
        if (static_cast<size_t>(idx) >= state.positions.size()) continue;
        
        polyline.points.push_back(state.positions[static_cast<size_t>(idx)]);
    }
    if (polyline.points.size() < 2)
    {
        ftpp::log::warning("load_obj: line ", state.line_no, ": malformed 'l' line, skipped");
        return true;        /* degenerate line: skip, not a hard failure */
    }

    state.current_lines.push_back(std::move(polyline));
    return true;
}

bool handle_point_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    std::string         tok;
    while (ls >> tok) {
        int                 idx     = 0;

        if (!resolve_index(tok, state.positions.size(), idx)) continue;
        if (static_cast<size_t>(idx) >= state.positions.size()) continue;
        
        state.current_points.push_back(state.positions[static_cast<size_t>(idx)]);
    }

    return true;
}
