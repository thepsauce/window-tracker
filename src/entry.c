#include "entry.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct entry_list Entries;

bool search_entry(const char *title, size_t *pIndex)
{
    size_t l, r;

    l = 0;
    r = Entries.n;
    while (l < r) {
        const size_t m = (l + r) / 2;
        const int cmp = strcmp(Entries.p[m].title, title);
        if (cmp == 0) {
            if (pIndex != NULL) {
                *pIndex = m;
            }
            return true;
        }
        if (cmp < 0) {
            l = m + 1;
        } else {
            r = m;
        }
    }
    if (pIndex != NULL) {
        *pIndex = r;
    }
    return false;
}

void add_entry(const char *title, struct timespec t1, struct timespec t2)
{
    struct entry *p = NULL;
    struct timespec td;
    size_t index;

    td = sub_timespec(t2, t1);

    if (Entries.n == 0) {
        Entries.first = t1;
        Entries.spent = td;
        Entries.last = t2;
    } else {
        if (cmp_timespec(Entries.first, t1) > 0) {
            Entries.first = t1;
        }
        Entries.spent = add_timespec(Entries.spent, td);
        if (cmp_timespec(Entries.last, t2) < 0) {
            Entries.last = t2;
        }
    }

    if (search_entry(title, &index)) {
        p = &Entries.p[index];
        if (cmp_timespec(p->first, t1) > 0) {
            p->first = t1;
        }
        p->spent = add_timespec(p->spent, td);
        if (cmp_timespec(p->last, t2) < 0) {
            p->last = t2;
        }
        return;
    }

    if (Entries.n >= Entries.a) {
        Entries.a *= 2;
        Entries.a++;
        Entries.p = realloc(Entries.p, sizeof(*Entries.p) * Entries.a);
        assert(Entries.p != NULL);
    }

    p = &Entries.p[index];

    memmove(p + 1, p, sizeof(*p) * (Entries.n - index));
    Entries.n++;

    p->title = strdup(title);
    assert(p->title != NULL);
    p->first = t1;
    p->spent = td;
    p->last = t2;
}

static int compare_spent_time(const void *a, const void *b)
{
    const struct entry *const p1 = a;
    const struct entry *const p2 = b;
    return cmp_timespec(p1->spent, p2->spent);
}

void entry_status(FILE *fp)
{
    fprintf(fp, "%zu entries\n", Entries.n);
    if (Entries.n == 0) {
        return;
    }
    qsort(Entries.p, Entries.n, sizeof(*Entries.p), compare_spent_time);
    for (size_t i = 0; i < Entries.n; i++) {
        struct entry *const p = &Entries.p[i];
        fprintf(fp, "%s: %ld hours, %ld minutes, %ld seconds\n",
                p->title, p->spent.tv_sec / 3600, (p->spent.tv_sec / 60) % 60, p->spent.tv_sec % 60);
    }
    fprintf(fp, "\nfirst: %s", ctime(&Entries.first.tv_sec));
    fprintf(fp, "total spent: %ld hours, %ld minutes, %ld seconds\n",
            Entries.spent.tv_sec / 3600, (Entries.spent.tv_sec / 60) % 60, Entries.spent.tv_sec % 60);
    fprintf(fp, "last: %s", ctime(&Entries.last.tv_sec));

    struct timespec t1 = sub_timespec(Entries.last, Entries.first);
    struct timespec days = ldouble_to_timespec(timespec_to_days(t1));
    struct timespec avg = div_timespec(Entries.spent, days);
    double perc = 100 * timespec_to_ldouble(avg) / NANOS_PER_SECOND / SECONDS_PER_DAY;
    fprintf(fp, "\nspent an average of %ld hours, %ld minutes, %ld seconds per day (%%%lf of the day)\n",
            avg.tv_sec / 3600, (avg.tv_sec / 60) % 60, avg.tv_sec % 60, perc);

}
