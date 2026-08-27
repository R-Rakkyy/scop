#ifndef SCOP_UTILS_OBJ_PRIMITIVE_PARSER_HPP
#define SCOP_UTILS_OBJ_PRIMITIVE_PARSER_HPP

#include "utils/obj/obj_parser_types.hpp"

#include <string_view>

/* "l" and "p" statements reference already-known positions
 * they're put into plain float positions immediately rather
 * than deferred to finalize. 
 */

bool handle_line_line(std::string_view rest, ParserState &state);
bool handle_point_line(std::string_view rest, ParserState &state);

#endif /* SCOP_UTILS_OBJ_PRIMITIVE_PARSER_HPP */
