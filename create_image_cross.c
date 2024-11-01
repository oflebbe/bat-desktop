#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "flo_pixmap.h"

const int WIDTH = 120;

// at pos indices -width-delay until width+delay are valid!
float cross_correlation(int length, const uint16_t left[], const uint16_t right[], int pos, int delay)
{
    long sum = 0.0;
    for (int i = pos - WIDTH + 1; i < pos + WIDTH; i++)
    {
        sum += ((long)left[i] - 2048.) * ((long)right[i + delay] - 2048);
    }
    float res = sum;
    res /= (2 * WIDTH + 1) * (2048 * 2048);
    return res; // / (2*WIDTH-1);
}

struct pos
{
    float c;
    int delay;
};

// Function to estimate the source position using cross-correlation
struct pos estimate_source_position(int length, const uint16_t left[], const uint16_t right[], int pos)
{
    struct pos best = {.c = -1, .delay = 0};

    // Check positive and negative delays within the max delay range
    for (int delay = -WIDTH; delay <= WIDTH; delay++)
    {
        float correlation = cross_correlation(length, left, right, pos, delay);

        // Keep track of the delay with the highest correlation
        if (correlation > best.c)
        {
            best.c = correlation;
            best.delay = delay;
        }
    }

    return best;
}

flo_pixmap_t *create_image_cross(int length, const uint16_t left[], const uint16_t right[], int fft_size, float overlap_percent)
{
    const int off = fft_size * (1.f - overlap_percent);
    const int width = (length - fft_size) / off;

    // start 15kHz
    // 0 - 125 kHz -> 512 Px
    // 15 / 125 * 512

    flo_pixmap_t *pixmap = flo_pixmap_create(width, 2 * WIDTH + 1);
    memset(pixmap->buf, 0xff, pixmap->len * 2);

    for (int col = 0; col < width; col++)
    {
#define NN 5
        if (col % NN == 0)
        {
            continue;
        }
        int i = col * off;
        if (i < 2 * WIDTH)
            continue;
        if (i > length - 2 * WIDTH)
            continue;

        struct pos best = estimate_source_position(length, left, right, i);
        if (best.c > 0.4)
        {
            for (int i = 0; i < NN; i++)
            {
                flo_pixmap_set_pixel(pixmap, col + i, best.delay + WIDTH, BLACK);
            }
        }
    }
    return pixmap;
}
