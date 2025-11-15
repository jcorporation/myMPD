/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#include "compile_time.h"
#include "src/webserver/utility.h"

#include "src/lib/cache/cache_disk_images.h"
#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/webserver/response.h"

/**
 * Public functions
 */

/**
 * Matches the acl against the client ip and
 * sends an error response / drains the connection if acl is not matched
 * @param nc mongoose connection
 * @param acl acl string to check
 * @return true if acl matches, else false
 */
bool webserver_enforce_acl(struct mg_connection *nc, sds acl) {
    if (sdslen(acl) == 0) {
        return true;
    }
    if (nc->rem.is_ip6 == true) {
        //acls for ipv6 is not implemented in mongoose
        return true;
    }
    int acl_result = mg_check_ip_acl(mg_str(acl), &nc->rem);
    MYMPD_LOG_DEBUG(NULL, "Check against acl \"%s\": %d", acl, acl_result);
    if (acl_result == 1) {
        return true;
    }
    if (acl_result < 0) {
        MYMPD_LOG_ERROR(NULL, "Malformed acl \"%s\"", acl);
        return false;
    }

    sds ip = print_ip(sdsempty(), &nc->rem);
    MYMPD_LOG_ERROR(NULL, "Connection from \"%s\" blocked by ACL", ip);
    webserver_send_error(nc, 403, "Request blocked by ACL");
    nc->is_draining = 1;
    FREE_SDS(ip);

    return false;
}

/**
 * Enforces the connection limit
 * @param nc mongoose connection
 * @param connection_count connection count
 * @return true if connection count is not exceeded, else false
 */
bool webserver_enforce_conn_limit(struct mg_connection *nc, int connection_count) {
    if (connection_count > HTTP_CONNECTIONS_MAX) {
        MYMPD_LOG_DEBUG(NULL, "Connections: %d", connection_count);
        MYMPD_LOG_ERROR(NULL, "Concurrent connections limit exceeded: %d", connection_count);
        webserver_send_error(nc, 429, "Concurrent connections limit exceeded");
        nc->is_draining = 1;
        return false;
    }
    return true;
}

/**
 * Returns the mongoose connection by id
 * @param mgr mongoose mgr
 * @param id connection id
 * @return struct mg_connection* or NULL if not found
 */
struct mg_connection *get_nc_by_id(struct mg_mgr *mgr, unsigned long id) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if (nc->id == id) {
            return nc;
        }
        nc = nc->next;
    }
    return NULL;
}

/**
 * Extract arguments from query parameters
 * @param hm Http message
 * @return Parsed arguments as t_list struct.
 */
struct t_list *webserver_parse_arguments(struct mg_http_message *hm) {
    struct t_list *arguments = list_new();
    sds decoded_key = sdsempty();
    sds decoded_value = sdsempty();
    int params_count;
    sds *params = sdssplitlen(hm->query.buf, (ssize_t)hm->query.len, "&", 1, &params_count);
    for (int i = 0; i < params_count; i++) {
        int kv_count;
        sds *kv = sdssplitlen(params[i], (ssize_t)sdslen(params[i]), "=", 1, &kv_count);
        if (kv_count == 2) {
            decoded_key = sds_urldecode(decoded_key, kv[0], sdslen(kv[0]), false);
            decoded_value = sds_urldecode(decoded_value, kv[1], sdslen(kv[1]), false);
            list_push(arguments, decoded_key, 0, decoded_value, NULL);
            sdsclear(decoded_key);
            sdsclear(decoded_value);
        }
        sdsfreesplitres(kv, kv_count);
    }
    sdsfreesplitres(params, params_count);
    FREE_SDS(decoded_key);
    FREE_SDS(decoded_value);
    return arguments;
}

/**
 * Prints the ip address from a mg_addr struct
 * @param s already allocated sds string to append the ip
 * @param addr pointer to struct mg_addr
 * @return sds pointer to s
 */
sds print_ip(sds s, struct mg_addr *addr) {
    if (addr->is_ip6 == false) {
        //IPv4
        uint8_t *p = (uint8_t *)&addr->ip;
        return sdscatprintf(s, "%d.%d.%d.%d", (int) p[0], (int) p[1], (int) p[2], (int) p[3]);
    }
    //IPv6
    uint16_t *p = (uint16_t *)&addr->ip;
    return sdscatprintf(s, "[%x:%x:%x:%x:%x:%x:%x:%x]",
            mg_ntohs(p[0]), mg_ntohs(p[1]), mg_ntohs(p[2]), mg_ntohs(p[3]),
            mg_ntohs(p[4]), mg_ntohs(p[5]), mg_ntohs(p[6]), mg_ntohs(p[7]));
}

