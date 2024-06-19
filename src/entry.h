#ifndef ENTRY_H
#define ENTRY_H

#include "extime.h"

#include <stdbool.h>
#include <stdio.h>

struct entry {
    char *title;
    struct timespec first;
    struct timespec spent;
    struct timespec last;
};

extern struct entry_list {
    struct entry *p;
    size_t n, a;
    struct timespec first;
    struct timespec spent;
    struct timespec last;
} Entries;

bool search_entry(const char *title, size_t *pIndex);
void add_entry(const char *title, struct timespec t1, struct timespec t2);
void entry_status(FILE *fp);

#endif

