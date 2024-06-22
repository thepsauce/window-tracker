#include "args.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>

struct program_arguments Args;

bool is_not_filtered_out(const char *s)
{
    for (size_t i = 0; i < Args.numFilterWords; i++) {
        if (strcasestr(s, Args.filterWords[i]) == NULL) {
            return false;
        }
    }
    return true;
}

enum arg_type {
    ARG_NULL,
    ARG_HELP,
    ARG_FILTER,
};

static void receive_arg(enum arg_type type, char **vals, int numVals)
{
    switch (type) {
    case ARG_NULL:
        break;
    case ARG_HELP:
        Args.needsHelp = true;
        break;
    case ARG_FILTER:
        Args.filterWords = vals;
        Args.numFilterWords = numVals;
        break;
    }
}

bool parse_args(int argc, char **argv)
{
    struct {
        enum arg_type type;
        const char *lng;
        char shrt;
        /* 0 - no arguments */
        /* 1 - single argument */
        /* 2 - arguments until the next '-' */
        int n;
        void *dest;
    } pArgs[] = {
        { ARG_HELP, "help", 'h', 0, &Args.needsHelp },
        { ARG_HELP, "usage", '\0', 0, &Args.needsHelp },
        { ARG_FILTER, "filter", '\0', 2, &Args.needsHelp },
    };

    argc--;
    argv++;
    char sArg[2] = { '.', '\0' };
    char *pArg;
    for (int i = 0; i != argc; ) {
        char *arg = argv[i];
        char **vals = NULL;
        int numVals = 0;
        enum arg_type type = ARG_NULL;
        int n = 0;
        if (arg[0] == '-') {
            arg++;
            if (arg[0] == '-') {
                arg++;
                if (arg[0] == '\0') {
                    Args.trackFiles = &argv[i];
                    Args.numTrackFiles = argc - i;
                    break;
                }
                char *equ = strchr(arg, '=');
                if (equ != NULL) {
                    *(equ++) = '\0';
                }
                for (size_t i = 0; i < ARRAY_SIZE(pArgs); i++) {
                    if (strcmp(pArgs[i].lng, arg) == 0) {
                        type = pArgs[i].type;
                        n = pArgs[i].n;
                        break;
                    }
                }
                i++;
                if (equ == NULL && n > 0 && i != argc) {
                    vals = &argv[i];
                    for (; i != argc && argv[i][0] != '-'; i++) {
                        numVals++;
                        if (n == 1) {
                            break;
                        }
                    }
                } else if (equ != NULL) {
                    pArg = equ;
                    vals = &pArg;
                    numVals = 1;
                }
            } else {
                sArg[0] = *(arg++);
                for (size_t i = 0; i < ARRAY_SIZE(pArgs); i++) {
                    if (pArgs[i].shrt == sArg[0]) {
                        type = pArgs[i].type;
                        n = pArgs[i].n;
                        break;
                    }
                }
                if (arg[0] != '\0') {
                    if (n > 0) {
                        pArg = arg;
                        vals = &pArg;
                        numVals = 1;
                        i++;
                    } else {
                        arg[-1] = '-';
                        argv[i] = arg - 1;
                    }
                } else {
                    i++;
                    if (n == 1 && i != argc) {
                        vals = &argv[i++];
                        numVals = 1;
                    } else if (n == 2) {
                        vals = &argv[i];
                        for (; i != argc && argv[i][0] != '-'; i++) {
                            numVals++;
                        }
                    }
                }
                arg = sArg;
            }
        } else {
            /* use rest as track files */
            Args.trackFiles = &argv[i];
            Args.numTrackFiles = argc - i;
            break;
        }
        if (numVals > 0 && n == 0) {
            fprintf(stderr, "option '%s' does not expect any arguments\n", arg);
            return false;
        } else if (numVals == 0 && n == 1) {
            fprintf(stderr, "option '%s' expects one argument\n", arg);
            return false;
        }
        if (type == ARG_NULL) {
            fprintf(stderr, "invalid option '%s'\n", arg);
            return false;
        }
        receive_arg(type, vals, numVals);
    }
    return true;
}

void usage(FILE *fp, const char *programName)
{
    fprintf(fp, "window tracker usage:\n"
            "%s [options] [track files]\n\n"
            "where 'track files' are either directories or files\n"
            "and options are one of the following:\n"
            "  --help|--usage|-h    Show this help",
            programName);
}
