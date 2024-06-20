#include "args.h"
#include "entry.h"
#include "windowsystem.h"
#include "track.h"
#include "macros.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (!parse_args(argc, argv)) {
        usage(stderr, argv[0]);
        return -1;
    }

    if (Args.needsHelp) {
        usage(stdout, argv[0]);
        return 0;
    }

    if (Args.numTrackFiles > 0) {
        struct parse_track p;

        memset(&p, 0, sizeof(p));
        for (size_t i = 0; i < Args.numTrackFiles; i++) {
            p.file = Args.trackFiles[i];
            read_tracks(&p);
        }

        printf("read %zu track files of witch %zu were misformatted\n",
                p.numFiles, p.numMisformatted);

        entry_status(stdout);

        printf("\nend of reports for");
        for (size_t i = 0; i < Args.numTrackFiles; i++) {
            printf(" '%s'", Args.trackFiles[i]);
        }
        printf("\n");

        free(p.title);
        free(p.data);
        return 0;
    }

    if (init_window_system() < 0) {
        return 1;
    }

    FILE *fp;

    fp = fopen("file.track", "wb");
    setvbuf(fp, NULL, _IONBF, 0);

    fwrite(FILE_VERSION, 1, STRING_SIZE(FILE_VERSION), fp);
    fwrite(FILE_HEADER, 1, STRING_SIZE(FILE_HEADER), fp);

    if (ferror(fp) != 0) {
        /* handle error */
        /* We can not just quit as this process might have been
         * started in the background on system start which
         * means the user will have no clue that the program
         * is not working. If any error happens here,
         * there needs to be found a way to recover.
         */
    }

    struct timespec real;
    struct timespec start, end, diff;

    static char *recov = "OUT OF MEMORY";
    char *old = NULL;

    clock_gettime(CLOCK_REALTIME, &real);
    clock_gettime(CLOCK_BOOTTIME, &start);
    fprintf(stdout, "now: %s", ctime(&real.tv_sec));

    fwrite(&real, sizeof(real), 1, fp);
    fwrite(&start, sizeof(start), 1, fp);
    while (1) {
        clock_gettime(CLOCK_BOOTTIME, &end);
        diff = sub_timespec(end, start);
        while (diff.tv_sec > 0) {
            fputc(FILE_TIME_PASSED, fp);
            diff.tv_sec--;
            start.tv_sec++;
            start.tv_nsec = NANOS_PER_SECOND - start.tv_nsec;
        }
        if (update_active_window()) {
            char *n, *i, *t;
            size_t ln, li, lt;
            n = get_window_name();
            n = n == NULL ? "" : n;
            i = get_window_instance();
            i = i == NULL ? "" : i;
            t = get_window_title();
            t = t == NULL ? "" : t;
            ln = strlen(n);
            li = strlen(i);
            lt = strlen(t);
            if (old == NULL || strcmp(old, n) != 0 ||
                    strcmp(old + ln + 1, i) != 0 ||
                    strcmp(old + ln + 1 + li + 1, t) != 0) {
                fputc(FILE_FOCUS_CHANGE, fp);
                if (old != recov) {
                    free(old);
                }
                old = malloc(ln + 1 + li + 1 + lt + 1);
                if (old == NULL) {
                    /* uh oh, big problem */
                    old = recov;
                } else {
                    strcpy(old, n);
                    strcpy(old + ln + 1, i);
                    strcpy(old + ln + 1 + li + 1, t);
                }
                fwrite(n, 1, ln + 1, fp);
                fwrite(i, 1, li + 1, fp);
                fwrite(t, 1, lt + 1, fp);
            }
        } else if (old != NULL) {
            if (old != recov) {
                free(old);
            }
            old = NULL;
            fputc(FILE_FOCUS_NULL, fp);
        }
        /* 100 millisecond sleep */
        usleep(100000);
    }

    /* this point is never reached, the program is stopped with a signal */
    fclose(fp);

    return 0;
}

