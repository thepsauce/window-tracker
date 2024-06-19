#include "timet.h"

#include <stdio.h>

int main(void)
{
    struct time t1, t2;

    t1.s = 249;
    t1.n = 0;
    t2.s = 249;
    t2.n = 0;

    printf("%d\n", TCMP(t1, t2));

    t2.n = 800;
    printf("%d\n", TCMP(t1, t2));

    t1.n = 9000;
    printf("%d\n", TCMP(t1, t2));

    t2.s = 800;
    printf("%d\n", TCMP(t1, t2));

    t1.s = 1800;
    printf("%d\n", TCMP(t1, t2));

    return 0;
}
