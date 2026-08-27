#ifndef SCOP_UTILS_OBJ_PARSE_UTILS_HPP
#define SCOP_UTILS_OBJ_PARSE_UTILS_HPP

#include <cstddef>
#include <string_view>

bool                parse_float(std::string_view s, float &out);
bool                parse_int(std::string_view s, int &out);
bool                resolve_index(std::string_view token, size_t count, int &out);
std::string_view    trim(std::string_view s);

#endif /* SCOP_UTILS_OBJ_PARSE_UTILS_HPP */
