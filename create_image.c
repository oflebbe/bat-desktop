#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <fftw3.h>

#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"

#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
#include "create_image.h"
#include "kiss_fftr.h"


flo_pixmap_t *create_image( long bufsize, uint16_t buffer[bufsize], int fft_size)
{
  kiss_fft_scalar *fft_in = malloc(fft_size * sizeof(kiss_fft_scalar));
  kiss_fft_cpx *fft_out = malloc(fft_size * sizeof(kiss_fft_cpx));
  kiss_fftr_cfg cfg = kiss_fftr_alloc(fft_size, 0, 0, 0);
  
  const float a0 = 25. / 46.;
  float *window = calloc(1,sizeof(float)* fft_size);
  if (!window) {
    free(fft_out);
    free(fft_in);
    return NULL;
  }
  
  for (int i = 0; i < fft_size; i++)
  {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (fft_size - 1));
  }

  const int R = 128;
  
  const int width = bufsize / R;
  const int height = fft_size / 2;

  // start 15kHz
  // 0 - 125 kHz -> 512 Px
  // 15 / 125 * 512

  flo_pixmap_t *pixmap = flo_pixmap_create( width, height);
  bool relevant = false;
  for (int i = 0; i < width; i++)
  {
    long index = ((bufsize - fft_size) * i) / (width - 1);
    for (int j = 0; j < fft_size; j++)
    {
      assert(index + j < bufsize);
      fft_in[j] = (buffer[index + j] - 2048) * window[j];
    }
    kiss_fftr(cfg, fft_in, fft_out);

    int count_more_05 = 0;

    for (int j = 0; j < height; j++)
    {
      const float power =  fft_out[j].r*fft_out[j].r + fft_out[j].i*fft_out[j].i;

      const float ang = (log10f(power) - 4) / (11 - 4);
      const float clamp = fmaxf(0.f, ang);

      if (ang > 0.5)
      {
        count_more_05++;
      }
      uint16_t color = flo_hslToRgb565(clamp, 1.0f, 0.5f);
      flo_pixmap_set_pixel(pixmap, i, height-j-1, color);
    }
    if (count_more_05 > 100)
    {
      relevant = true;
    }
  }
  relevant = true;
  free( fft_in);
  free( fft_out);
  free( cfg);
  free(window);
  return pixmap;
}
