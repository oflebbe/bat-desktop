#ifndef CREATE_IMAGE_H
#define CREATE_IMAGE_H

#include "flo_matrix.h"

typedef struct
{
    flo_matrix_t *channel;
} mono_result_t;


typedef struct
{
    flo_matrix_t *left;
    flo_matrix_t *right;
    flo_matrix_t *correlation;
} stereo_result_t;



const mono_result_t create_image_meow(long bufsize, const uint16_t buffer[bufsize], int scale, int offset, int fft_size, float overlap_percent);
const stereo_result_t create_stereo_image_meow(long bufsize, const uint16_t buffer[bufsize], int fft_size, float overlap_percent);

#endif