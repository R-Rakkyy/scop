#ifndef SCOP_UTILS_OBJ_GEOMETRY_PARSER_HPP
#define SCOP_UTILS_OBJ_GEOMETRY_PARSER_HPP

#include "utils/obj/obj_parser_types.hpp"

#include <string_view>

// 'v' x y z [w]
void handle_vertex_line(std::string_view rest, ParserState &state);

// 'vn' x y z
void handle_normal_line(std::string_view rest, ParserState &state);

// 'vt' u [v]
void handle_texcoord_line(std::string_view rest, ParserState &state);

// 'f' v[/vt][/vn] ...
void handle_face_line(std::string_view rest, ParserState &state);

#endif /* SCOP_UTILS_OBJ_GEOMETRY_PARSER_HPP */
