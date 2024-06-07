#include <complex.h>
#include <fftw3.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <format>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


float hue2rgb(float p, float q, float t) {
  if (t < 0)
    t += 1;
  if (t > 1)
    t -= 1;
  if (t < (1.0 / 6))
    return p + (q - p) * 6 * t;
  if (t < (1. / 2.))
    return q;
  if (t < (2. / 3.))
    return p + (q - p) * ((2. / 3.) - t) * 6;
  return p;
}

typedef struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_t;

color_t hsl_to_color(float h, float s, float l) {
  float r, g, b;

  if (s == 0) {
    r = g = b = l; // achromatic
  } else {

    float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;

    r = hue2rgb(p, q, h + 1 / 3.);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1 / 3.);
  }

  color_t c = (color_t) { .r = (uint8_t) (r * 255), .g = (uint8_t) (g * 255), .b = (uint8_t) (b * 255)};
  return c;
}
  

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("usage: filename\n");
    exit(1);
  }
  FILE *fin = fopen(argv[1], "rb");
  if (fin == NULL) {
    printf("could not open %s\n", argv[1]);
    exit(1);
  }

  if (0 != fseek(fin, 0, SEEK_END)) {
    printf("error seeking\n");
    exit(1);
  }
  const long size = ftell(fin);
  if (0 != fseek(fin, 0, SEEK_SET)) {
    printf("error seeking\n");
    exit(1);
  }

  if (size % 2 != 0) {
    printf("invalid file size %ld\n", size);
    exit(1);
  }

  const int len = size / 2;

  uint16_t buf[len];
  const size_t tmp_len = fread(buf, 2, len, fin);
  if (tmp_len != len) {
    printf("read error %zu\n", tmp_len);
    exit(1);
  }
/*
  for (int i = 0; i < len; i++) {
    printf("%d\n",buf[i]-2048);
  }
*/
  // assuming we are on little endian machine

  const int SIZE = 512;
  const float a0 = 25. / 46.;
  const int R = 2;

  float window[SIZE];
  for (int i = 0; i < SIZE; i++) {
    window[i] = a0 - (1.0 - a0) * cosf(2 * M_PI * i / (SIZE-1));
  }
  float *in = (float *) fftwf_malloc(sizeof(float) * SIZE);
  fftwf_complex *out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * (SIZE / 2 + 1));
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(SIZE, in, out, FFTW_ESTIMATE);

  const int width = len / R;
  const int height = SIZE / 2;
  uint8_t *image = (uint8_t *) malloc(width * height * 3);
  float min = 100;
  float max = 0;

  // start 15kHz 
  // 0 - 125 kHz -> 512 Px
  // 15 / 125 * 512 
  for (int i = 0; i < width; i++) {
    unsigned int index = ((len - SIZE) * i)  /(width - 1);
    for (int j = 0; j < SIZE; j++) {
      assert(index+j < len);
      auto k = buf.at( 777);
      in[j] = (buf[index + j ]-2048) * window[j];
    }
    fftwf_execute(plan);

    int count_more_05 = 0;
    for (int j = 0; j < height; j++) {
      std::complex<float> *outf = reinterpret_cast<std::complex<float> *>(out[j]);
      float p = abs(*outf);
      const float z = log10f(p) ;
      
      float ang = (z + 0.4) / (6 + 0.4);
      
      color_t c = hsl_to_color( ang, 1.f, ang);

     
      if (z > max) {
        max = z;
      }
      if (z < min) {
        min = z;
      }
      if ( ang > 0.5) {
        count_more_05++;
      }
      
      
      image[(i + (height-j-1) * width) * 3] = c.r;
      image[(i + (height-j-1) * width) * 3 + 1] = c.g;
      image[(i + (height-j-1) * width) * 3 + 2] = c.b;
    }

  
    if (count_more_05 > height*0.5) {
   
      std::string file_str = std::format("more75%d.txt", i);
      std::ofstream outfile( file_str);
     
      for (int j = 0; j < SIZE; j++) {
        outfile <<  (buf[index + j ]-2048) * window[j] << "\n";
      }
    }
  }
  printf("%g %g\n", min, max);

  fftwf_free(in);
  fftwf_free(out);
  fftwf_destroy_plan(plan);

  stbi_write_png("output.png", width, height, 3, image, 0);
  free(image);
}
