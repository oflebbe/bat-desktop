#include <cassert>
#include <cmath>
#include <complex>
#include <fftw3.h>
#include <fstream>
#include <fmt/core.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include <QImage>
#include <QColor>
#include <vector>
#include <algorithm>

#include "classify.h"
#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"

Classifier::Classifier(int SIZE)
{
 // fftwf_init_threads();
  // fftwf_plan_with_nthreads(2);
  this->SIZE = SIZE;

  in = reinterpret_cast<float *>(fftwf_malloc(sizeof(float) * SIZE));
  out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (SIZE / 2 + 1));
  plan = fftwf_plan_dft_r2c_1d(SIZE, in, out, FFTW_ESTIMATE);

  window = std::vector<float>(SIZE);
  const float a0 = 25. / 46.;
  for (int i = 0; i < SIZE; i++)
  {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (SIZE - 1));
  }
}


void Classifier::open(const std::string &filename)
{
  relevant = false;
  fmt::print(" {}\n", filename);
  std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
  if (!ifs)
  {
    fmt::print("could not open {}\n", filename);
    return;
  }
  std::ifstream::pos_type pos = ifs.tellg();

  if (pos == 0)
  {
    fmt::print("empty file\n");
    return;
  }
  if (pos % 2 == 1)
  {
    fmt::print("invald file\n");
    return;
  }

  std::vector<uint16_t> filebuf(pos);

  ifs.seekg(0, std::ios::beg);
  ifs.read(reinterpret_cast<char *>(filebuf.data()), pos);
  ifs.close();

  const int R = 128;
  const int len = pos / 2;
  //
  
  const int width = len / R;
  const int height = SIZE / 2;
  float min = 100;
  float max = 0;

  // start 15kHz
  // 0 - 125 kHz -> 512 Px
  // 15 / 125 * 512

  flo_pixmap_t *pixmap = flo_pixmap_create( width, height);
  relevant = false;
  for (int i = 0; i < width; i++)
  {
    int index = ((len - SIZE) * i) / (width - 1);
    for (int j = 0; j < SIZE; j++)
    {
      assert(index + j < len);
      in[j] = (filebuf[index + j] - 2048) * window[j];
    }
    fftwf_execute(plan);

    int count_more_05 = 0;

    for (int j = 0; j < height; j++)
    {
      const std::complex<float> *outf =
          reinterpret_cast<std::complex<float> *>(out[j]);
      float p = abs(*outf);
      const float z = log10(p);

      float ang = (z + 0.4) / (6 + 0.4);
      ang = std::max(0.f, ang);

      if (z > max)
      {
        max = z;
      }
      if (z < min)
      {
        min = z;
      }
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
  qimage =  QImage( (uchar *) pixmap->buf, width, height, QImage::Format_RGB16, free, pixmap);
}

Classifier::~Classifier() {
  fftwf_free(in);
  fftwf_free(out);
  fftwf_destroy_plan(plan);
}