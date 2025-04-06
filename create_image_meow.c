#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <complex.h>
#include <math.h>

#include "flo_file.h"
#include "flo_matrix.h"
#include "create_image.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#define MEOW_FFT_IMPLEMENTATION
#include "meow_fft.h"
#pragma GCC diagnostic pop




mono_result_t create_image_meow(unsigned long bufsize, const uint16_t buffer[bufsize], unsigned int scale, unsigned int offset, unsigned int fft_size, float overlap_percent)
{
  if (offset >= scale)
  {
    return (mono_result_t){0};
  }
  const unsigned int height = fft_size / 2;
  const float a0 = 25.f / 46.f;
  float *window = calloc(1, sizeof(float) * fft_size);
  if (!window)
  {
    return (mono_result_t){0};
  }
  for (unsigned int i = 0; i < fft_size; i++)
  {
    window[i] = a0 - (1.0f - a0) * cosf((float) M_2_PI * (float) i / ((float) fft_size - 1.0f));
  }

  // last valid index
  const unsigned long end = bufsize / scale - fft_size;
  const unsigned int off = (float) fft_size * (1.f - overlap_percent);
  const unsigned int width_px = (end / off / 2) * 2;

  flo_matrix_t *matrix = flo_matrix_create(width_px, height);
#pragma omp parallel
  {
    float *fft_in = malloc(fft_size * sizeof(float));
    Meow_FFT_Complex *fft_out = malloc((fft_size / 2 + 1) * sizeof(Meow_FFT_Complex));

    size_t workset_bytes = meow_fft_generate_workset_real((int)fft_size, NULL);
    Meow_FFT_Workset_Real *fft_real =
        (Meow_FFT_Workset_Real *)malloc(workset_bytes);
    meow_fft_generate_workset_real((int) fft_size, fft_real);

    // start 15kHz
    // 0 - 125 kHz -> 512 Px
    // 15 / 125 * 512

#pragma omp for
    for (unsigned int col = 0; col < width_px; col++)
    {
      unsigned int i = (col * off);
      for (unsigned int j = 0; j < fft_size; j++)
      {
        const unsigned int index = scale * (i + j) + offset;
        assert(index < bufsize);
        // subtract 2048 for 12 bit sampling i.e. from 0 .. 4096
        fft_in[j] = (buffer[index] - 2048) * window[j];
      }
      meow_fft_real(fft_real, fft_in, fft_out);

      for (unsigned int j = 0; j < height; j++)
      {
        const float power = fft_out[j].r * fft_out[j].r + fft_out[j].j * fft_out[j].j;
        float ang = (log10f(power) - 4) / (11 - 4);

        if (ang >= 0)
        {
          if (ang > 1.0)
          {
            ang = 1.0f;
          }
          flo_matrix_set_value(matrix, col, height - j - 1, ang);
        }
      }
    }

    free(fft_in);
    free(fft_out);
    free(fft_real);
  }
  free(window);
  return (mono_result_t){.channel = matrix};
}

