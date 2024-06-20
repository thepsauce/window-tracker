#ifndef TRACK_H
#define TRACK_H

#include <stddef.h>

#define FILE_VERSION "\x00\x01"

#define FILE_HEADER "WTF"

/* file format:
 * \x00\x01WTF
 * <real time> <boot time> (struct timespec)
 *
 * <class name> <class instance> <title> (all null terminated)
 * \x01 \x01 \x01 ... \x00
 *
 * (or)
 *
 * \x01 \x01 \x01 ... \x00 (meaning the desktop is focused and not any
 * specific window)
 *
 * .
 * .
 * .
 */

#define FILE_FOCUS_NULL '\x00'
#define FILE_FOCUS_CHANGE '\x01'
#define FILE_TIME_PASSED '\x02'

struct parse_track {
    const char *file;
    size_t line;
    char *title, *data;
    size_t capTitle, capData;

    char **words;
    size_t numWords;

    size_t numFiles, numMisformatted;
};

int read_track_file(struct parse_track *p);
int read_tracks(struct parse_track *p);

#endif

