#include "engine/ui/ui_font.hpp"

std::vector<uint8_t>        build_ui_solid_texture()
{
    return std::vector<uint8_t>(static_cast<size_t>(UI_SOLID_TEXTURE_WIDTH) * UI_SOLID_TEXTURE_HEIGHT * 4, 255);
}
