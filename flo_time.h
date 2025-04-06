#ifndef FLO_TIME_H
#define FLO_TIME_H

typedef struct flo_timer flo_timer_t;

flo_timer_t *flo_alloc_timer();

// start or reset the timer
void flo_start_timer(flo_timer_t *timer);

// start or reset the timer
void flo_continue_timer(flo_timer_t *timer);

// stop the timer and accumulate
double flo_stop_timer(flo_timer_t *timer);

void flo_benchmark(void (*b)(flo_timer_t *timer, unsigned int n), char str[static 1], unsigned int count);

#ifdef FLO_TIME_IMPLEMENTATION

#include <stdio.h>
#include <assert.h>

#ifndef WIN32
#include <time.h>
typedef struct flo_timer
{
    double accumulated;
    struct timespec current;
} flo_timer_t;

flo_timer_t *flo_alloc_timer()
{
    flo_timer_t *timer = calloc(1, sizeof(flo_timer_t));
    assert(timer);
    return timer;
}

void flo_release_timer(flo_timer_t *timer)
{
    assert(timer);
    free(timer);
}

void flo_start_timer(flo_timer_t *timer)
{
    assert(timer);
    timer->accumulated = 0.0;
    // simply forget whatever was current
    clock_gettime(CLOCK_REALTIME, &timer->current);
}

void flo_continue_timer(flo_timer_t *timer)
{
    assert(timer);
    // simply forget whatever was current
    clock_gettime(CLOCK_REALTIME, &timer->current);
}

double flo_stop_timer(flo_timer_t *timer)
{
    assert(timer);
    struct timespec end = {0};
    clock_gettime(CLOCK_REALTIME, &end);
    timer->accumulated += (end.tv_sec - timer->current.tv_sec) + 1e-9 * (end.tv_nsec - timer->current.tv_nsec);
    timer->current = (struct timespec){.tv_sec = 0, .tv_nsec = 0};
    return timer->accumulated;
}
#else
#include <windows.h>
typedef struct
{
    double acummulated;
    struct LARGE_INTEGER current;
} flo_timer_t;

flo_time_t flo_start_timer(void)
{
    QueryPerformanceCounter(&timer->current);
}

double flo_stop_timer(flo_time_t *timer)
{
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    timer->accumulate += (double)(end.QuadPart - current.QuadPart) / (double)frequency.QuadPart;
    timer->current.QuadPart = 0;
    return timer->accumulate;
}

#endif

void flo_benchmark(void (*b)(flo_timer_t *timer, unsigned int n), char str[static 1], unsigned int count)
{
    assert(b);
    for (unsigned int c = 0; c < count; c++)
    {
        unsigned int n = 1;
        double diff = 0.0;
        flo_timer_t *timer = flo_alloc_timer();
        do
        {
            flo_start_timer(timer);
            b(timer, n);
            diff = flo_stop_timer(timer);
            if (diff >= 1.0)
            {
                break;
            }
            if (diff < 1e-6)
            {
                n *= 1000;
            }
            else
            {
                n = (unsigned int)((double)n / diff) + 1;
            }
        } while (true);
        free(timer);
        printf("Benchmark-%s %d %9f ns/op\n", str, n, diff * 1.0e9 / n);
    }
}

#endif
#endif
