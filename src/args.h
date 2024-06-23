#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>
#include <stdio.h>

extern struct program_arguments {
    bool needsHelp;
    char **trackFiles;
    size_t numTrackFiles;
    char **filterWords;
    size_t numFilterWords;
    char *output;
    char *format;
} Args;

bool is_not_filtered_out(const char *s);
bool parse_args(int argc, char **argv);
void usage(FILE *fp, const char *programName);

#endif

