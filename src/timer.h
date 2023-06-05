#ifndef TIMER_H
#define TIMER_H

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 199309L
#error "must define _POSIX_C_SOURCE >= 199309L"
#endif

#include <time.h>

typedef struct {
    struct timespec start_time;
    struct timespec stop_time;
} Timer;

enum { TIMER_NS, TIMER_US, TIMER_MS, TIMER_S };

void timer_start(Timer * timer);
void timer_stop(Timer * timer);
long timer_get_elapsed(Timer * timer, int unit);

#define timer_get_elapsed_ns(timer) timer_get_elapsed(timer, TIMER_NS)
#define timer_get_elapsed_us(timer) timer_get_elapsed(timer, TIMER_US)
#define timer_get_elapsed_ms(timer) timer_get_elapsed(timer, TIMER_MS)
#define timer_get_elapsed_s(timer)  timer_get_elapsed(timer, TIMER_S)

#endif /* TIMER_H */
