#ifndef _UTIL_H
#define _UTIL_H 1

// Various utilities.

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <Eigen/Core>

#include "opengl.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// Converts a HSV color to RGB. Assumes h in [0, 2pi> or [-pi, pi>
void hsv2rgb(float h, float s, float v, float *r, float *g, float *b);

// Converts a HSV color to RGB, but keeps luminance constant
// (ie. color luminance is as if S=0).
void hsv2rgb_normalized(float h, float s, float v, float *r, float *g, float *b);

// Read a file from disk and return its contents.
// Dies if the file does not exist.
std::string read_file(const std::string &filename);

// Compile the given GLSL shader (typically a vertex or fragment shader)
// and return the object number.
GLuint compile_shader(const std::string &shader_src, GLenum type);

// Print a 3x3 matrix to standard output. Useful for debugging.
void print_3x3_matrix(const Eigen::Matrix3d &m);

// Output a GLSL 3x3 matrix declaration.
std::string output_glsl_mat3(const std::string &name, const Eigen::Matrix3d &m);

// Calculate where to sample, and with what weight, if one wants to use
// the GPU's bilinear hardware to sample w1 * x[0] + w2 * x[1].
static inline void combine_two_samples(float w1, float w2, float *offset, float *total_weight)
{
	assert(w1 * w2 >= 0.0f);  // Should not have differing signs.
	if (fabs(w1 + w2) < 1e-6) {
		*offset = 0.5f;
		*total_weight = 0.0f;
	} else {
		*offset = w2 / (w1 + w2);
		*total_weight = w1 + w2;
	}
	assert(*offset >= 0.0f);
	assert(*offset <= 1.0f);
}

#ifdef NDEBUG
#define check_error()
#else
#define check_error() { int err = glGetError(); if (err != GL_NO_ERROR) { printf("GL error 0x%x at %s:%d\n", err, __FILE__, __LINE__); exit(1); } }
#endif

#endif // !defined(_UTIL_H)
