/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/utility.h"

#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Private definitions
 */
static sds get_mympd_host(sds mpd_host, sds http_host);
static sds get_local_ip(void);

/**
 * Public functions
 */

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
            MYMPD_LOG_DEBUG(NULL, "Path \"%s\" is a virtual cuesheet directory", filename);
            is_cue_file = true;
        }
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Error accessing \"%s\"", full_path);
    }
    FREE_SDS(full_path);
    return is_cue_file;
}

/**
 * Checks if uri is a remote uri or a local file
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
 * List of special mympd uris to resolv
 */
struct t_mympd_uris {
    const char *uri;       //!< mympd uri to resolv
    const char *resolved;  //!< resolved path
};

const struct t_mympd_uris mympd_uris[] = {
    {"mympd://webradio/", "/browse/"DIR_WORK_WEBRADIOS"/"},
    {"mympd://", "/"},
    {NULL,                NULL}
};

/**
 * Resolves mympd:// uris to the real myMPD host address and respects the mympd_uri config option
 * @param uri uri to resolv
 * @param mpd_host mpd host
 * @param config pointer to config struct
 * @return resolved uri
 */
sds resolv_mympd_uri(sds uri, sds mpd_host, struct t_config *config) {
    const struct t_mympd_uris *p = NULL;
    for (p = mympd_uris; p->uri != NULL; p++) {
        size_t len = strlen(p->uri);
        if (strncmp(uri, p->uri, len) == 0) {
            sdsrange(uri, (ssize_t)len, -1);
            sds new_uri = sdsempty();
            if (strcmp(config->mympd_uri, "auto") != 0) {
                //use defined uri
                new_uri = sdscatfmt(new_uri, "%S%s%S", config->mympd_uri, p->resolved, uri);
                FREE_SDS(uri);
                return new_uri;
            }
            //calculate uri
            sds host = get_mympd_host(mpd_host, config->http_host);
            if (config->http == false) {
                //use ssl port
                new_uri = sdscatfmt(new_uri, "https://%S:%i%s%S", host, config->ssl_port, p->resolved, uri);
            }
            else {
                new_uri = sdscatfmt(new_uri, "http://%S:%i%s%S", host, config->http_port, p->resolved, uri);
            }
            FREE_SDS(uri);
            FREE_SDS(host);
            return new_uri;
        }
    }
    //uri could not be resolved
    return uri;
}

/**
 * Checks for IPv6 support by searching for an IPv6 adress on all interfaces
 * @return true on IPv6 support, else false
 */
bool get_ipv6_support(void) {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    bool rc = false;
    errno = 0;
    if (getifaddrs(&ifaddr) == -1) {
        MYMPD_LOG_ERROR(NULL, "Can not get list of interface ip addresses");
        MYMPD_LOG_ERRNO(NULL, errno);
        return rc;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != NULL &&
            ifa->ifa_addr->sa_family == AF_INET6)
        {
            rc = true;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return rc;
}

/**
 * Private functions
 */

/**
 * Gets the listening address of the embedded webserver
 * @param mpd_host mpd_host config setting
 * @param http_host http_host config setting
 * @return address of the embedded webserver as sds string
 */
static sds get_mympd_host(sds mpd_host, sds http_host) {
    if (strcmp(http_host, "0.0.0.0") != 0 &&
        strcmp(http_host, "[::]") != 0)
    {
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
        MYMPD_LOG_ERROR(NULL, "Can not get list of interface ip addresses");
        MYMPD_LOG_ERRNO(NULL, errno);
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
                MYMPD_LOG_ERROR(NULL, "getnameinfo() failed: %s\n", gai_strerror(s));
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
