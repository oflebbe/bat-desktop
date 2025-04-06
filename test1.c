#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <omp.h>
// #include <pthread.h>

#define FLO_TIME_IMPLEMENTATION
#include "flo_time.h"
#define FLO_MATRIX_IMPLEMENTATION
#include "flo_matrix.h"
#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"
#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"

#include "create_image.h"

#define STEREO 1

typedef struct results
{
    flo_matrix_t *matrix_l;
    flo_matrix_t *matrix_r;
} result_t;

typedef struct image_params
{
    long size;
    const uint16_t *raw_file;
    int fft_size;
    int offset;
    int scale;
    float overlap_percent;
} image_params_t;

#define TEXTURE_WIDTH 4096

void calculate_texture(const flo_matrix_t *matrix, float sat, float lit)
{

    unsigned int num_tex_line = matrix->width / TEXTURE_WIDTH - 1;

    unsigned int height = matrix->height;
    // unsigned int width = matrix->width;

    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (unsigned int i = 0; i < num_tex_line; i++)
    {
#pragma omp parallel for
        for (unsigned int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (unsigned int h = 0; h < height; h++)
            {
                flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565(1.0f - flo_matrix_get_value(matrix, i * TEXTURE_WIDTH + w, h), sat, lit));
            }
        }
    }
    free(p);
}

uint16_t *raw_file = NULL;
size_t size = 0;

void benchmark_meow(flo_timer_t *timer, unsigned int n)
{
    stereo_result_t stereo = {};
    for (int i = 0; i < n; i++)
    {
        if (stereo.left)
        {
            free(stereo.left);
            free(stereo.right);
            free(stereo.correlation);
        }
        stereo = create_stereo_image_meow(size, raw_file, 512.0f, 0.1f);
    }

    free(stereo.left);
    free(stereo.right);
    free(stereo.correlation);
}

void benchmark_texture(flo_timer_t *timer, unsigned int n)
{
    const stereo_result_t stereo = create_stereo_image_meow(size, raw_file, 512, 0.1f);
    // reset timer
    flo_start_timer(timer);
    for (int i = 0; i < n; i++)
    {
        calculate_texture(stereo.left, 0.9f, 0.9f);
        calculate_texture(stereo.right, 0.9f, 0.9f);
    }
    free(stereo.left);
    free(stereo.right);
    free(stereo.correlation);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Need 1 filename argument\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp)
    {
        fprintf(stderr, "could not open %s\n", argv[1]);
        exit(1);
    }
    // size_t size;
    /* const uint16_t */ raw_file = (uint16_t *)flo_mapfile(fp, &size);
    size /= 20;
    fclose(fp);
    if (!raw_file)
    {
        fprintf(stderr, "could not map %s\n", argv[1]);
        exit(1);
    }

    size /= 2;
    if (size % 2 == 1)
    {
        raw_file = NULL;
        fprintf(stderr, "could not map %s\n", argv[1]);
        exit(1);
    }

    flo_benchmark(benchmark_meow, "meow", 6);
    flo_benchmark(benchmark_texture, "texture", 6);

    // stbi_write_png("output_l.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_l->buf, 0);
    // stbi_write_png("output_r.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_r->buf, 0);
}
