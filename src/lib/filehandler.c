/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "filehandler.h"

#include "log.h"
#include "sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Getline function that trims whitespace characters
 * @param s an already allocated sds string
 * @param fp a file descriptor to read from
 * @param max max line length to read
 * @return GETLINE_OK on success,
 *         GETLINE_EMPTY for empty line,
 *         GETLINE_TOO_LONG for too long line
 */
int sds_getline(sds *s, FILE *fp, size_t max) {
    sdsclear(*s);
    size_t i = 0;
    for (;;) {
        int c = fgetc(fp);
        if (c == EOF) {
            sdstrim(*s, "\r \t");
            if (sdslen(*s) > 0) {
                return GETLINE_OK;
            }
            return GETLINE_EMPTY;
        }
        if (c == '\n') {
            sdstrim(*s, "\r \t");
            return GETLINE_OK;
        }
        if (i < max) {
            *s = sds_catchar(*s, (char)c);
            i++;
        }
        else {
            MYMPD_LOG_ERROR("Line is too long, max length is %lu", (unsigned long)max);
            return GETLINE_TOO_LONG;
        }
    }
}

/**
 * Getline function that first trims whitespace characters and adds a newline afterwards
 * @param s an already allocated sds string
 * @param fp a file descriptor to read from
 * @param max max line length to read
 * @return GETLINE_OK on success,
 *         GETLINE_EMPTY for empty line,
 *         GETLINE_TOO_LONG for too long line
 */
int sds_getline_n(sds *s, FILE *fp, size_t max) {
    int rc = sds_getline(s, fp, max);
    *s = sdscat(*s, "\n");
    return rc;
}

/**
 * Reads a whole file in the sds string s from *fp
 * Removes whitespace characters from start and end
 * @param s an already allocated sds string that should hold the file content
 * @param fp FILE pointer to read
 * @param max maximum bytes to read
 * @param remove_newline removes CR/LF if true
 * @return GETLINE_OK on success,
 *         GETLINE_EMPTY for empty file,
 *         GETLINE_TOO_LONG for too long file
 */
int sds_getfile(sds *s, FILE *fp, size_t max, bool remove_newline) {
    sdsclear(*s);
    size_t i = 0;
    for (;;) {
        int c = fgetc(fp);
        if (c == EOF) {
            sdstrim(*s, "\r \t\n");
            MYMPD_LOG_DEBUG("Read %lu bytes from file", (unsigned long)sdslen(*s));
            if (sdslen(*s) > 0) {
                return GETLINE_OK;
            }
            return GETLINE_EMPTY;
        }
        if (remove_newline == true &&
            (c == '\n' || c == '\r'))
        {
            continue;
        }
        if (i < max) {
            *s = sds_catchar(*s, (char)c);
            i++;
        }
        else {
            MYMPD_LOG_ERROR("File is too long, max length is %lu", (unsigned long)max);
            return GETLINE_TOO_LONG;
        }
    }
}

/**
 * Checks if a filename can be opened read-only
 * @param filename filename to check
 * @return true on success, else false
 */
bool testfile_read(const char *filename) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Error opening file ro \"%s\"", filename);
            MYMPD_LOG_ERRNO(errno);
        }
        return false;
    }
    (void) fclose(fp);
    return true;
}

/**
 * Checks if dir exists
 * @param desc descriptive name
 * @param dir_name directory path to check
 * @param create true creates the directory
 * @param silent true to report only errors
 * @return enum testdir_status
 */
int testdir(const char *desc, const char *dir_name, bool create, bool silent) {
    DIR* dir = opendir(dir_name);
    if (dir != NULL) {
        closedir(dir);
        if (silent == false) {
            MYMPD_LOG_NOTICE("%s: \"%s\"", desc, dir_name);
        }
        //directory exists
        return DIR_EXISTS;
    }

    if (create == true) {
        errno = 0;
        if (mkdir(dir_name, 0770) != 0) {
            MYMPD_LOG_ERROR("%s: creating \"%s\" failed", desc, dir_name);
            MYMPD_LOG_ERRNO(errno);
            //directory does not exist and creating it failed
            return DIR_CREATE_FAILED;
        }
        MYMPD_LOG_NOTICE("%s: \"%s\" created", desc, dir_name);
        //directory successfully created
        return DIR_CREATED;
    }

    MYMPD_LOG_ERROR("%s: \"%s\" does not exist", desc, dir_name);
    //directory does not exist
    return DIR_NOT_EXISTS;
}

