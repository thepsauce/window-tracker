#include "extime.h"

int cmp_timespec(struct timespec t1, struct timespec t2)
{
    if (t2.tv_sec > t1.tv_sec) {
        return -1;
    }
    if (t2.tv_sec < t1.tv_sec) {
        return 1;
    }
    return t2.tv_nsec > t1.tv_nsec ? -1 : t2.tv_nsec < t1.tv_nsec ? 1 : 0;
}

struct timespec add_timespec(struct timespec t1, struct timespec t2)
{
    t1.tv_sec += t2.tv_sec;
    t1.tv_nsec += t2.tv_nsec;
    if (t1.tv_nsec > NANOS_PER_SECOND) {
        t1.tv_sec++;
        t1.tv_nsec -= NANOS_PER_SECOND;
    }
    return t1;
}

struct timespec sub_timespec(struct timespec t1, struct timespec t2)
{
    t1.tv_sec -= t2.tv_sec;
    t1.tv_nsec -= t2.tv_nsec;
    if (t1.tv_nsec < 0) {
        t1.tv_sec--;
        t1.tv_nsec += NANOS_PER_SECOND;
    }
    return t1;
}

struct timespec div_timespec(struct timespec t1, struct timespec t2)
{
    const long double d = timespec_to_ldouble(t1) / timespec_to_ldouble(t2);
    return ldouble_to_timespec(d);
}

long double timespec_to_ldouble(struct timespec t)
{
    return t.tv_sec * NANOS_PER_SECOND + t.tv_nsec;
}

struct timespec ldouble_to_timespec(long double d)
{
    struct timespec t;
    t.tv_sec = d;
    t.tv_nsec = (d - t.tv_sec) * NANOS_PER_SECOND;
    return t;
}

long double timespec_to_days(struct timespec t)
{
    const long double s = (long double) t.tv_sec + (long double) t.tv_nsec / NANOS_PER_SECOND;
    return s / SECONDS_PER_DAY;
}
