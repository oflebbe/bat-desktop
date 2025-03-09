#ifndef FLO_TIME_H
#define FLO_TIME_H

#ifndef WIN32
#include <time.h>
typedef struct timespec flo_time_t;
#else
#include <windows.h>
typedef LARGE_INTEGER flo_time_t;
#endif

flo_time_t flo_get_time(void);
double flo_end_time(flo_time_t start);

#define FLO_TIME_IMPLEMENTATION
#ifdef FLO_TIME_IMPLEMENTATION

#ifndef WIN32
flo_time_t flo_get_time(void)
{
    flo_time_t start;
    clock_gettime(CLOCK_REALTIME, &start);
    return start;
}

double flo_end_time(flo_time_t start)
{
    flo_time_t end = flo_get_time();
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1.0e9;
}
#else


flo_time_t flo_get_time(void)
{
    flo_time_t start;
    QueryPerformanceCounter(&start);
    return start;
}

double flo_end_time(flo_time_t start)
{
    flo_time_t end = flo_get_time();
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    return (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
}

#endif
#endif
#endif