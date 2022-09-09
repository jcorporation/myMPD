/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "log.h"
#include "sds_extras.h"

#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

//private definitions
static sds get_local_ip(void);

//public functions

/**
 * Gets an environment variable and checks its length
 * @param env_var environment variable name
 * @param max_len maximum length
 * @return environment variable value or NULL if it is not set or to long
 */
const char *getenv_check(const char *env_var, size_t max_len) {
    const char *env_value = getenv(env_var); /* Flawfinder: ignore */
    if (env_value == NULL) {
        MYMPD_LOG_DEBUG("Environment variable \"%s\" not set", env_var);
        return NULL;
    }
    if (env_value[0] == '\0') {
        MYMPD_LOG_DEBUG("Environment variable \"%s\" is empty", env_var);
        return NULL;
    }
    if (strlen(env_value) > max_len) {
        MYMPD_LOG_WARN("Environment variable \"%s\" is too long", env_var);
        return NULL;
    }
    MYMPD_LOG_INFO("Got environment variable \"%s\" with value \"%s\"", env_var, env_value);
    return env_value;
}

/**
 * Sleep function
 * @param msec milliseconds to sleep
 */
void my_msleep(long msec) {
    struct timespec ts = {
        .tv_sec = (time_t)(msec / 1000),
        .tv_nsec = (msec % 1000L) * 1000000L
    };
    nanosleep(&ts, NULL);
}

/**
 * Checks if the filename is a mpd virtual cue sheet directory
 * MPD uses the cue filename as path, we simply check if the filename is a file or not
 * @param music_directory mpd music directory
 * @param filename filename to check
 * @return true if it is a cue file else false
 */
bool is_virtual_cuedir(sds music_directory, sds filename) {
    sds full_path = sdscatfmt(sdsempty(), "%S/%S", music_directory, filename);
    bool is_cue_file = false;
    struct stat stat_buf;
    if (stat(full_path, &stat_buf) == 0) {
        if (S_ISREG(stat_buf.st_mode)) {
            MYMPD_LOG_DEBUG("Path \"%s\" is a virtual cuesheet directory", filename);
            is_cue_file = true;
        }
    }
    else {
        MYMPD_LOG_ERROR("Error accessing \"%s\"", full_path);
    }
    FREE_SDS(full_path);
    return is_cue_file;
}

/**
 * Checks if uri is realy uri or a local file
 * @param uri uri to check
 * @return true it is a uri else false
 */
bool is_streamuri(const char *uri) {
    if (uri != NULL &&
        strstr(uri, "://") != NULL)
    {
        return true;
    }
    return false;
}

/**
 * Gets the extension of a filename
 * @param filename filename to get extension from
 * @return pointer to the extension
 */
const char *get_extension_from_filename(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }
    char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        //skip dot
        ext++;
        if (ext[0] == '\0') {
            return NULL;
        }
    }
    return ext;
}

/**
 * Calculates the basename for files and uris
 * for files the path is removed
 * for uris the query string and hash is removed
 * @param uri sds string to modify in place
 */
void basename_uri(sds uri) {
    size_t len = sdslen(uri);
    if (len == 0) {
        return;
    }

    if (strstr(uri, "://") == NULL) {
        //filename, remove path
        for (int i = (int)len - 1; i >= 0; i--) {
            if (uri[i] == '/') {
                sdsrange(uri, i + 1, -1);
                break;
            }
        }
        return;
    }

    //uri, remove query and hash
    for (size_t i = 0; i < len; i++) {
        if (uri[i] == '#' ||
            uri[i] == '?')
        {
            sdssubstr(uri, 0, i);
            break;
        }
    }
}

/**
 * Strips all slashes from the end
 * @param dirname sds string to strip
 */
void strip_slash(sds dirname) {
    char *sp = dirname;
    char *ep = dirname + sdslen(dirname) - 1;
    while(ep >= sp &&
          *ep == '/')
    {
        ep--;
    }
    size_t len = (size_t)(ep-sp)+1;
    dirname[len] = '\0';
    sdssetlen(dirname, len);
}

/**
 * Removes the file extension
 * @param filename sds string to remove the extension
 */
void strip_file_extension(sds filename) {
    char *sp = filename;
    char *ep = filename + sdslen(filename) - 1;
    while (ep >= sp) {
        if (*ep == '.') {
            size_t len = (size_t)(ep-sp);
            filename[len] = '\0';
            sdssetlen(filename, len);
            break;
        }
        ep --;
    }
}

/**
 * Replaces the file extension
 * @param filename sds string to replace the extension
 * @param ext new file extension
 * @return newly allocated sds string with new file extension
 */
sds replace_file_extension(sds filename, const char *ext) {
    sds newname = sdsdup(filename);
    strip_file_extension(newname);
    if (sdslen(newname) == 0) {
        return newname;
    }
    newname = sdscatfmt(newname, ".%s", ext);
    return newname;
}

static const char *invalid_filename_chars = "<>/.:?&$!#=;\a\b\f\n\r\t\v\\|";

/**
 * Replaces invalid filename characters with "_"
 * @param filename sds string to sanitize
 */
void sanitize_filename(sds filename) {
    const size_t len = strlen(invalid_filename_chars);
    for (size_t i = 0; i < len; i++) {
        for (size_t j = 0; j < sdslen(filename); j++) {
            if (filename[j] == invalid_filename_chars[i]) {
                filename[j] = '_';
            }
        }
    }
}

/**
 * Gets the listening address of the embedded webserver
 * @param mpd_host mpd_host config setting
 * @param http_host http_host config setting
 * @return address of the embedded webserver as sds string
 */
sds get_mympd_host(sds mpd_host, sds http_host) {
    if (strcmp(http_host, "0.0.0.0") != 0) {
        //host defined in configuration
        return sdsdup(http_host);
    }
    if (strncmp(mpd_host, "/", 1) == 0) {
        //local socket - use localhost
        return sdsnew("localhost");
    }
    //get local ip
    return get_local_ip();
}

//private functions

/**
 * Gets the ip address of the first interface
 * @return ip address as sds string
 */
static sds get_local_ip(void) {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    char host[NI_MAXHOST];

    errno = 0;
    if (getifaddrs(&ifaddr) == -1) {
        MYMPD_LOG_ERROR("Can not get list of inteface ip addresses");
        MYMPD_LOG_ERRNO(errno);
        return sdsempty();
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL ||
            strcmp(ifa->ifa_name, "lo") == 0)
        {
            continue;
        }
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET ||
            family == AF_INET6)
        {
            int s = getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                MYMPD_LOG_ERROR("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }
            char *crap;
            // remove zone info from ipv6
            char *ip = strtok_r(host, "%", &crap);
            sds ip_str = sdsnew(ip);
            freeifaddrs(ifaddr);
            return ip_str;
        }
    }
    freeifaddrs(ifaddr);
    return sdsempty();
}
