#ifndef CREATE_IMAGE_H
#define CREATE_IMAGE_H

#include "flo_pixmap.h"
#ifdef __cplusplus
extern "C" {
#endif

flo_pixmap_t *create_image_meow( long size, uint16_t buffer[], int fft_size, float overlap_percent);

#ifdef __cplusplus
}
#endif

#endif