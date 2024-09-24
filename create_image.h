#ifndef CREATE_IMAGE_H
#define CREATE_IMAGE_H

#include "flo_pixmap.h"
#ifdef __cplusplus
extern "C" {
flo_pixmap_t *create_image( long size,  uint16_t buffer[], int fft_size);
}
#else
flo_pixmap_t *create_image( long size, uint16_t buffer[size], int fft_size);
#endif
#endif