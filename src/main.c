#include "args.h"
#include "entry.h"
#include "windowsystem.h"
#include "track.h"
#include "stream.h"
#include "macros.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (argc == 1) {
        usage(stdout, argv[0]);
        return 0;
    }

    if (!parse_args(argc, argv)) {
        usage(stderr, argv[0]);
        return -1;
    }

    if (Args.needsHelp) {
        usage(stdout, argv[0]);
        return 0;
    }

    if (Args.output != NULL) {
        if (init_window_system() < 0) {
            return 1;
        }
        stream_track_data(Args.output);
        return 0;
    }

    struct parse_track p;

    memset(&p, 0, sizeof(p));
    for (size_t i = 0; i < Args.numTrackFiles; i++) {
        p.file = Args.trackFiles[i];
        read_tracks(&p);
    }

    fprintf(stderr, "read %zu track files of which %zu were misformatted\n",
            p.numFiles, p.numMisformatted);

    entry_status(stdout);

    printf("\nend of reports");
    if (Args.numFilterWords > 0) {
        printf(" for");
        for (size_t i = 0; i < Args.numFilterWords; i++) {
            printf(" '%s'", Args.filterWords[i]);
        }
    }
    printf("\n");

    free(p.title);
    free(p.data);
    return 0;
}

