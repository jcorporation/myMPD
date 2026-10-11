/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief File handling
 */

#include "compile_time.h"
#include "src/lib/filehandler.h"

#include "dist/sds/sds.h"
#include "src/lib/log.h"
#include "src/lib/sds/sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

/**
 * Updates the timestamp of a file
 * @param filename file to update the timestamp
 * @return true on success, else false
 */
bool update_mtime(const char *filename) {
    time_t mtime = time(NULL);
    struct utimbuf new_times;
    new_times.actime = mtime;
    new_times.modtime = mtime;
    if (utime(filename, &new_times) == 0) {
        return true;
    }
    return false;
}

/**
 * Returns the modification time of a file
 * @param filepath filepath
 * @return time_t modification time
 */
time_t get_mtime(const char *filepath) {
    // Verify with lstat() to prevent TOCTTOU attacks and symlink following
    struct stat sb;
    errno = 0;
    if (lstat(filepath, &sb) != 0) {
        // File disappeared or is inaccessible, skip it
        MYMPD_LOG_ERROR(NULL, "Error getting mtime for \"%s\", file not accessible", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        return 0;
    }
    if (S_ISREG(sb.st_mode) == false) {
        // File type changed or is a symlink/directory, skip it
        MYMPD_LOG_ERROR(NULL, "Error getting mtime for \"%s\", file is not a regular file", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        return 0;
    }
    return sb.st_mtime;
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
            MYMPD_LOG_NOTICE(NULL, "%s: \"%s\"", desc, dir_name);
        }
        //directory exists
        return DIR_EXISTS;
    }

    if (create == true) {
        errno = 0;
        if (mkdir(dir_name, 0770) != 0) {
            MYMPD_LOG_ERROR(NULL, "%s: creating \"%s\" failed", desc, dir_name);
            MYMPD_LOG_ERRNO(NULL, errno);
            //directory does not exist and creating it failed
            return DIR_CREATE_FAILED;
        }
        if (silent == false) {
            MYMPD_LOG_NOTICE(NULL, "%s: \"%s\" created", desc, dir_name);
        }
        //directory successfully created
        return DIR_CREATED;
    }
    if (silent == false) {
        MYMPD_LOG_ERROR(NULL, "%s: \"%s\" does not exist", desc, dir_name);
    }
    //directory does not exist
    return DIR_NOT_EXISTS;
}

/**
 * Checks if dir_name is really a directory entry
 * @param dir_name directory path to check
 * @return true if it is a directory, else false
 */
bool is_dir(const char *dir_name) {
    struct stat status;
    errno = 0;
    if (lstat(dir_name, &status) != 0) {
        MYMPD_LOG_ERROR(NULL, "Error getting status for \"%s\"", dir_name);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return S_ISDIR(status.st_mode);
}

/**
 * Checks if a path is a regular file
 * @param file_name file path to check
 * @return true if it is a regular file, else false
 */
bool is_file(const char *file_name) {
    struct stat status;
    errno = 0;
    if (lstat(file_name, &status) != 0) {
        MYMPD_LOG_ERROR(NULL, "Error getting status for \"%s\"", file_name);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return S_ISREG(status.st_mode);
}

/**
 * Checks if a path is a regular file
 * Logs no errors, just returns false if file does not exist or is not a regular file
 * @param file_name file path to check
 * @return true if it is a regular file, else false
 */
bool is_file_silent(const char *file_name) {
    struct stat status;
    errno = 0;
    if (lstat(file_name, &status) != 0) {
        return false;
    }
    return S_ISREG(status.st_mode);
}

/**
 * Creates a file in specified path if it does not exists.
 * @param filepath filepath to create
 * @return bool true on success, else false
 */
bool create_tmp_file(const char *filepath) {
    errno = 0;
    int fd = open(filepath, O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, S_IRWXU);
    if (fd < 0) {
        MYMPD_LOG_ERROR(NULL, "Can not open file descriptor \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    close(fd);
    return true;
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
        MYMPD_LOG_ERROR(NULL, "Can not open tmp file descriptor \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        return NULL;
    }
    errno = 0;
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open tmp file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        close(fd);
    }
    return fp;
}

/**
 * Closes the tmp file and moves it to its destination name
 * This is done by removing the last 7 characters from the tmp_file.
 * See open_tmp_file for corresponding open function.
 * @param fp FILE pointer
 * @param tmp_file tmp file to close and move
 * @param write_rc if false tmp file will be removed
 * @return true on success else false
 */
bool rename_tmp_file(FILE *fp, sds tmp_file, bool write_rc) {
    if (fclose(fp) != 0 ||
        write_rc == false)
    {
        MYMPD_LOG_ERROR(NULL, "Error writing data to file \"%s\"", tmp_file);
        rm_file(tmp_file);
        return false;
    }
    errno = 0;
    //filepath is tmp_file without .XXXXXX suffix
    sds filepath = sdscatlen(sdsempty(), tmp_file, sdslen(tmp_file) - 7);
    if (is_file(tmp_file) == false) {
        return false;
    }
    if (rename(tmp_file, filepath) == -1) {
        MYMPD_LOG_ERROR(NULL, "Rename file from \"%s\" to \"%s\" failed", tmp_file, filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        rm_file(tmp_file);
        FREE_SDS(filepath);
        return false;
    }
    FREE_SDS(filepath);
    return true;
}

/**
 * Renames a file. src and dst must be in the same filesystem.
 * @param src source filename
 * @param dst destination filename
 * @return true on success, else false
 */
bool rename_file(const char *src, const char *dst) {
    if (is_file(src) == false) {
        return false;
    }
    if (rename(src, dst) == -1) {
        MYMPD_LOG_ERROR(NULL, "Rename file from \"%s\" to \"%s\" failed", src, dst);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return true;
}

/**
 * Removes a file and reports all errors
 * @param filepath filepath to remove
 * @return true on success else false
 */
bool rm_file(const char *filepath) {
    if (is_file(filepath) == false) {
        return false;
    }
    errno = 0;
    if (unlink(filepath) != 0) {
        MYMPD_LOG_ERROR(NULL, "Error removing file \"%s\"", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
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
int try_rm_file(const char *filepath) {
    if (is_file(filepath) == false) {
        return RM_FILE_ENOENT;
    }
    errno = 0;
    if (unlink(filepath) != 0) {
        if (errno == ENOENT) {
            MYMPD_LOG_DEBUG(NULL, "File \"%s\" does not exist", filepath);
            return RM_FILE_ENOENT;
        }
        MYMPD_LOG_ERROR(NULL, "Error removing file \"%s\"", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
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
bool write_data_to_file(const char *filepath, const char *data, size_t data_len) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s.XXXXXX", filepath);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }
    size_t written = fwrite(data, 1, data_len, fp);
    bool write_rc = written == data_len ? true : false;
    bool rc = rename_tmp_file(fp, tmp_file, write_rc);
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
        MYMPD_LOG_ERROR(NULL, "Error opening directory \"%s\"", dir_name);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }

    struct dirent *next_file;
    sds filepath = sdsempty();
    while ((next_file = readdir(directory)) != NULL ) {
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%s/%s", dir_name, next_file->d_name);
        // Verify file is a regular file (prevents TOCTTOU and symlink attacks)
        if (is_file(filepath) == false) {
            continue;
        }
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
        MYMPD_LOG_ERROR(NULL, "Error removing directory \"%s\"", dir_name);
        MYMPD_LOG_ERRNO(NULL, errno);
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
