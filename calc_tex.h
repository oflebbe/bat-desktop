#ifndef CALC_TEX_H
#define CALC_TEX_H

#include <stdint.h>
#include <assert.h>
#include "glad.h"
#include "flo_pixmap.h"
#include "flo_matrix.h"

typedef struct
{
    float rot_h;
    float s;
    float v;
} hsv_rotate_t;

static inline __attribute__((always_inline)) uint16_t map_hsv_rotated(const hsv_rotate_t hsv, float h)
{
    h += hsv.rot_h;
    if (h >= 1.0f)
    {
        h -= 1.0f;
    }
    return flo_hsvToRgb565(h, hsv.s, hsv.v);
}

typedef void (*tex_apply_f)(GLuint texture_id, const flo_pixmap_t *p, unsigned int width, unsigned int height, bool first);

void calculate_texture_line_hsv(const flo_matrix_t *mat, const hsv_rotate_t hsv, unsigned int num_tex_line, GLuint texture_id[num_tex_line], tex_apply_f tex, bool first)
{
    assert(mat);
    //   assert(hsv);

    const unsigned int height = mat->height;
    const unsigned int width = mat->width;

    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (unsigned int i = 0; i < num_tex_line; i++)
    {
#pragma omp parallel for
        for (unsigned int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                unsigned real_w = w + i * TEXTURE_WIDTH;
                flo_pixmap_set_pixel(p, w, h, map_hsv_rotated(hsv, 1.0f - flo_matrix_get_value(mat, real_w, h)));
            }
        }
        if (tex)
        {
            tex(texture_id[i], p, TEXTURE_WIDTH, height, first);
        }
    }
    free(p);
}

void calculate_texture_line_grey(const flo_matrix_t *mat, unsigned int num_tex_line, GLuint texture_id[num_tex_line], tex_apply_f tex, bool first)
{
    assert(mat);

    const unsigned int height = mat->height;
    const unsigned int width = mat->width;
    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (unsigned int i = 0; i < num_tex_line; i++)
    {
#pragma omp parallel for
        for (unsigned int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                unsigned real_w = w + i * TEXTURE_WIDTH;
                flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565(0, 0, 1.0f - flo_matrix_get_value(mat, real_w, h)));
            }
        }
        if (tex)
        {
            tex(texture_id[i], p, width, height, first);
        }
    }
    free(p);
}
#endif