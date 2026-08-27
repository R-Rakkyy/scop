#ifndef SCOP_MATRIX_UTILS_HPP
#define SCOP_MATRIX_UTILS_HPP

#include "mathematics/matrix.hpp"

void                mat4_to_floats(const ftpp::Matrix &m, float out[16]);
void                rotated_axes(const ftpp::Matrix &rot, float out_x[3], float out_y[3], float out_z[3]);
ftpp::Matrix        translate_matrix(float x, float y, float z);
ftpp::Matrix        rotate_x(float angle);
ftpp::Matrix        rotate_y(float angle);
ftpp::Matrix        rotate_z(float angle);

#endif /* SCOP_MATRIX_UTILS_HPP */
