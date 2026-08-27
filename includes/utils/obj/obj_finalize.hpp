#ifndef SCOP_UTILS_OBJ_FINALIZE_HPP
#define SCOP_UTILS_OBJ_FINALIZE_HPP

#include "utils/obj/obj_parser_types.hpp"

/*
 * ### IMPORTANT NOTE:
 *  Right now, mesh->lines & mesh->points are not used nor rendered. Never.
 */
void finalize_object(ParserState &state);

#endif /* SCOP_UTILS_OBJ_FINALIZE_HPP */
