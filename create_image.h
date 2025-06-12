#ifndef CREATE_IMAGE_H
#define CREATE_IMAGE_H

#include "flo_matrix.h"

// mono result will have right and correlation null
typedef struct
{
    const flo_matrix_t *left;
    const flo_matrix_t *right;
    const flo_matrix_t *correlation;
} stereo_result_t;

stereo_result_t create_image_meow(unsigned long bufsize, const uint16_t buffer[bufsize], unsigned int scale, unsigned int offset, unsigned int fft_size, float overlap_percent, bool stereo);
void stereo_result_release(stereo_result_t r);

#endif

