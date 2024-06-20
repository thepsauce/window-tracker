#include "entry.h"
#include "track.h"
#include "macros.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static int parse_legacy_file(struct parse_track *p, FILE *fp)
{
    for (p->line = 1;; p->line++) {
        ssize_t lenTitle = getline(&p->title, &p->capTitle, fp);
        if (lenTitle <= 0) {
            fclose(fp);
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

        bool match = true;
        for (size_t i = 0; i < p->numWords; i++) {
            if (strcasestr(p->title, p->words[i]) == NULL) {
                match = false;
                break;
            }
        }
        if (!match) {
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
                add_entry(p->title, start, end);
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

static int parse_track_file(struct parse_track *p, FILE *fp)
{
    (void) p; (void) fp; /*TODO:*/
    return 0;
}

int read_track_file(struct parse_track *p)
{
    FILE *fp = fopen(p->file, "r");
    int result;

    if (fp == NULL) {
        fprintf(stderr, "failed opening: '%s'\n", p->file);
        return 1;
    }

    char buf[STRING_SIZE(FILE_HEADER)];
    if (fread(buf, 1, sizeof(buf), fp) == sizeof(buf) &&
            memcmp(buf, FILE_HEADER, sizeof(buf)) == 0) {
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
            fprintf(stderr, "failed opening current directory\n");
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
