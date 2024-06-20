#include "args.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>

struct program_arguments Args;

enum arg_type {
    ARG_NULL,
    ARG_HELP,
};

static void receive_arg(enum arg_type type, const char *val)
{
    (void) val;
    switch (type) {
    case ARG_NULL:
        break;
    case ARG_HELP:
        Args.needsHelp = true;
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
        int n;
        void *dest;
    } pArgs[] = {
        { ARG_HELP, "help", 'h', 0, &Args.needsHelp },
        { ARG_HELP, "usage", '\0', 0, &Args.needsHelp },
    };

    argc--;
    argv++;
    char sArg[2] = { '.', '\0' };
    for (int i = 0; i != argc; ) {
        char *arg = argv[i];
        char *val = NULL;
        enum arg_type type = ARG_NULL;
        int n = 0;
        if (arg[0] == '-') {
            arg++;
            if (arg[0] == '-') {
                arg++;
                val = strchr(arg, '=');
                if (val != NULL) {
                    *(val++) = '\0';
                }
                for (size_t i = 0; i < ARRAY_SIZE(pArgs); i++) {
                    if (strcmp(pArgs[i].lng, arg) == 0) {
                        type = pArgs[i].type;
                        n = pArgs[i].n;
                        break;
                    }
                }
                i++;
                if (val == NULL && n == 1 && i != argc) {
                    val = argv[i++];
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
                    if (n == 1) {
                        val = arg;
                        i++;
                    } else {
                        arg[-1] = '-';
                        argv[i] = arg - 1;
                    }
                } else {
                    i++;
                    if (n == 1 && i != argc) {
                        val = argv[i++];
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
        if (val != NULL && n == 0) {
            fprintf(stderr, "option '%s' does not expect any arguments\n", arg);
            return false;
        } else if (val == NULL && n == 1) {
            fprintf(stderr, "option '%s' expects one argument\n", arg);
            return false;
        }
        if (type == ARG_NULL) {
            fprintf(stderr, "invalid option '%s'\n", arg);
            return false;
        }
        receive_arg(type, val);
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
