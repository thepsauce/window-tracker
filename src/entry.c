#include "entry.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct entry_list Entries;

static int null_strcmp(const char *s1, const char *s2)
{
    if (s1 == NULL) {
        return s2 == NULL ? 0 : -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    return strcmp(s1, s2);
}

static int compare_entry_strings(const struct entry *e1, const struct entry *e2)
{
    int cmp = null_strcmp(e1->name, e2->name);
    if (cmp == 0) {
        cmp = null_strcmp(e1->instance, e2->instance);
        if (cmp == 0) {
            cmp = null_strcmp(e1->title, e2->title);
        }
    }
    return cmp;
}

bool search_entry(const struct entry *entry, size_t *pIndex)
{
    size_t l, r;

    l = 0;
    r = Entries.n;
    while (l < r) {
        const size_t m = (l + r) / 2;

        const int cmp = compare_entry_strings(&Entries.p[m], entry);
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

struct entry *add_entry(struct entry *entry, struct timespec t1, struct timespec t2)
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

    if (search_entry(entry, &index)) {
        p = &Entries.p[index];
        if (cmp_timespec(p->first, t1) > 0) {
            p->first = t1;
        }
        p->spent = add_timespec(p->spent, td);
        if (cmp_timespec(p->last, t2) < 0) {
            p->last = t2;
        }
        return p;
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

    p->name = entry->name;
    p->instance = entry->instance;
    p->title = entry->title;
    p->first = t1;
    p->spent = td;
    p->last = t2;
    return p;
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

    const struct timespec dt = sub_timespec(Entries.last, Entries.first);
    struct timespec avg = div_timespec(Entries.spent, dt);
    const double perc = 100 * timespec_to_ldouble(avg) / NANOS_PER_SECOND;
    avg = ldouble_to_timespec(SECONDS_PER_DAY * timespec_to_ldouble(avg));
    fprintf(fp, "\nspent an average of %ld hours, %ld minutes, %ld seconds per day (%%%lf of the day)\n",
            avg.tv_sec / 3600, (avg.tv_sec / 60) % 60, avg.tv_sec % 60, perc);

}
