#include "entry.h"

#include <assert.h>

int main(void)
{
    struct time t1, t2;

    t1.s = 2800;
    t1.n = 8000;
    t2.s = 2820;
    t2.n = 0;
    add_entry("Hey", t1, t2);

    t1.s = 3800;
    t1.n = 8000;
    t2.s = 4820;
    t2.n = 0;
    add_entry("Yo", t1, t2);

    t1.s = 8100;
    t1.n = 4000;
    t2.s = 9980;
    t2.n = 8234;
    add_entry("Yo", t1, t2);

    size_t index;
    assert(search_entry("Yo", &index));

    struct entry *p = &Entries.p[index];
    printf("%s\n", p->title);
    fprintf(stdout, "spent: %ld hours, %ld minutes, %ld seconds, %ld nanoseconds\n",
            p->spent.s / 3600,
            (p->spent.s / 60) % 60,
            p->spent.s % 60, p->spent.n);

    entry_status(stdout);
    return 0;
}
