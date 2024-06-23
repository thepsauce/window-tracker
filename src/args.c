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

bool parse_args(int argc, char **argv)
{
    struct opt {
        const char *lng;
        char shrt;
        /* 0 - no arguments */
        /* 1 - single argument */
        /* 2 - arguments until the next '-' */
        int n;
        union {
            bool *b;
            struct {
                char ***p;
                size_t *n;
            } v;
            char **s;
        } dest;
    } pArgs[] = {
        { "help", 'h', 0, { .b = &Args.needsHelp } },
        { "usage", '\0', 0, { .b = NULL } },
        { "filter", '\0', 2, { .v = { &Args.filterWords, &Args.numFilterWords } } },
        { "output", 'o', 1, { .s = &Args.output } },
        { "format", '\0', 1, { .s = &Args.format } },
    };

    argc--;
    argv++;
    char sArg[2] = { '.', '\0' };
    for (int i = 0; i != argc; ) {
        char *arg = argv[i];
        char **vals = NULL;
        int numVals = 0;
        const struct opt *o;
        int on = -1;
        char *equ;
        if (arg[0] == '-') {
            arg++;
            equ = strchr(arg, '=');
            if (equ != NULL) {
                *(equ++) = '\0';
            }
            if (arg[0] == '-' || equ != NULL) {
                if (arg[0] == '-') {
                    arg++;
                }

                if (arg[0] == '\0') {
                    i++;
                    Args.trackFiles = &argv[i];
                    Args.numTrackFiles = argc - i;
                    break;
                }

                for (size_t i = 0; i < ARRAY_SIZE(pArgs); i++) {
                    if (strcmp(pArgs[i].lng, arg) == 0) {
                        o = &pArgs[i];
                        on = o->n;
                        break;
                    }
                }
                if (equ != NULL) {
                    argv[i] = equ;
                } else {
                    i++;
                }

                vals = &argv[i];
                if (on > 0 && i != argc) {
                    for (; i != argc && argv[i][0] != '-'; i++) {
                        numVals++;
                        if (on != 1) {
                            break;
                        }
                    }
                } else if (equ != NULL) {
                    numVals = 1;
                }
            } else {
                sArg[0] = *(arg++);
                for (size_t i = 0; i < ARRAY_SIZE(pArgs); i++) {
                    if (pArgs[i].shrt == sArg[0]) {
                        o = &pArgs[i];
                        on = o->n;
                        break;
                    }
                }
                if (arg[0] != '\0') {
                    if (on > 0) {
                        argv[i] = arg;
                        vals = &argv[i];
                        numVals = 1;
                        i++;
                    } else {
                        arg[-1] = '-';
                        argv[i] = arg - 1;
                    }
                } else {
                    i++;
                    if (on == 1 && i != argc) {
                        vals = &argv[i++];
                        numVals = 1;
                    } else if (on == 2) {
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
        switch (on) {
        case -1:
            fprintf(stderr, "invalid option '%s'\n", arg);
            return false;
        case 0:
            if (numVals > 0) {
                fprintf(stderr, "option '%s' does not expect any arguments\n",
                        arg);
                return false;
            }
            if (o->dest.b != NULL) {
                *o->dest.b = true;
            }
            break;
        case 1:
            if (numVals == 0) {
                fprintf(stderr, "option '%s' expects one argument\n", arg);
                return false;
            }
            if (o->dest.s != NULL) {
                *o->dest.s = vals[0];
            }
            break;
        case 2:
            if (o->dest.v.p != NULL) {
                *o->dest.v.p = vals;
                *o->dest.v.n = numVals;
            }
            break;
        }
    }
    return true;
}

void usage(FILE *fp, const char *programName)
{
    fprintf(fp, "window tracker usage:\n"
            "%s [options] [track files]\n"
            "\n"
            "where 'track files' are either directories or files\n"
            "and options are one of the following:\n"
            "  --help|--usage|-h          Show this help\n"
            "  --filter <filter words...> Apply a filter\n"
            "  --output|-o <file>         Start outputting tracking data, <file> can also be\n"
            "                             a folder\n"
            "  --format <format>          Change file output format this help\n"
            "\n"
            "Examples:\n"
            "1. --filter=firefox file.track\n"
            "2. --filter firefox youtube -- file.track\n"
            "3. --output $HOME/.tracks/\n"
            "4. --output $HOME/.track_file\n"
            "\n"
            "1: search in the file.track file and apply a single filter (firefox)\n"
            "2: search in file.track using multiple filters\n"
            "3: start tracking and put the data in the .tracks directory\n"
            "4: start tracking and put the data in the .track_file file\n",
            programName);
}
