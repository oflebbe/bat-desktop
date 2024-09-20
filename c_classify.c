#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <fftw3.h>

#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"
#include "c_classify.h"

flo_pixmap_t *get_image( const char filename[static 1], int size)
{
  FILE *fin = fopen( filename, "rb");
  if (!fin) {
    return NULL;
  }
  int pos = fseek( fin, 0, SEEK_END);
  if (pos < 0) {
    return NULL;
  }
  fseek(fin, 0, SEEK_SET);
  if (pos % 2 == 1) {
    return NULL;
  }
  uint16_t *filebuf = calloc( 1, pos);
  if (!filebuf) {
    return NULL;
  }
  if (pos != fread( fin, 1, pos, fin)) {
    free( filebuf);
    return NULL;
  }
  fclose( fin);

  float *in = fftwf_malloc(sizeof(float) * size);
  if (!in) {
    free( filebuf);
    return NULL;
  }
  fftwf_complex *out = fftwf_malloc(sizeof(fftwf_complex) * (size / 2 + 1));
  if (!out) {
    free(in);
    free( filebuf);
    return NULL;
  }

  fftwf_plan plan = fftwf_plan_dft_r2c_1d(size, in, out, FFTW_ESTIMATE);
  
  const float a0 = 25. / 46.;
  float *window = calloc(1,sizeof(float)* size);
  if (!window) {
    free(out);
    free(in);
    free( filebuf);
    return NULL;
  }
  
  for (int i = 0; i < size; i++)
  {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (size - 1));
  }

  const int R = 128;
  const int len = pos / 2;
  
  const int width = len / R;
  const int height = size / 2;
  float min = 100;
  float max = 0;

  // start 15kHz
  // 0 - 125 kHz -> 512 Px
  // 15 / 125 * 512

  flo_pixmap_t *pixmap = flo_pixmap_create( width, height);
  bool relevant = false;
  for (int i = 0; i < width; i++)
  {
    int index = ((len - size) * i) / (width - 1);
    for (int j = 0; j < size; j++)
    {
      assert(index + j < len);
      in[j] = (filebuf[index + j] - 2048) * window[j];
    }
    fftwf_execute(plan);

    int count_more_05 = 0;

    for (int j = 0; j < height; j++)
    {
      const float p = cabsf( out[j]);
      const float z = log10f(p);

      const float ang = (z + 0.4) / (6 + 0.4);
      const float clamp = fmaxf(0.f, ang);

      if (ang > 0.5)
      {
        count_more_05++;
      }
      uint16_t color = flo_hslToRgb565(ang, 1.0f, 0.5f);
      flo_pixmap_set_pixel(pixmap, i, height-j-1, color);
    }
    if (count_more_05 > 100)
    {
      relevant = true;
    }
  }
  relevant = true;
  free( filebuf);
  fftwf_free( in);
  fftwf_free( out);
  fftwf_destroy_plan(plan);
  return pixmap;
}
