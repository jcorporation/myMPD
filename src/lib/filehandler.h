/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief File handling
 */

#ifndef MYMPD_FILEHANDLER_H
#define MYMPD_FILEHANDLER_H

#include "dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

/**
 * Status for a directory
 */
enum testdir_status {
    DIR_EXISTS = 0,
    DIR_CREATED = 1,
    DIR_CREATE_FAILED = 2,
    DIR_NOT_EXISTS = 3
};

/**
 * Status of file remove function
 */
enum try_rm_file_status {
    RM_FILE_OK = 0,
    RM_FILE_ENOENT = 1,
    RM_FILE_ERROR = 2
};

/**
 * Status of sds_getfile functions
 */
enum getfile_status {
    FILE_IS_EMPTY = 0,
    FILE_NOT_EXISTS = -1,
    FILE_TO_BIG = -2
};

bool update_mtime(const char *filename);
time_t get_mtime(const char *filepath);

sds sds_getline(sds s, FILE *fp, size_t max, int *nread);
sds sds_getfile(sds s, const char *file_path, size_t max, bool remove_newline, bool warn, int *nread);
sds sds_getfile_from_fp(sds s, FILE *fp, size_t max, bool remove_newline, int *nread);

bool create_tmp_file(const char *filepath);
FILE *open_tmp_file(sds filepath);
bool rename_tmp_file(FILE *fp, sds tmp_file, bool write_rc);
bool write_data_to_file(const char *filepath, const char *data, size_t data_len);
bool rm_file(const char *filepath);
int try_rm_file(const char *filepath);

bool rename_file(const char *src, const char *dst);

bool testfile_read(const char *filename);
int testdir(const char *desc, const char *dir_name, bool create, bool silent);
bool is_dir(const char *dir_name);
bool clean_directory(const char *dir_name);
bool rm_directory(const char *dir_name);
bool clean_rm_directory(const char *dir_name);

#endif
