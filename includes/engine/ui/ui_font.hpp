#ifndef SCOP_UI_FONT_HPP
#define SCOP_UI_FONT_HPP

#include <cstdint>
#include <vector>

inline constexpr uint32_t   UI_SOLID_TEXTURE_WIDTH    = 8;
inline constexpr uint32_t   UI_SOLID_TEXTURE_HEIGHT   = 8;

/** Builds a solid-white RGBA texture for the UI font atlas. Each pixel is 4 bytes (RGBA) and set to 255.
    @note It's a placeholder to satisfy the descriptor-set binding, not a real font. w*/
std::vector<uint8_t>        build_ui_solid_texture();

#endif /* SCOP_UI_FONT_HPP */