/**
 * Gets and decodes an url parameter.
 * The key will not be url decoded before matching.
 * @param query query string to parse
 * @param name name to get, you must append "=" to the name
 * @return url decoded value or NULL on error
 */
sds get_uri_param(struct mg_str *query, const char *name) {
    sds result = NULL;
    int count = 0;
    size_t name_len = strlen(name);
    sds *params = sdssplitlen(query->buf, (ssize_t)query->len, "&", 1, &count);
    for (int i = 0; i < count; i++) {
        if (strncmp(params[i], name, name_len) == 0) {
            sdsrange(params[i], (ssize_t)name_len, -1);
            result = sds_urldecode(sdsempty(), params[i], sdslen(params[i]), false);
            break;
        }
    }
    sdsfreesplitres(params, count);
    return result;
}

/**
 * Sets the partition from uri and handles errors
 * @param nc mongoose connection
 * @param hm http message
 * @param frontend_nc_data frontend nc data
 * @return true on success, else false
 */
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data) {
    sds partition = sds_urldecode(sdsempty(), hm->uri.buf, hm->uri.len, false);
    basename_uri(partition);
    FREE_SDS(frontend_nc_data->partition);
    frontend_nc_data->partition = partition;
    if (sdslen(partition) == 0) {
        //no partition identifier - close connection
        webserver_send_error(nc, 400, "No partition identifier");
        nc->is_draining = 1;
        return false;
    }
    return true;
}

/**
 * Checks the covercache and serves the image
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data pointer to mongoose configuration
 * @param type cache type: cover or thumbs
 * @param uri_decoded image uri
 * @param offset embedded image offset
 * @return true if an image is served,
 *         false if no image was found in cache
 */
bool check_imagescache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, const char *type, sds uri_decoded, int offset)
{
    sds imagescachefile = cache_disk_images_get_basename(mg_user_data->config->cachedir, type, uri_decoded, offset);
    imagescachefile = webserver_find_image_file(imagescachefile);
    if (sdslen(imagescachefile) > 0) {
        webserver_serve_file(nc, hm, EXTRA_HEADERS_IMAGE, imagescachefile);
        FREE_SDS(imagescachefile);
        return true;
    }
    MYMPD_LOG_DEBUG(NULL, "No %s cache file found", type);
    FREE_SDS(imagescachefile);
    return false;
}

/**
 * Image file extensions to detect
 */
static const char *image_file_extensions[] = {
    "webp", "jpg", "jpeg", "png", "svg", "avif",
    "WEBP", "JPG", "JPEG", "PNG", "SVG", "AVIF",
    NULL};

/**
 * Finds the first image with basefilename by trying out extensions
 * @param basefilename basefilename to append extensions
 * @return pointer to extended basefilename on success, else empty
 */
sds webserver_find_image_file(sds basefilename) {
    MYMPD_LOG_DEBUG(NULL, "Searching image file for basename \"%s\"", basefilename);
    const char **p = image_file_extensions;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%S.%s", basefilename, *p);
        if (testfile_read(testfilename) == true) {
            break;
        }
        sdsclear(testfilename);
        p++;
    }
    FREE_SDS(testfilename);
    if (*p != NULL) {
        basefilename = sdscatfmt(basefilename, ".%s", *p);
    }
    else {
        sdsclear(basefilename);
    }
    return basefilename;
}

/**
 * Finds an image in a specific subdir in dir
 * @param coverfile pointer to already allocated sds string to append the found image path
 * @param music_directory parent directory
 * @param path subdirectory
 * @param names sds array of names
 * @param names_len length of sds array
 * @return true on success, else false
 */
bool find_image_in_folder(sds *coverfile, sds music_directory, sds path, sds *names, int names_len) {
    for (int j = 0; j < names_len; j++) {
        *coverfile = sdscatfmt(*coverfile, "%S/%S/%S", music_directory, path, names[j]);
        if (strchr(names[j], '.') == NULL) {
            //basename, try extensions
            *coverfile = webserver_find_image_file(*coverfile);
        }
        if (sdslen(*coverfile) > 0 &&
            testfile_read(*coverfile) == true)
        {
            return true;
        }
        sdsclear(*coverfile);
    }
    return false;
}

/**
 * Drains the connection if connection is set to close
 * @param nc mongoose connection
 */
void webserver_handle_connection_close(struct mg_connection *nc) {
    if (nc->data[2] == 'C') {
        MYMPD_LOG_DEBUG(NULL, "Set connection %lu to is_draining", nc->id);
        nc->is_draining = 1;
    }
    nc->is_resp = 0;
}
