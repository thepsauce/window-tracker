#include "entry.h"
#include "track.h"
#include "extime.h"
#include "macros.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static bool parse_matches(struct parse_track *p, const char *s)
{
    for (size_t i = 0; i < p->numWords; i++) {
        if (strcasestr(s, p->words[i]) == NULL) {
            return false;
        }
    }
    return true;
}

static int parse_legacy_file(struct parse_track *p, FILE *fp)
{
    for (p->line = 1;; p->line++) {
        ssize_t lenTitle = getline(&p->title, &p->capTitle, fp);
        if (lenTitle <= 0) {
            return errno == 0 ? 0 : -1;
        }
        if (p->title[lenTitle - 1] == '\n') {
            p->title[lenTitle - 1] = '\0';
        }
        p->line++;

        ssize_t lenData = getline(&p->data, &p->capData, fp);
        if (lenData <= 0) {
            return -1;
        }

        if (!parse_matches(p, p->title)) {
            continue;
        }

        bool hasStart = false;
        struct timespec start, end;
        char *s = strtok(p->data, " ");
        while (s != NULL) {
            char *d = strchr(s, '.');
            if (d == NULL) {
                return -1;
            }
            *d = '\0';
            end.tv_sec = strtoll(s, NULL, 10);
            end.tv_nsec = strtoll(d + 1, NULL, 10);
            if (hasStart) {
                struct entry e;
                e.name = NULL;
                e.instance = NULL;
                e.title = strdup(p->title);
                add_entry(&e, start, end);
            } else {
                start = end;
            }
            s = strtok(NULL, " ");
            hasStart = !hasStart;
        }
        if (hasStart) {
            return -1;
        }
    }
    return 0;
}

static char *read_string(struct parse_track *p, FILE *fp)
{
    size_t len = 0;
    for (int c; (c = fgetc(fp)) != '\0' && c != EOF; ) {
        if (len + 2 >= p->capData) {
            p->capData *= 2;
            char *b = realloc(p->data, p->capData);
            if (b == NULL) {
                return NULL;
            }
        }
        p->data[len++] = c;
    }
    p->data[len++] = '\0';
    return p->data;
}

static char *null_strdup(const char *s)
{
    if (s == NULL) {
        return NULL;
    }
    return strdup(s);
}

static int parse_track_file_v01(struct parse_track *p, FILE *fp)
{
    struct timespec now;
    struct entry *cur = NULL;
    struct entry e;
    struct timespec tv;

    if (fread(&now, sizeof(now), 1, fp) != 1) {
        return 1;
    }
    tv.tv_sec = 0;
    for (int c; (c = fgetc(fp)) != EOF; ) {
        if (c == FILE_FOCUS_NULL || c == FILE_FOCUS_CHANGE) {
            if (fread(&tv.tv_nsec, sizeof(&tv.tv_nsec), 1, fp) != 1) {
                return 1;
            }
            if (cur != NULL) {
                cur->spent = add_timespec(cur->spent, tv);
                cur->last = add_timespec(cur->last, tv);
            }
            now = add_timespec(now, tv);
            Entries.spent = add_timespec(Entries.spent, tv);
        }
        switch (c) {
        case FILE_TIME_ADJUST:
            if (fread(&now, sizeof(now), 1, fp) != 1) {
                return 1;
            }
            if (cur != NULL) {
                cur->last = now;
            }
            break;
        case FILE_FOCUS_NULL:
            cur = NULL;
            break;
        case FILE_FOCUS_CHANGE:
            e.name = null_strdup(read_string(p, fp));
            e.instance = null_strdup(read_string(p, fp));
            e.title = null_strdup(read_string(p, fp));
            cur = add_entry(&e, now, now);
            break;
        case FILE_TIME_PASSED:
            if (cur != NULL) {
                cur->spent.tv_sec++;
                cur->last.tv_sec++;
                now.tv_sec++;
                Entries.spent.tv_sec++;
            }
            break;
        }
    }
    Entries.last = now;
    return 0;
}

static int parse_track_file(struct parse_track *p, FILE *fp)
{
    char version[2];
    if (fread(version, 1, sizeof(version), fp) != sizeof(version)) {
        return -1;
    }
    if (version[0] == '\0') {
        fprintf(stderr, "track file version '%u'\n",
                (uint8_t) version[1]);
        switch ((uint8_t) version[1]) {
        case 0x01:
            return parse_track_file_v01(p, fp);
        }
    }

    rewind(fp);
    return parse_legacy_file(p, fp);
}

int read_track_file(struct parse_track *p)
{
    int result;
    FILE *fp = fopen(p->file, "r");

    if (fp == NULL) {
        fprintf(stderr, "failed opening '%s': %s\n", p->file,
                strerror(errno));
        return 1;
    }

    char header[STRING_SIZE(FILE_HEADER)];
    if (fread(header, 1, sizeof(header), fp) == sizeof(header) &&
            memcmp(header, FILE_HEADER, sizeof(header)) == 0) {
        result = parse_track_file(p, fp);
    } else {
        rewind(fp);
        result = parse_legacy_file(p, fp);
    }

    fclose(fp);
    return result;
}

int read_tracks(struct parse_track *p)
{
    if (p->capData == 0) {
        p->capData = 4096;
        p->data = malloc(p->capData);
        if (p->data == NULL) {
            fprintf(stderr, "out of memory: %s\n", strerror(errno));
            return -1;
        }
    }

    struct stat st;
    if (stat(p->file, &st) < 0) {
        return -1;
    }
    const int type = st.st_mode & S_IFMT;
    if (type == S_IFDIR) {
        chdir(p->file);

        struct dirent *ent;
        DIR *dir = opendir(".");
        if (dir == NULL) {
            fprintf(stderr, "failed opening current directory: %s\n",
                    strerror(errno));
            return -1;
        }
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type != DT_REG) {
                continue;
            }
            p->numFiles++;
            p->file = ent->d_name;
            if (read_track_file(p) < 0) {
                fprintf(stderr, "misformatted file: %s:%zu\n", p->file, p->line);
                p->numMisformatted++;
            }
        }
    } else if (type == S_IFREG) {
        read_track_file(p);
    }
    return 0;
}