stereo_result_t create_stereo_image_meow(unsigned long bufsize, const uint16_t buffer[bufsize], unsigned int fft_size, float overlap_percent)
{
  assert( bufsize > 0);
  assert( overlap_percent >= 0.0f && overlap_percent < 1.0f);
  assert( fft_size > 0);
  const unsigned int scale = 2;
  const unsigned int height = fft_size / 2;
  const float a0 = 25.f / 46.f;

  float *window = calloc(1, sizeof(float) * fft_size);
  if (!window)
  {
    return (stereo_result_t){0};
  }
  for (int i = 0; i < fft_size; i++)
  {
    window[i] = a0 - (1.0f - a0) * cosf( M_2_PI *  i / (fft_size - 1.0f));
  }

  // last valid index
  const unsigned long end = bufsize / scale - fft_size;
  // step size
  const unsigned int off = (unsigned int) ((float) fft_size * (1.f - overlap_percent));
  const unsigned int width_px = (end / off / 2) * 2;

  flo_matrix_t *matrix_left = flo_matrix_create(width_px, height);
  flo_matrix_t *matrix_right = flo_matrix_create(width_px, height);
  flo_matrix_t *matrix_correlation = flo_matrix_create(width_px, height);
  stereo_result_t result = {.left = matrix_left, .right = matrix_right, .correlation = matrix_correlation};

#pragma omp parallel
  {
    float *fft_in_left = calloc(fft_size, sizeof(float));
    float *fft_in_right = calloc(fft_size, sizeof(float));
    float *fft_out_correlation = calloc(fft_size, sizeof(float));
    Meow_FFT_Complex *fft_out_left = malloc((fft_size / 2 + 1) * sizeof(Meow_FFT_Complex));
    assert( fft_out_left);
    Meow_FFT_Complex *fft_out_right = malloc((fft_size / 2 + 1) * sizeof(Meow_FFT_Complex));
    assert( fft_out_left);

    size_t workset_bytes = meow_fft_generate_workset_real((int) fft_size, NULL);
    Meow_FFT_Workset_Real *fft_real =
        (Meow_FFT_Workset_Real *)malloc(workset_bytes);
    assert(fft_real);
    meow_fft_generate_workset_real((int) fft_size, fft_real);

    // start 15kHz
    // 0 - 125 kHz -> 512 Px
    // 15 / 125 * 512

#pragma omp for
    for (unsigned int col = 0; col < width_px; col++)
    {
      unsigned int i = (col * off);
      for (unsigned int j = 0; j < fft_size; j++)
      {
        const unsigned int index = scale * (i + j);
        assert(index < bufsize - 1);
        // subtract 2048 for 12 bit sampling i.e. from 0 .. 4096
        fft_in_left[j] = (buffer[index] - 2048) * window[j];
        fft_in_right[j] = (buffer[index + 1] - 2048) * window[j];
      }
      meow_fft_real(fft_real, fft_in_left, fft_out_left);
      meow_fft_real(fft_real, fft_in_right, fft_out_right);

      for (unsigned int j = 0; j < height; j++)
      {
        const float power = fft_out_left[j].r * fft_out_left[j].r + fft_out_left[j].j * fft_out_left[j].j;
        float ang = (log10f(power) - 4) / (11 - 4);

        if (ang >= 0)
        {
          if (ang > 1.0)
          {
            ang = 1.0f;
          }
          flo_matrix_set_value(matrix_left, col, height - j - 1, ang);
        }
      }
#pragma omp simd
      for (unsigned int j = 0; j < height; j++)
      {
        const float power = fft_out_right[j].r * fft_out_right[j].r + fft_out_right[j].j * fft_out_right[j].j;
        float ang = (log10f(power) - 4) / (11 - 4);

        if (ang >= 0)
        {
          if (ang > 1.0)
          {
            ang = 1.0f;
          }
          flo_matrix_set_value(matrix_right, col, height - j - 1, ang);
        }
      }
#pragma omp simd
      for (unsigned int j = 0; j < height; j++)
      {
        fft_out_left[j] = meow_mul_by_conjugate(fft_out_left[j], fft_out_right[j]);
      }

      meow_fft_real_i(fft_real, fft_out_left, fft_out_right, fft_out_correlation);
      for (unsigned int j = 0; j < height; j++)
      {
        float ang = fft_out_correlation[j] / (398504800000.0f * 0.005f);

        if (ang >= 0)
        {
          if (ang > 1.0)
          {
            ang = 1.0f;
          }
          flo_matrix_set_value(matrix_correlation, col, height - j - 1, ang);
        }
      }
    }

    free(fft_in_left);
    free(fft_in_right);
    free(fft_out_left);
    free(fft_out_right);
    free(fft_out_correlation);

    free(fft_real);
  }
  free(window);
  return result;
}
