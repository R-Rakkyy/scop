#include "utils/math/matrix_utils.hpp"

#include <cmath>
#include <cstddef>

void mat4_to_floats(const ftpp::Matrix &m, float out[16])
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            out[c * 4 + r] = static_cast<float>(m.at(static_cast<std::size_t>(r), static_cast<std::size_t>(c)));
}

void rotated_axes(const ftpp::Matrix &rot, float out_x[3], float out_y[3], float out_z[3])
{
    // matrices are in column-major order.
    for (int r = 0; r < 3; ++r) {
        out_x[r] = static_cast<float>(rot.at(static_cast<std::size_t>(r), 0));
        out_y[r] = static_cast<float>(rot.at(static_cast<std::size_t>(r), 1));
        out_z[r] = static_cast<float>(rot.at(static_cast<std::size_t>(r), 2));
    }
}

ftpp::Matrix translate_matrix(float x, float y, float z)
{
    return ftpp::Matrix({
        ftpp::Vector({1, 0, 0, x}),
        ftpp::Vector({0, 1, 0, y}),
        ftpp::Vector({0, 0, 1, z}),
        ftpp::Vector({0, 0, 0, 1}),
    });
}

ftpp::Matrix rotate_x(float angle)
{
    double cos_x = std::cos(angle), sin_x = std::sin(angle);
    return ftpp::Matrix({
        ftpp::Vector({1, 0, 0, 0}),
        ftpp::Vector({0, cos_x, -sin_x, 0}),
        ftpp::Vector({0, sin_x, cos_x, 0}),
        ftpp::Vector({0, 0, 0, 1}),
    });
}

ftpp::Matrix rotate_y(float angle)
{
    double cos_y = std::cos(angle), sin_y = std::sin(angle);
    return ftpp::Matrix({
        ftpp::Vector({cos_y, 0, -sin_y, 0}),
        ftpp::Vector({0, 1, 0, 0}),
        ftpp::Vector({sin_y, 0, cos_y, 0}),
        ftpp::Vector({0, 0, 0, 1}),
    });
}

ftpp::Matrix rotate_z(float angle)
{
    double cos_z = std::cos(angle), sin_z = std::sin(angle);
    return ftpp::Matrix({
        ftpp::Vector({cos_z, -sin_z, 0, 0}),
        ftpp::Vector({sin_z, cos_z, 0, 0}),
        ftpp::Vector({0, 0, 1, 0}),
        ftpp::Vector({0, 0, 0, 1}),
    });
}
