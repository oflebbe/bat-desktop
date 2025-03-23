// MIT License
// (c) Olaf Flebbe 2024

// A rgb16 pixmap

#ifndef FLO_MATRIX_H
#define FLO_MATRIX_H

#include <stdlib.h>
#include <assert.h>

// float flo_matrix_t
typedef struct
{
  size_t len;
  unsigned int width;
  unsigned int height;

  float buf[];
} flo_matrix_t;

// allocates an flo_pixmap_t
flo_matrix_t *flo_matrix_create(unsigned int width, unsigned int height);

void flo_matrix_set_value(flo_matrix_t *matrix, unsigned int x,
                          unsigned int y, float value);
__attribute__((always_inline)) inline float flo_matrix_get_value(const flo_matrix_t *matrix, unsigned int x,
                                                                 unsigned int y)
{
  assert(x < matrix->width);
  assert(y < matrix->height);
  return matrix->buf[y * matrix->width + x];
}

#ifdef FLO_MATRIX_IMPLEMENTATION

#include <stdlib.h>
#include <stdint.h>

// allocates a flo_pixmap_t
// should be deallocated with free
flo_matrix_t *flo_matrix_create(unsigned int width, unsigned int height)
{
  size_t len = (long)width * (long)height;
  flo_matrix_t *matrix = (flo_matrix_t *)calloc(1, sizeof(flo_matrix_t) + sizeof(float) * len);
  if (matrix == NULL)
  {
    abort();
  }
  *matrix = (flo_matrix_t){.width = width, .height = height, .len = len};

  return matrix;
}

void flo_matrix_set_value(flo_matrix_t *matrix, unsigned int x,
                          unsigned int y, float value)
{
  assert(x < matrix->width);
  assert(y < matrix->height);
  matrix->buf[y * matrix->width + x] = value;
}

#endif

#endif
