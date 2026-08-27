#ifndef SCOP_UTILS_OBJ_DIRECTIVE_PARSER_HPP
#define SCOP_UTILS_OBJ_DIRECTIVE_PARSER_HPP

#include "utils/obj/obj_parser_types.hpp"

#include <string_view>

// 'o' name
void handle_object_line(std::string_view rest, ParserState &state);

// 's' group_number | off
void handle_smoothing_line(std::string_view rest, ParserState &state);

// 'usemtl' material_name
void handle_usemtl_line(std::string_view rest, ParserState &state);

// 'mtllib' filename [filename ...]
void handle_mtllib_line(std::string_view rest, ParserState &state);

#endif /* SCOP_UTILS_OBJ_DIRECTIVE_PARSER_HPP */
