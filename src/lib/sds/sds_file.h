/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief File functions for sds strings
 */

#ifndef MYMPD_SDS_FILE_H
#define MYMPD_SDS_FILE_H

#include "dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * Status of sds_getfile functions
 */
enum getfile_status {
    FILE_IS_EMPTY = 0,
    FILE_NOT_EXISTS = -1,
    FILE_TO_BIG = -2
};

sds sds_getline(sds s, FILE *fp, size_t max, int *nread);
sds sds_getfile(sds s, const char *file_path, size_t max, bool remove_newline, bool warn, int *nread);
sds sds_getfile_from_fp(sds s, FILE *fp, size_t max, bool remove_newline, int *nread);

sds sds_basename(sds s);
sds sds_dirname(sds s);

void sds_basename_uri(sds uri);
void sds_strip_file_extension(sds filename);
sds sds_replace_file_extension(sds filename, const char *ext);
void sds_strip_slash(sds dirname);
void sds_sanitize_filename(sds filename);
void sds_sanitize_filename2(sds filename);

#endif
