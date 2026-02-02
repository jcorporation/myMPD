/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief File functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_file.h"

#include "dist/sds/sds.h"
#include "src/lib/log.h"

#include <errno.h>

/**
 * Getline function that trims whitespace characters
 * @param s an already allocated sds string
 * @param fp a file descriptor to read from
 * @param max max line length to read
 * @param nread Number of bytes read,
                -1 on EOF
 * @return Pointer to s
 */
sds sds_getline(sds s, FILE *fp, size_t max, int *nread) {
    sdsclear(s);
    s = sdsMakeRoomFor(s, max + 1);
    for (size_t i = 0; i < max; i++) {
        int c = fgetc(fp);
        if (c == EOF ||
            c == '\n')
        {
            s[i] = '\0';
            sdstrim(s, "\r \t");
            *nread = c == EOF
                ? -1
                : (int)sdslen(s);
            return s;
        }
        s[i] = (char)c;
        sdsinclen(s, 1);
    }
    MYMPD_LOG_ERROR(NULL, "Line is too long, max length is %lu", (unsigned long)max);
    s[max] = '\0';
    sdstrim(s, "\r \t");

    *nread = (int)sdslen(s);
    return s;
}

/**
 * Reads a whole file in the sds string s from *fp
 * Removes whitespace characters from start and end
 * @param s an already allocated sds string that should hold the file content
 * @param file_path filename to read
 * @param max maximum bytes to read
 * @param remove_newline removes CR/LF if true
 * @param warn log an error if file does not exist
 * @param nread Number of bytes read,
 *              -1 error reading file,
 *              -2 file is too big
 * @return pointer to s
 */
sds sds_getfile(sds s, const char *file_path, size_t max, bool remove_newline, bool warn, int *nread) {
    errno = 0;
    FILE *fp = fopen(file_path, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (warn == false &&
            errno == ENOENT)
        {
            MYMPD_LOG_DEBUG(NULL, "File \"%s\" does not exist", file_path);
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Error opening file \"%s\"", file_path);
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        *nread = FILE_NOT_EXISTS;
        return s;
    }
    s = sds_getfile_from_fp(s, fp, max, remove_newline, nread);
    (void) fclose(fp);
    return s;
}

/**
 * Reads a whole file in the sds string s from *fp
 * Removes whitespace characters from start and end
 * @param s an already allocated sds string that should hold the file content
 * @param fp FILE pointer to read
 * @param max maximum bytes to read
 * @param remove_newline removes CR/LF if true
 * @param nread Number of bytes read,
 *              -2 if file is too big
 * @return pointer to s
 */
sds sds_getfile_from_fp(sds s, FILE *fp, size_t max, bool remove_newline, int *nread) {
    sdsclear(s);
    s = sdsMakeRoomFor(s, max + 1);
    int c;
    size_t i = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (i > max) {
            s[max] = '\0';
            sdstrim(s, "\r \t\n");
            MYMPD_LOG_ERROR(NULL, "File is too big, max size is %lu", (unsigned long)max);
            *nread = FILE_TO_BIG;
            return s;
        }
        if (remove_newline == true &&
            (c == '\n' || c == '\r'))
        {
            continue;
        }
        s[i] = (char)c;
        sdsinclen(s, 1);
        i++;
    }
    s[i] = '\0';
    sdstrim(s, "\r \t\n");
    *nread = (int)sdslen(s);
    return s;
}

/**
 * Replacement for basename function.
 * Modifies the sds string in place.
 * @param s sds string to apply basename function
 * @return new pointer to s
 */
sds sds_basename(sds s) {
    if (!s || !*s) {
        sdsclear(s);
        return sdscat(s, ".");
    }

    size_t idx = sdslen(s) - 1;
    int end = -1;
    // remove trailing slash
    for (; idx && s[idx] == '/'; idx--) {
        end--;
    }
    // get non-slash component
    for (; idx && s[idx - 1] != '/'; idx--) {
        // count only
    }
    sdsrange(s, (ssize_t)idx, end);
    return s;
}

/**
 * Replacement for dirname function.
 * Modifies the sds string in place.
 * @param s sds string to apply dirname function
 * @return new pointer to s
 */
sds sds_dirname(sds s) {
    if (!s || !*s) {
        sdsclear(s);
        return sdscat(s, ".");
    }
    size_t idx = sdslen(s) - 1;
    // remove trailing slash
    for (; s[idx] == '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, "/");
        }
    }
    // remove last non-slash component
    for (; s[idx] != '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, ".");
        }
    }
    // remove trailing slash
    for (; s[idx] == '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, "/");
        }
    }
    sdsrange(s, 0, (ssize_t)idx);
    return s;
}
