#ifndef CALC_TEX_H
#define CALC_TEX_H


#include <stdint.h>
#include <assert.h>
#include "glad.h"
#include "flo_pixmap.h"
#include "flo_matrix.h"

typedef struct color_map color_map_t;

typedef struct color_map {
    uint16_t (*color_map)( const color_map_t *self, float h);
} color_map_t;

static uint16_t map_grey([[unused]] const color_map_t  *p, float  h)
{
    return flo_hsvToRgb565(0.0f, 0.0f, h);
}


typedef struct hsv_rotate hsv_rotate_t;

typedef struct hsv_rotate {
    color_map_t base;
    float rot_h;
    float s;
    float l;
} hsv_rotate_t;


static uint16_t map_hsv_rotated( const color_map_t *p, float h) {
    assert( p);
    hsv_rotate_t *self = (hsv_rotate_t *) p;
    h = fmodf( h + self->rot_h, 1.0f);
    return flo_hsvToRgb565( h, self->s, self->l);
}

flo_pixmap_t *calculate_texture_line(const flo_matrix_t *mat, const color_map_t *f)
{

    assert(mat);
    assert(f);
    assert(f->color_map);

    const unsigned int height = mat->height;
    const unsigned int width = mat->width;
    flo_pixmap_t *p = flo_pixmap_create(width, height);

#pragma omp parallel for
        for (unsigned int w = 0; w < width; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                    //flo_pixmap_set_pixel(p, w, h, f->color_map(f, 1.0f - flo_matrix_get_value(mat, w, h)));
            flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565( 1.0f - flo_matrix_get_value(mat, w, h), 0.8, 0.8));
        }
    }
    return p;
}
#endif