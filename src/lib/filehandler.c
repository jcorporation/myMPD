/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/filehandler.h"

#include "src/lib/log.h"
#include "src/lib/passwd.h"
#include "src/lib/sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Sets the owner of a file and group to the primary group of the user
 * @param file_path file to change ownership
 * @param username new owner username
 * @return true on success else false
 */
bool do_chown(const char *file_path, const char *username) {
    errno = 0;
    int fd = open(file_path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        MYMPD_LOG_ERROR(NULL, "Can't open \"%s\"", file_path);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }

    errno = 0;
    struct stat status;
    if (lstat(file_path, &status) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can't get status for \"%s\"", file_path);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }

    struct passwd pwd;
    if (get_passwd_entry(&pwd, username) == NULL) {
        MYMPD_LOG_ERROR(NULL, "User \"%s\" does not exist", username);
        return false;
    }

    if (status.st_uid == pwd.pw_uid &&
        status.st_gid == pwd.pw_gid)
    {
        //owner and group already set
        close(fd);
        return true;
    }

    errno = 0;
    int rc = fchown(fd, pwd.pw_uid, pwd.pw_gid); /* Flawfinder: ignore */
    close(fd);
    if (rc == -1) {
        MYMPD_LOG_ERROR(NULL, "Can't chown \"%s\" to \"%s\"", file_path, username);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    MYMPD_LOG_INFO(NULL, "Changed ownership of \"%s\" to \"%s\"", file_path, username);
    return true;
}

/**
 * Returns the modification time of a file
 * @param filepath filepath
 * @return time_t modification time
 */
time_t get_mtime(const char *filepath) {
    struct stat status;
    errno = 0;
    if (stat(filepath, &status) != 0) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR(NULL, "Error getting mtime for \"%s\"", filepath);
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        else {
            MYMPD_LOG_DEBUG(NULL, "File \"%s\" does not exist", filepath);
        }
        return 0;
    }
    return status.st_mtime;
}

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
        *nread = -1;
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
            *nread = -2;
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
 * Checks if a filename can be opened read-only
 * @param filename filename to check
 * @return true on success, else false
 */
bool testfile_read(const char *filename) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR(NULL, "Error opening file ro \"%s\"", filename);
            MYMPD_LOG_ERRNO(NULL, errno);
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
 * Checks if dir_name is realy a directory entry
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
 * Opens a temporary file for write using mkstemp
 * @param filepath filepath to open, e.g. /tmp/test.XXXXXX
 *                 XXXXXX is replaced with a random string
 * @return FILE pointer
 */
FILE *open_tmp_file(sds filepath) {
    errno = 0;
    int fd = mkstemp(filepath);
    if (fd < 0) {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
        return NULL;
    }
    errno = 0;
    FILE *fp = fdopen(fd, "w");
    if (fp == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\" for write", filepath);
        MYMPD_LOG_ERRNO(NULL, errno);
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
bool rm_file(sds filepath) {
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
int try_rm_file(sds filepath) {
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
bool write_data_to_file(sds filepath, const char *data, size_t data_len) {
    sds tmp_file = sdscatfmt(sdsempty(), "%S.XXXXXX", filepath);
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
