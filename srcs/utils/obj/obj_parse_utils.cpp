#include "utils/obj/obj_parse_utils.hpp"

#include <charconv>

bool parse_float(std::string_view s, float &out)
{
    s           =   trim(s);
    auto res    =   std::from_chars(s.data(), s.data() + s.size(), out); // i love cpp :DDDD

    return res.ec == std::errc();
}

bool parse_int(std::string_view s, int &out)
{
    s           =   trim(s);
    auto res    =   std::from_chars(s.data(), s.data() + s.size(), out);

    return res.ec == std::errc();
}

bool resolve_index(std::string_view token, size_t count, int &out)
{
    int idx = 0;
    if (!parse_int(token, idx))
        return false;
    
    if (idx < 0) idx = static_cast<int>(count) + idx + 1; // revserse search. -1 = last.

    if (idx <= 0) return false;
    
    out = idx - 1;
    return true;
}

std::string_view trim(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))                   s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);

    return s;
}
