#include "windowsystem.h"
#include "entry.h"

#define _GNU_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char *title = NULL, *data = NULL;
    ssize_t lenTitle, lenData;
    size_t capTitle = 0, capData = 0;
    size_t numFiles = 0, numMisformatted = 0;

    setlocale(LC_ALL, "");

    /*if (init_window_system() < 0) {
        return 1;
    }

    while (1) {
        void *w = get_active_window();
        fprintf(stdout, "%s\n", (char*) w);
        sleep(1);
    }*/

    chdir("/home/steves/.tracks");

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
        FILE *fp = fopen(ent->d_name, "r");

        if (fp == NULL) {
            fprintf(stderr, "failed opening: '%s'\n", ent->d_name);
            continue;
        }

        numFiles++;

        bool misformat = false;
        size_t line;
        for (line = 1;; line++) {
            lenTitle = getline(&title, &capTitle, fp);
            if (lenTitle <= 0) {
                misformat = errno != 0;
                break;
            }
            if (title[lenTitle - 1] == '\n') {
                title[lenTitle - 1] = '\0';
            }
            line++;

            lenData = getline(&data, &capData, fp);
            if (lenData <= 0) {
                misformat = true;
                break;
            }

            bool match = true;
            for (int i = 1; i < argc; i++) {
                if (strcasestr(title, argv[i]) == NULL) {
                    match = false;
                    break;
                }
            }
            if (!match) {
                continue;
            }

            bool hasStart = false;
            struct timespec start, end;
            char *s = strtok(data, " ");
            while (s != NULL) {
                char *d = strchr(s, '.');
                if (d == NULL) {
                    misformat = true;
                    break;
                }
                *d = '\0';
                end.tv_sec = strtoll(s, NULL, 10);
                end.tv_nsec = strtoll(d + 1, NULL, 10);
                if (hasStart) {
                    add_entry(title, start, end);
                } else {
                    start = end;
                }
                s = strtok(NULL, " ");
                hasStart = !hasStart;
            }
            if (misformat) {
                break;
            }
        }
        if (misformat) {
            fprintf(stderr, "misformatted file: %s:%zu\n", ent->d_name, line);
            numMisformatted++;
        }
    }

    printf("read %zu track files of witch %zu were misformatted\n",
            numFiles, numMisformatted);

    entry_status(stdout);

    printf("\nend of reports");
    if (argc > 1) {
        printf(" for");
        for (int i = 1; i < argc; i++) {
            printf(" '%s'", argv[i]);
        }
    }
    printf("\n");

    return 0;
}

