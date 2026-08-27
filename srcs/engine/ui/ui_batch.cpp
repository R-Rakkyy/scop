#include "engine/ui/ui_batch.hpp"

/* stb_easy_font.h declares a couple of helpers (e.g. stb_easy_font_spacing) that
 * this project doesn't call. However those helpers then create warnings. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
    #include "stb_easy_font.h"
#pragma GCC diagnostic pop

#include <cmath>
#include <vector>

namespace {

constexpr float k_solid_uv[4] = {0.0f, 0.0f, 1.0f, 1.0f}; /* whole solid-white texture */

/* couldn't be more easy to understand */
struct EasyVert { float x, y, z; unsigned char color[4]; };

void sanitize_for_easy_font(std::vector<char> &text_buf)
{
    for (char &c : text_buf) {
        // // replace upper case to lower case.
        // if (c >= 'A' && c <= 'Z')   c += ('a' - 'A');

        // stb_easy_font only supports ASCII 32-126. Anything else is replaced with a question mark.
        if (c < 32 || c > 126)      c =  '?';
    }
}

} // namespace

void UiBatch::clear()   { _verts.clear(); }

void UiBatch::push_quad(float x, float y, float w, float h, const float uv[4], const float color[4])
{
    const float x0 = x;
    const float y0 = y;
    const float x1 = x + w;
    const float y1 = y + h;

    const float u0 = uv[0];
    const float v0 = uv[1];
    const float u1 = uv[2];
    const float v1 = uv[3];

    auto emit = [&](float px, float py, float pu, float pv) {
        _verts.insert(_verts.end(), { px, py, pu, pv, color[0], color[1], color[2], color[3] });
    };

    // we do le funny triangles with a square, hehe :3
    // (abcd) => (a,b,c) + (a,c,d) 
    //        => (a.x,a.y) (b.x,b.y) (c.x,c.y) + (a.x,a.y) (c.x,c.y) (d.x,d.y)
    //        => (x0,y0) (x1,y0) (x1,y1) + (x0,y0) (x1,y1) (x0,y1)
    emit(x0, y0, u0, v0);
    emit(x1, y0, u1, v0); 
    emit(x1, y1, u1, v1);
    // second triangle
    emit(x0, y0, u0, v0); 
    emit(x1, y1, u1, v1);
    emit(x0, y1, u0, v1);
}

void UiBatch::push_solid(float x, float y, float w, float h, const float color[4])
{
    push_quad(x, y, w, h, k_solid_uv, color);
}

void UiBatch::push_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const float color[4])
{
    float u = k_solid_uv[0]; /* any point inside the solid texture is fully opaque */
    float v = k_solid_uv[1]; /* any point inside the solid texture is fully opaque */

    _verts.insert(_verts.end(), { x0, y0, u, v, color[0], color[1], color[2], color[3] });
    _verts.insert(_verts.end(), { x1, y1, u, v, color[0], color[1], color[2], color[3] });
    _verts.insert(_verts.end(), { x2, y2, u, v, color[0], color[1], color[2], color[3] });
}

void UiBatch::push_thick_line(float cx, float cy, float dx, float dy, const float color[4])
{
    float len   = std::sqrt(dx * dx + dy * dy);
    float px    = (len > 0.0001f) ? -dy / len * 1.5f : 0.0f;
    float py    = (len > 0.0001f) ?  dx / len * 1.5f : 0.0f;
    float ex    = cx + dx, ey = cy + dy;

    push_triangle(cx - px, cy - py, cx + px, cy + py, ex + px, ey + py, color);
    push_triangle(cx - px, cy - py, ex + px, ey + py, ex - px, ey - py, color);
}

void UiBatch::push_text(float x, float y, float scale, const std::string &text, const float color[4])
{
    if (text.empty()) return;

    /* ~270 bytes/char worst case per stb_easy_font's own docs. */
    std::vector<char> buf(text.size() * 280 + 64);          // +64 for stb_easy_font's own internal buffer alignment
    std::vector<char> text_buf(text.begin(), text.end());
    sanitize_for_easy_font(text_buf);
    text_buf.push_back('\0');                               // stb_easy_font_print expects a null-terminated string

    int num_quads = stb_easy_font_print(0.0f, 0.0f, text_buf.data(), nullptr, buf.data(), static_cast<int>(buf.size()));

    float u = k_solid_uv[0];
    float v = k_solid_uv[1];

    const auto *verts = reinterpret_cast<const EasyVert *>(buf.data()); /* auto reinterpret_cast like a chad */
    auto emit = [&](const EasyVert &e) {
        _verts.insert(_verts.end(), { x + e.x * scale, y + e.y * scale, u, v, color[0], color[1], color[2], color[3] });
    };

    for (int q = 0; q < num_quads; ++q) {
        const EasyVert &v0 = verts[q * 4 + 0];
        const EasyVert &v1 = verts[q * 4 + 1];
        const EasyVert &v2 = verts[q * 4 + 2];
        const EasyVert &v3 = verts[q * 4 + 3];
        
        emit(v0); emit(v1); emit(v2);
        emit(v0); emit(v2); emit(v3);
    }
}

float UiBatch::text_width(const std::string &text, float scale) const
{
    std::vector<char> tmp(text.begin(), text.end());

    sanitize_for_easy_font(tmp);
    tmp.push_back('\0');
    
    return static_cast<float>(stb_easy_font_width(tmp.data())) * scale;
}

float UiBatch::glyph_advance(float scale) const
{
    char h[] = "A";
    return static_cast<float>(stb_easy_font_height(h)) * scale;
}

const std::vector<float> &UiBatch::vertices() const
{
    return _verts;
}

uint32_t UiBatch::vertex_count() const
{
    return static_cast<uint32_t>(_verts.size() / FLOATS_PER_VERTEX);
}
