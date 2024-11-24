#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <complex.h>
#include <math.h>

#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"

#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
#include "create_image.h"
#define MEOW_FFT_IMPLEMENTATION
#include "meow_fft.h"

#ifndef M_PI
#define M_PI 3.1415926
#endif

flo_pixmap_t *create_image_meow(long bufsize, uint16_t buffer[], long start, int width_px, int fft_size, float overlap_percent)
{
  if (start < 0 || width_px < 0)
  {
    return NULL;
  }
  const int height = fft_size / 2;
  const float a0 = 25. / 46.;
  float *window = calloc(1, sizeof(float) * fft_size);
  if (!window)
  {
    return NULL;
  }
  for (int i = 0; i < fft_size; i++)
  {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (fft_size - 1));
  }

  flo_pixmap_t *pixmap = flo_pixmap_create(width_px, height);
  #pragma omp parallel
  {
    float *fft_in = malloc(fft_size * sizeof(float));
    Meow_FFT_Complex *fft_out = malloc((fft_size / 2 + 1) * sizeof(Meow_FFT_Complex));
    size_t workset_bytes = meow_fft_generate_workset_real(fft_size, NULL);
    Meow_FFT_Workset_Real *fft_real =
        (Meow_FFT_Workset_Real *)malloc(workset_bytes);
    meow_fft_generate_workset_real(fft_size, fft_real);

    if (!fft_out || !fft_in)
    {
      free(fft_out);
      free(fft_in);
    }

    const int off = fft_size * (1.f - overlap_percent);

    size_t end = width_px * off + start;
    size_t fft_end = end + fft_size;
    if (fft_end > bufsize)
    {
      fft_end = bufsize - 1;
      end = fft_end - fft_size;
    }

    // start 15kHz
    // 0 - 125 kHz -> 512 Px
    // 15 / 125 * 512

#pragma omp for
    for (int col = 0; col < width_px; col++)
    {
      int i = col * off + start;
      for (int j = 0; j < fft_size; j++)
      {
        assert(i + j < bufsize);
        fft_in[j] = (buffer[i + j] - 2048) * window[j];
      }
      meow_fft_real(fft_real, fft_in, fft_out);

      int count_more_05 = 0;

      for (int j = 0; j < height; j++)
      {
        const float power = fft_out[j].r * fft_out[j].r + fft_out[j].j * fft_out[j].j;
        const float ang = (log10f(power) - 4) / (11 - 4);

        if (ang < 0)
        {
          flo_pixmap_set_pixel(pixmap, col, height - j - 1, 0x0000);
        }
        else
        {
          uint16_t color = flo_hslToRgb565(1.f - ang, 1.0f, 0.5f);
          flo_pixmap_set_pixel(pixmap, col, height - j - 1, color);
        }
      }
    }

    free(fft_in);
    free(fft_out);
    free(fft_real);
  }
  free(window);
  return pixmap;
}
