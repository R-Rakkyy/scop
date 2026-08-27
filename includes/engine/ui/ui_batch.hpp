#ifndef SCOP_UI_BATCH_HPP
#define SCOP_UI_BATCH_HPP

#include <cstdint>
#include <string>
#include <vector>

/* Immediate-mode 2D quad batcher: builds one interleaved vertex buffer
 * per frame (pos.xy in screen pixels, uv, rgba), consumed by vk_ui_draw. */
class UiBatch {
    public:
        UiBatch()                           = default;
        UiBatch(const UiBatch &)            = delete;
        UiBatch &operator=(const UiBatch &) = delete;
        UiBatch(UiBatch &&)                 = default;
        UiBatch &operator=(UiBatch &&)      = default;
        ~UiBatch()                          = default;

        void clear();

        /** Pushes a textured quad to the batch, using two triangles from a rect + uv coordinates */
        void push_quad(float x, float y, float w, float h, const float uv[4], const float color[4]);
        /** Pushes a solid color quad to the batch, using two triangles from a rect, using k_solid_uv */
        void push_solid(float x, float y, float w, float h, const float color[4]);
        /** Pushes three raw verts, same solid UV trick. */
        void push_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const float color[4]);
        /** Pushes a thick line segment (two triangles) from (cx,cy) to (cx+dx, cy+dy). */
        void push_thick_line(float cx, float cy, float dx, float dy, const float color[4]);
        /** Pushes text to the batch, using stb_easy_font_print and EasyVert. */
        void push_text(float x, float y, float scale, const std::string &text, const float color[4]);

        /* Text is built from stb_easy_font. This function query information about the text's dimensions. */
        float text_width(const std::string &text, float scale) const;
        /** Returns the horizontal advance of a single glyph at the given scale. */
        float glyph_advance(float scale) const;

        const std::vector<float> &vertices() const;
        uint32_t vertex_count() const;

        static constexpr int FLOATS_PER_VERTEX = 8; /* pos(2) uv(2) color(4) */

    private:
        std::vector<float> _verts;
};

#endif /* SCOP_UI_BATCH_HPP */
