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

#define FLO_MATRIX_IMPLEMENTATION
#include "flo_matrix.h"
#define FLO_PIXMAP_IMPLEMENTATION
#include "flo_pixmap.h"
#define FLO_TIME_IMPLEMENTATION
#include "flo_time.h"
#define FLO_FILE_IMPLEMENTATION
#include "flo_file.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

    int num_tex_line = matrix->width / TEXTURE_WIDTH - 1;

    int height = matrix->height;
    int width = matrix->width;

    flo_pixmap_t *p = flo_pixmap_create(TEXTURE_WIDTH, height);
    for (int i = 0; i < num_tex_line; i++)
    {
#pragma omp parallel for
        for (int w = 0; w < TEXTURE_WIDTH; w++)
        {
            for (int h = 0; h < height; h++)
            {
                flo_pixmap_set_pixel(p, w, h, flo_hsvToRgb565(1.0f - flo_matrix_get_value(matrix, i * TEXTURE_WIDTH + w, h), sat, lit));
            }
        }
    }

    free(p);
}

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
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
    size_t size;
    const uint16_t *raw_file = (uint16_t *)flo_mapfile(fp, &size);
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

    flo_time_t start_stereo_time = flo_get_time();
    stereo_result_t stereo = create_stereo_image_meow(size, raw_file, 512, 0.1);
    calculate_texture(stereo.left, 0.9, 0.9);
    calculate_texture(stereo.right, 0.9, 0.9);
    const double diff_stereo = flo_end_time(start_stereo_time);
    printf("stereo: %g\n", diff_stereo);

    free(stereo.left);
    free(stereo.right);
    free(stereo.correlation);

    // stbi_write_png("output_l.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_l->buf, 0);
    // stbi_write_png("output_r.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_r->buf, 0);
}
