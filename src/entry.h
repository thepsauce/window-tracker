#ifndef ENTRY_H
#define ENTRY_H

#include "extime.h"

#include <stdbool.h>
#include <stdio.h>

struct entry {
    char *name;
    char *instance;
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

bool search_entry(const struct entry *entry, size_t *pIndex);
struct entry *add_entry(struct entry *entry, struct timespec t1, struct timespec t2);
void entry_status(FILE *fp);

#endif

