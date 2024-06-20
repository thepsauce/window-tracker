#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>

extern struct program_arguments {
    bool needsHelp;
    char *const *trackFiles;
    size_t numTrackFiles;
} Args;

bool parse_args(int argc, char **argv);
void usage(FILE *fp, const char *programName);

#endif

