#include "utils/obj/obj_directive_parser.hpp"
#include "utils/obj/obj_finalize.hpp"
#include "utils/obj/obj_parse_utils.hpp"
#include "utils/obj/mtl_loader.hpp"

#include "iostream/logger.hpp"

#include <sstream>
#include <string>

void handle_object_line(std::string_view rest, ParserState &state)
{
    std::string name(trim(rest));

    if (!parser_state_object_is_empty(state)) {
        finalize_object(state); // finalize the current object and start a new one
    }

    state.name = std::move(name);
}

void handle_smoothing_line(std::string_view rest, ParserState &state)
{
    std::string_view    tok     = trim(rest);
    int                 group   = 0;

    if (tok != "off" && tok != "0") parse_int(tok, group);
    state.current_smoothing_group = group;
}

void handle_usemtl_line(std::string_view rest, ParserState &state)
{
    state.current_material_name = std::string(trim(rest));
}

void handle_mtllib_line(std::string_view rest, ParserState &state)
{
    std::istringstream  ls{std::string(rest)};
    std::string         filename;

    while (ls >> filename) {
        std::string full_path   = state.obj_dir + filename;
        std::string first_name;

        if (!load_mtl(full_path.c_str(), state.available_materials, &first_name)) {
            ftpp::log::warning("load_obj: mtllib '", full_path, "' could not be read");
            continue;
        }

        if (state.current_material_name.empty() && !first_name.empty())
            state.current_material_name = first_name;
    }
}
