#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>

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
    flo_pixmap_t *pixmap_l;
    flo_pixmap_t *pixmap_r;
} result_t;

typedef struct image_params {
    long size;
    const uint16_t *raw_file;
    int fft_size;
    int offset;
    int scale;
    float overlap_percent;
} image_params_t;

void *thread_routine( void *arg ) {
    image_params_t *params = (image_params_t *) arg;
    return (void *) create_image_meow( params->size, params->raw_file, params->scale, params->offset, params->fft_size, params->overlap_percent);
}

// Function to generate a simple image (random colors)
result_t generate_image(long size, const uint16_t raw_file[size], int fft_size, float overlap_percent)
{
    result_t r;

    image_params_t parms1 = {.size = size, .raw_file = raw_file, .fft_size = fft_size, .scale = 2, .offset = 0, .overlap_percent = overlap_percent};
    pthread_t thrd1;
    pthread_create( &thrd1, NULL, thread_routine, &parms1);

    image_params_t parms2 = {.size = size, .raw_file = raw_file, .fft_size = fft_size, .scale = 2, .offset = 1, .overlap_percent = overlap_percent};
    pthread_t thrd2;
    pthread_create( &thrd2, NULL, thread_routine, &parms2);

    pthread_join(thrd1, (void **) &r.pixmap_l);
    pthread_join(thrd2, (void **) &r.pixmap_r);
    return r;
}

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
int main(int argc, char *argv[])
{
    flo_time_t start_time = flo_get_time();

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
    uint16_t *raw_file = (uint16_t *)flo_mapfile(fp, &size);
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

    result_t res = generate_image(size, raw_file, 512, 0.1);
    
    double diff = flo_end_time(start_time);
    printf("%g\n", diff);
   // stbi_write_png("output_l.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_l->buf, 0);
    // stbi_write_png("output_r.png", res.pixmap_l->width, res.pixmap_l->height, 2, res.pixmap_r->buf, 0);
    free(res.pixmap_l);
    free(res.pixmap_r);
}
