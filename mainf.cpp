#include <cassert>
#include <cmath>
#include <complex>
#include <fftw3.h>
#include <format>
#include <fstream>
#include <fmt/core.h>
#include <stdint.h>
#include <stdio.h>

#include "mylabel.h"
#include <QApplication>
#include <QImage>
#include <QInputDialog>
#include <vector>


int main(int argc, char *argv[]) {
  if (argc <= 1) {
    fmt::print("usage: filename\n");
    exit(1);
  }

  QApplication app(argc, argv);

  FILE *fin = fopen(argv[1], "rb");
  if (fin == NULL) {
    fmt::print("could not open {}\n", argv[1]);
    exit(1);
  }

  if (0 != fseek(fin, 0, SEEK_END)) {
    fmt::print("error seeking\n");
    exit(1);
  }
  const long size = ftell(fin);
  if (0 != fseek(fin, 0, SEEK_SET)) {
    fmt::print("error seeking\n");
    exit(1);
  }

  if (size % 2 != 0) {
    fmt::print("invalid file size {}\n", size);
    exit(1);
  }

  const int len = size / 2;

  uint16_t buf[len];
  const size_t tmp_len = fread(buf, 2, len, fin);
  if (tmp_len != (size_t)len) {
    fmt::print("read error {}\n", tmp_len);
    exit(1);
  }

  // assuming we are on little endian machine

  const int SIZE = 512;
  const float a0 = 25. / 46.;
  const int R = 128;

  std::vector<float> window(SIZE);
  for (int i = 0; i < SIZE; i++) {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (SIZE - 1));
  }
  float *in = reinterpret_cast<float *>(fftwf_malloc(sizeof(float) * SIZE));
  fftwf_complex *out =
      (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (SIZE / 2 + 1));
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(SIZE, in, out, FFTW_ESTIMATE);

  const int width = len / R;
  const int height = SIZE / 2;
  float min = 100;
  float max = 0;

  // start 15kHz
  // 0 - 125 kHz -> 512 Px
  // 15 / 125 * 512
  QImage image(width, height, QImage::Format_RGB888);
  for (int i = 0; i < width; i++) {
    int index = ((len - SIZE) * i) / (width - 1);
    for (int j = 0; j < SIZE; j++) {
      assert(index + j < len);
      in[j] = (buf[index + j] - 2048) * window[j];
    }
    fftwf_execute(plan);

    int count_more_05 = 0;
    for (int j = 0; j < height; j++) {
      const std::complex<float> *outf =
          reinterpret_cast<std::complex<float> *>(out[j]);
      float p = abs(*outf);
      const float z = log10(p);

      float ang = (z + 0.4) / (6 + 0.4);

      QColor c;
      c.setHslF(ang, 1.f, ang);

      if (z > max) {
        max = z;
      }
      if (z < min) {
        min = z;
      }
      if (ang > 0.5) {
        count_more_05++;
      }
      image.setPixelColor(i, j, c.toRgb());
    }
  }
  
  fftwf_free(in);
  fftwf_free(out);
  fftwf_destroy_plan(plan);

  MyLabel label;

  QPixmap qpix = QPixmap::fromImage(image);
  label.setPixmap(qpix);
  label.show();
  app.exec();
}
