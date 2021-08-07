/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_UTILITY_H
#define MYMPD_UTILITY_H

#include <stdbool.h>

#include "../dist/src/sds/sds.h"

int testdir(const char *name, const char *dirname, bool create);
int replacechar(char *str, const char orig, const char rep);
int uri_to_filename(char *str);
sds find_image_file(sds basefilename);
sds get_extension_from_filename(const char *filename);
sds get_mime_type_by_ext(const char *filename);
sds get_ext_by_mime_type(const char *mime_type);
sds get_mime_type_by_magic(const char *filename);
sds get_mime_type_by_magic_stream(sds stream);
bool strtobool(const char *value);
int strip_extension(char *s);
void strip_slash(sds s);
char *strtolower(char* s);
void my_usleep(time_t usec);
unsigned long substractUnsigned(unsigned long num1, unsigned long num2);
char *basename_uri(char *uri);
int unsigned_to_int(unsigned x);

#define FREE_PTR(PTR) do { \
    if (PTR != NULL) \
        free(PTR); \
    PTR = NULL; \
} while (0)

struct mime_type_entry {
    const char *extension;
    const char *mime_type;
};

struct magic_byte_entry {
    const char *magic_bytes;
    const char *mime_type;
};

#endif
