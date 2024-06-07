/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_FILEHANDLER_H
#define MYMPD_FILEHANDLER_H

#include "dist/sds/sds.h"

#include <stdbool.h>
#include <stdio.h>

enum testdir_status {
    DIR_EXISTS = 0,
    DIR_CREATED = 1,
    DIR_CREATE_FAILED = 2,
    DIR_NOT_EXISTS = 3
};

enum try_rm_file_status {
    RM_FILE_OK = 0,
    RM_FILE_ENOENT = 1,
    RM_FILE_ERROR = 2
};

bool update_mtime(const char *filename);
bool do_chown(const char *file_path, const char *username);
time_t get_mtime(const char *filepath);

sds sds_getline(sds s, FILE *fp, size_t max, int *nread);
sds sds_getfile(sds s, const char *file_path, size_t max, bool remove_newline, bool warn, int *nread);
sds sds_getfile_from_fp(sds s, FILE *fp, size_t max, bool remove_newline, int *nread);

FILE *open_tmp_file(sds filepath);
bool rename_tmp_file(FILE *fp, sds tmp_file, bool write_rc);
bool write_data_to_file(const char *filepath, const char *data, size_t data_len);
bool rm_file(sds filepath);
int try_rm_file(sds filepath);

bool rename_file(const char *src, const char *dst);

bool testfile_read(const char *filename);
int testdir(const char *desc, const char *dir_name, bool create, bool silent);
bool is_dir(const char *dir_name);
bool clean_directory(const char *dir_name);
bool rm_directory(const char *dir_name);
bool clean_rm_directory(const char *dir_name);

#endif