/**
 * Opens a temporary file for write using mkstemp
 * @param filepath filepath to open, e.g. /tmp/test.XXXXXX
 *                 XXXXXX is replaced with a random string
 * @return FILE pointer
 */
FILE *open_tmp_file(sds filepath) {
    errno = 0;
    int fd = mkstemp(filepath);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(errno);
        return NULL;
    }
    errno = 0;
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(errno);
    }
    return fp;
}

/**
 * Closes the tmp file and moves it to its destination name
 * @param fp FILE pointer
 * @param tmp_file tmp file to close and move
 * @param filepath destination path
 * @param write_rc if false tmp file will be removed
 * @return true on success else false
 */
bool rename_tmp_file(FILE *fp, sds tmp_file, sds filepath, bool write_rc) {
    if (fclose(fp) != 0 ||
        write_rc == false)
    {
        MYMPD_LOG_ERROR("Error writing data to file \"%s\"", tmp_file);
        rm_file(tmp_file);
        FREE_SDS(tmp_file);
        return false;
    }
    errno = 0;
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(errno);
        rm_file(tmp_file);
        return false;
    }
    return true;
}

/**
 * Removes a file and reports all errors
 * @param filepath filepath to remove
 * @return true on success else false
 */
bool rm_file(sds filepath) {
    errno = 0;
    if (unlink(filepath) != 0) {
        MYMPD_LOG_ERROR("Error removing file \"%s\"", filepath);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }
    return true;
}

/**
 * Removes a file and ignores none existing error
 * @param filepath filepath to remove
 * @return RM_FILE_ENOENT if file does not exist
 *         RM_FILE_ERROR error from unlink call
 *         RM_FILE_OK file was removed
 */
int try_rm_file(sds filepath) {
    errno = 0;
    if (unlink(filepath) != 0) {
        if (errno == ENOENT) {
            MYMPD_LOG_DEBUG("File \"%s\" does not exist", filepath);
            return RM_FILE_ENOENT;
        }
        MYMPD_LOG_ERROR("Error removing file \"%s\"", filepath);
        MYMPD_LOG_ERRNO(errno);
        return RM_FILE_ERROR;
    }
    return RM_FILE_OK;
}

/**
 * Writes data to a file
 * @param filepath filepath to write to
 * @param data data to write
 * @param data_len data length to write
 * @return true on success else false
 */
bool write_data_to_file(sds filepath, const char *data, size_t data_len) {
    sds tmp_file = sdscatfmt(sdsempty(), "%S.XXXXXX", filepath);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    size_t written = fwrite(data, 1, data_len, fp);
    bool write_rc = written == data_len ? true : false;
    bool rc = rename_tmp_file(fp, tmp_file, filepath, write_rc);
    FREE_SDS(tmp_file);
    return rc;
}

/**
 * Removes all regular files from a directory
 * @param dir_name directory to cleanup
 * @return true on success, else false
 */
bool clean_directory(const char *dir_name) {
    errno = 0;
    DIR *directory = opendir(dir_name);
    if (directory == NULL) {
        MYMPD_LOG_ERROR("Error opening directory \"%s\"", dir_name);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }

    struct dirent *next_file;
    sds filepath = sdsempty();
    while ((next_file = readdir(directory)) != NULL ) {
        if (next_file->d_type != DT_REG) {
            continue;
        }
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%s/%s", dir_name, next_file->d_name);
        bool rc = rm_file(filepath);
        if (rc == false) {
            FREE_SDS(filepath);
            closedir(directory);
            return false;
        }
    }
    closedir(directory);
    FREE_SDS(filepath);
    return true;
}

/**
 * Removes a directory and reports errors
 * @param dir_name directory to remove
 * @return true on success, else false
 */
bool rm_directory(const char *dir_name) {
    errno = 0;
    if (rmdir(dir_name) != 0) {
        MYMPD_LOG_ERROR("Error removing directory \"%s\"", dir_name);
        MYMPD_LOG_ERRNO(errno);
        return false;
    }
    return true;
}

/**
 * Shortcut for clean_directory and rm_directory
 * @param dir_name directory to cleanup and remove
 * @return true on success, else false
 */
bool clean_rm_directory(const char *dir_name) {
    bool rc = clean_directory(dir_name);
    if (rc == true) {
        rc = rm_directory(dir_name);
    }
    return rc;
}
