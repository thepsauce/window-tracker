#include "args.h"
#include "stream.h"
#include "track.h"
#include "windowsystem.h"
#include "extime.h"
#include "macros.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

static const char *BackupFile = ".window_tracker_backup";

struct stream {
    const char *fileName;
    FILE *fp;
};

char *get_next_file_name(void)
{
    static char buf[1024];
    static const char *format = "%F_%T";
    int n = 0;
    do {
        time_t t = time(NULL);
        size_t l = strftime(buf, sizeof(buf), 
                Args.format == NULL ? format : Args.format,
                localtime(&t));
        if (n > 0) {
            snprintf(&buf[l], sizeof(buf) - l, "_%d", n);
        }
    } while (access(buf, F_OK) == 0);
    return buf;
}

static int stream_open(struct stream *s)
{
    struct stat st;
    bool exists;

    exists = stat(s->fileName, &st) == 0;
    if (exists && (st.st_mode & S_IFMT) == S_IFDIR) {
        if (chdir(s->fileName) != 0) {
            return -1;
        }
        s->fp = fopen(get_next_file_name(), "wb");
    } else {
        s->fp = fopen(s->fileName, "ab");
    }

    if (s->fp == NULL) {
        return -1;
    }

    setvbuf(s->fp, NULL, _IONBF, 0);
    if (!exists) {
        if (fwrite(FILE_HEADER, 1, STRING_SIZE(FILE_HEADER), s->fp) !=
                STRING_SIZE(FILE_HEADER)) {
            fclose(s->fp);
            return -1;
        }
        if (fwrite(FILE_VERSION, 1, STRING_SIZE(FILE_VERSION), s->fp) !=
                STRING_SIZE(FILE_VERSION)) {
            fclose(s->fp);
            return -1;
        }
    }
    return 0;
}

int stream_write(struct stream *s, void *data, size_t size, size_t nmemb)
{
    /* things that can go wrong:
     * 1. file was destroyed
     * 2. access lost
     * 3. other
     */
    const int maxTries = 5;
    if (fwrite(data, size, nmemb, s->fp) != nmemb) {
        if (access(s->fileName, W_OK | F_OK) != 0) {
            switch (errno) {
            case EACCES:
            case ELOOP:
                goto backup;
            case ENOENT:
                fclose(s->fp);
                for (int tries = maxTries;; tries--) {
                    if (stream_open(s) == 0) {
                        break;
                    }
                    if (tries == 0) {
                        s->fp = NULL;
                        goto backup;
                    }
                    sleep(maxTries - tries);
                }
                break;
            }
        }
    }
    return 0;

backup:
    if (s->fp != NULL) {
        fclose(s->fp);
    }
    s->fileName = BackupFile;
    s->fp = fopen(s->fileName, "ab");
    if (s->fp == NULL) {
        return -1;
    }
    return 1;
}

void stream_track_data(const char *fileName)
{
    struct stream s;

    s.fileName = fileName;
    stream_open(&s);

    struct timespec start, end, diff;

    static char *recov = "OUT OF MEMORY";
    char *old = NULL;

    clock_gettime(CLOCK_REALTIME, &start);
    fwrite(&start, sizeof(start), 1, s.fp);
    while (1) {
        clock_gettime(CLOCK_REALTIME, &end);
        diff = sub_timespec(end, start);
        if (diff.tv_sec > 1) {
            /* too much time passed... */
            start = end;
            fputc(FILE_TIME_ADJUST, s.fp);
            fwrite(&end, sizeof(end), 1, s.fp);
            continue;
        }
        if (diff.tv_sec < 0) {
            /* the user set the time back */
            /* TODO: */
        }
        if (diff.tv_sec > 0) {
            fputc(FILE_TIME_PASSED, s.fp);
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
                fputc(FILE_FOCUS_CHANGE, s.fp);
                fwrite(&diff.tv_nsec, sizeof(diff.tv_nsec), 1, s.fp);
                start.tv_nsec = end.tv_nsec;
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
                fwrite(n, 1, ln + 1, s.fp);
                fwrite(i, 1, li + 1, s.fp);
                fwrite(t, 1, lt + 1, s.fp);
            }
        } else if (old != NULL) {
            if (old != recov) {
                free(old);
            }
            old = NULL;
            fputc(FILE_FOCUS_NULL, s.fp);
            fwrite(&diff.tv_nsec, sizeof(diff.tv_nsec), 1, s.fp);
            start.tv_nsec = end.tv_nsec;
        }
        /* 100 millisecond sleep */
        usleep(100000);
    }

    /* this point is never reached, the program is stopped with a signal */
    fclose(s.fp);
}
