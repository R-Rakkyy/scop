#include "utils/obj/obj_face_parser.hpp"
#include "utils/obj/obj_parse_utils.hpp"

bool parse_face_token(std::string_view tok, size_t vcount, size_t vtcount, size_t vncount, FaceVertRef &out)
{
    size_t              p1      = tok.find('/');
    std::string_view    v_tok   = (p1 == std::string_view::npos) ? tok : tok.substr(0, p1);

    if (!resolve_index(v_tok, vcount, out.v)) return false;
    out.vt = -1;
    out.vn = -1;
    if (p1 == std::string_view::npos)
        return true;

    std::string_view    rest    = tok.substr(p1 + 1);
    size_t              p2      = rest.find('/');
    std::string_view    vt_tok  = (p2 == std::string_view::npos) ? rest : rest.substr(0, p2);
    if (!vt_tok.empty())
        resolve_index(vt_tok, vtcount, out.vt);
    if (p2 == std::string_view::npos) return true;

    std::string_view vn_tok = rest.substr(p2 + 1);
    if (!vn_tok.empty()) 
        resolve_index(vn_tok, vncount, out.vn);

    return true;
}

ObjVertex build_triangle_vertex(const FaceVertRef &ref, const ParserState &state, const float normal[3], int material_id, uint32_t face_id)
{
    ObjVertex out{};
    out.face_id         = face_id;
    out.material_id     = material_id;

    const auto &p       = state.positions[static_cast<size_t>(ref.v)];
    out.pos[0] = p[0];
    out.pos[1] = p[1]; 
    out.pos[2] = p[2];

    out.normal[0] = normal[0];
    out.normal[1] = normal[1]; 
    out.normal[2] = normal[2];

    if (ref.vt >= 0 && static_cast<size_t>(ref.vt) < state.texcoords.size()) {
        const auto &uv = state.texcoords[static_cast<size_t>(ref.vt)];
        /* OBJ's v=0 is the bottom of the texture,
         * stb_image decodes row 0 as the top of the image. Need flip. */
        out.uv[0] = uv[0]; 
        out.uv[1] = (1.0f - uv[1]);
    } else {
        out.uv[0] = -1.0f;
        out.uv[1] = -1.0f;
    }
    return out;
}
