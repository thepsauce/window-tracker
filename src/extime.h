#ifndef TIMET_H
#define TIMET_H

#include <time.h>
#include <stdint.h>

#define SECONDS_PER_DAY ((time_t) (60*60*24))
#define NANOS_PER_SECOND ((time_t) 1000000000)

int cmp_timespec(struct timespec t1, struct timespec t2);
struct timespec add_timespec(struct timespec t1, struct timespec t2);
struct timespec sub_timespec(struct timespec t1, struct timespec t2);
struct timespec div_timespec(struct timespec t1, struct timespec t2);
long double timespec_to_ldouble(struct timespec t);
struct timespec ldouble_to_timespec(long double d);
long double timespec_to_days(struct timespec t);

#endif
