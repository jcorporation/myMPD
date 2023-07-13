/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/utility.h"

#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "embedded_files.c"
#endif

/**
 * Public functions
 */

/**
 * Converts a mg_str to int
 * @param str pointer to struct mg_str
 * @return parsed integer
 */
int mg_str_to_int(struct mg_str *str) {
    sds s = sdsnewlen(str->ptr, str->len);
    int i = (int)strtoimax(s, NULL, 10);
    FREE_SDS(s);
    return i;
}

/**
 * Converts a mg_str to long
 * @param str pointer to struct mg_str
 * @return parsed integer
 */
long mg_str_to_long(struct mg_str *str) {
    sds s = sdsnewlen(str->ptr, str->len);
    long l = strtol(s, NULL, 10);
    FREE_SDS(s);
    return l;
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
 * Sets the partition from uri and handles errors
 * @param nc mongoose connection
 * @param hm http message
 * @param frontend_nc_data frontend nc data
 * @return true on success, else false
 */
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data) {
    sds partition = sdsnewlen(hm->uri.ptr, hm->uri.len);
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
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 * @return NULL
 */
void *mg_user_data_free(struct t_mg_user_data *mg_user_data) {
    FREE_SDS(mg_user_data->browse_directory);
    FREE_SDS(mg_user_data->music_directory);
    sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
    sdsfreesplitres(mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
    list_clear(&mg_user_data->stream_uris);
    list_clear(&mg_user_data->session_list);
    FREE_SDS(mg_user_data->custom_booklet_image);
    FREE_SDS(mg_user_data->custom_mympd_image);
    FREE_SDS(mg_user_data->custom_na_image);
    FREE_SDS(mg_user_data->custom_stream_image);
    FREE_PTR(mg_user_data);
    return NULL;
}

/**
 * Checks the covercache and serves the image
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data pointer to mongoose configuration
 * @param uri_decoded image uri
 * @param offset embedded image offset
 * @return true if an image is served,
 *         false if waiting for mpd_client to handle request
 */
bool check_covercache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, sds uri_decoded, int offset)
{
    if (mg_user_data->config->covercache_keep_days > 0) {
        sds filename = sds_hash_sha1(uri_decoded);
        sds covercachefile = sdscatfmt(sdsempty(), "%S/%s/%S-%i", mg_user_data->config->cachedir, DIR_CACHE_COVER, filename, offset);
        FREE_SDS(filename);
        covercachefile = webserver_find_image_file(covercachefile);
        if (sdslen(covercachefile) > 0) {
            const char *mime_type = get_mime_type_by_ext(covercachefile);
            MYMPD_LOG_DEBUG(NULL, "Serving file %s (%s)", covercachefile, mime_type);
            static struct mg_http_serve_opts s_http_server_opts;
            s_http_server_opts.root_dir = mg_user_data->browse_directory;
            s_http_server_opts.extra_headers = EXTRA_HEADERS_IMAGE;
            s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
            mg_http_serve_file(nc, hm, covercachefile, &s_http_server_opts);
            webserver_handle_connection_close(nc);
            FREE_SDS(covercachefile);
            return true;
        }
        MYMPD_LOG_DEBUG(NULL, "No covercache file found");
        FREE_SDS(covercachefile);
    }
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
 * @return pointer to basefilename
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
 * Sends a http error response
 * @param nc mongoose connection
 * @param code http error code
 * @param msg the error message
 */
void webserver_send_error(struct mg_connection *nc, int code, const char *msg) {
    mg_http_reply(nc, code, "Content-Type: text/html\r\n", "<!DOCTYPE html><html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        msg);
    if (code >= 400) {
        MYMPD_LOG_ERROR(NULL, "HTTP %d: %s", code, msg);
    }
    webserver_handle_connection_close(nc);
}

/**
 * Sends a http OK reply with content-length header
 * @param nc mongoose connection
 * @param len length for the content-length header
 * @param headers extra headers to add
 */
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers) {
    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
        "%s"
        "Content-Length: %lu\r\n\r\n",
        headers, len);
}

/**
 * Sends binary data
 * @param nc mongoose connection
 * @param data data to send
 * @param len length of the data to send
 * @param headers extra headers to add
 */
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers) {
    MYMPD_LOG_DEBUG(NULL, "Sending %lu bytes to %lu", (unsigned long)len, nc->id);
    webserver_send_header_ok(nc, len, headers);
    mg_send(nc, data, len);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a 301 moved permanently header
 * @param nc mongoose connection
 * @param location destination for the redirect
 */
void webserver_send_header_redirect(struct mg_connection *nc, const char *location) {
    MYMPD_LOG_DEBUG(NULL, "Sending 301 Moved Permanently \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n\r\n",
        location);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a 302 found header
 * @param nc mongoose connection
 * @param location destination for the redirect
 */
void webserver_send_header_found(struct mg_connection *nc, const char *location) {
    MYMPD_LOG_DEBUG(NULL, "Sending 302 Found \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n\r\n",
        location);
    webserver_handle_connection_close(nc);
}

/**
 * Replies to preflighted requests in CORS
 * @param nc mongoose connection
 */
void webserver_send_cors_reply(struct mg_connection *nc) {
    MYMPD_LOG_DEBUG(NULL, "Sending 204 No Content to %lu", nc->id);
    mg_printf(nc, "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, HEAD, OPTIONS\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Access-Control-Expose-Headers: *\r\n"
        EXTRA_HEADERS_CACHE
        "\r\n");
    webserver_handle_connection_close(nc);
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

/**
 * Redirects to the not available image
 * @param nc mongoose connection
 */
void webserver_serve_na_image(struct mg_connection *nc) {
    struct t_mg_user_data *mg_user_data = nc->mgr->userdata;
    if (sdslen(mg_user_data->custom_na_image) == 0) {
        webserver_send_header_found(nc, "/assets/coverimage-notavailable.svg");
    }
    else {
        sds uri = sdscatfmt(sdsempty(), "/browse/%s/%S", DIR_WORK_PICS_THUMBS, mg_user_data->custom_na_image);
        webserver_send_header_found(nc, uri);
        FREE_SDS(uri);
    }
}

/**
 * Redirects to the stream image
 * @param nc mongoose connection
 */
void webserver_serve_stream_image(struct mg_connection *nc) {
    struct t_mg_user_data *mg_user_data = nc->mgr->userdata;
    if (sdslen(mg_user_data->custom_stream_image) == 0) {
        webserver_send_header_found(nc, "/assets/coverimage-stream.svg");
    }
    else {
        sds uri = sdscatfmt(sdsempty(), "/browse/%s/%S", DIR_WORK_PICS_THUMBS, mg_user_data->custom_stream_image);
        webserver_send_header_found(nc, uri);
        FREE_SDS(uri);
    }
}

/**
 * Redirects to the mympd image
 * @param nc mongoose connection
 */
void webserver_serve_mympd_image(struct mg_connection *nc) {
    struct t_mg_user_data *mg_user_data = nc->mgr->userdata;
    if (sdslen(mg_user_data->custom_mympd_image) == 0) {
        webserver_send_header_found(nc, "/assets/coverimage-mympd.svg");
    }
    else {
        sds uri = sdscatfmt(sdsempty(), "/browse/%s/%S", DIR_WORK_PICS_THUMBS, mg_user_data->custom_mympd_image);
        webserver_send_header_found(nc, uri);
        FREE_SDS(uri);
    }
}

/**
 * Redirects to the booklet image
 * @param nc mongoose connection
 */
void webserver_serve_booklet_image(struct mg_connection *nc) {
    struct t_mg_user_data *mg_user_data = nc->mgr->userdata;
    if (sdslen(mg_user_data->custom_booklet_image) == 0) {
        webserver_send_header_found(nc, "/assets/coverimage-booklet.svg");
    }
    else {
        sds uri = sdscatfmt(sdsempty(), "/browse/%s/%S", DIR_WORK_PICS_THUMBS, mg_user_data->custom_booklet_image);
        webserver_send_header_found(nc, uri);
        FREE_SDS(uri);
    }
}

#ifdef MYMPD_EMBEDDED_ASSETS
/**
 * Struct holding embedded file information
 */
struct embedded_file {
    const char *uri;
    const char *mimetype;
    bool compressed;
    bool cache;
    const unsigned char *data;
    const unsigned size;
};

/**
 * Serves the embedded files
 * @param nc mongoose connection
 * @param uri uri to server
 * @return true on success, else false
 */
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri) {
    const struct embedded_file embedded_files[] = {
        {"/", "text/html; charset=utf-8", true, false, index_html_data, index_html_size},
        {"/css/combined.css", "text/css; charset=utf-8", true, false, combined_css_data, combined_css_size},
        {"/js/combined.js", "application/javascript; charset=utf-8", true, false, combined_js_data, combined_js_size},
        {"/sw.js", "application/javascript; charset=utf-8", true, false, sw_js_data, sw_js_size},
        {"/mympd.webmanifest", "application/manifest+json", true, false, mympd_webmanifest_data, mympd_webmanifest_size},
        {"/assets/coverimage-notavailable.svg", "image/svg+xml", true, true, coverimage_notavailable_svg_data, coverimage_notavailable_svg_size},
        {"/assets/MaterialIcons-Regular.woff2", "font/woff2", false, true, MaterialIcons_Regular_woff2_data, MaterialIcons_Regular_woff2_size},
        {"/assets/coverimage-stream.svg", "image/svg+xml", true, true, coverimage_stream_svg_data, coverimage_stream_svg_size},
        {"/assets/coverimage-booklet.svg", "image/svg+xml", true, true, coverimage_booklet_svg_data, coverimage_booklet_svg_size},
        {"/assets/coverimage-mympd.svg", "image/svg+xml", true, true, coverimage_mympd_svg_data, coverimage_mympd_svg_size},
        {"/assets/mympd-background-dark.svg", "image/svg+xml", true, true, mympd_background_dark_svg_data, mympd_background_dark_svg_size},
        {"/assets/mympd-background-light.svg", "image/svg+xml", true, true, mympd_background_light_svg_data, mympd_background_light_svg_size},
        {"/assets/appicon-192.png", "image/png", false, true, appicon_192_png_data, appicon_192_png_size},
        {"/assets/appicon-512.png", "image/png", false, true, appicon_512_png_data, appicon_512_png_size},
        {"/assets/ligatures.json", "application/json", true, true, ligatures_json_data, ligatures_json_size},
        #ifdef I18N_de_DE
            {"/assets/i18n/de-DE.json", "application/json", true, true, i18n_de_DE_json_data, i18n_de_DE_json_size},
        #endif
        #ifdef I18N_en_US
        {"/assets/i18n/en-US.json", "application/json", true, true, i18n_en_US_json_data, i18n_en_US_json_size},
        #endif
        #ifdef I18N_es_AR
        {"/assets/i18n/es-AR.json", "application/json", true, true, i18n_es_AR_json_data, i18n_es_AR_json_size},
        #endif
        #ifdef I18N_es_ES
        {"/assets/i18n/es-ES.json", "application/json", true, true, i18n_es_ES_json_data, i18n_es_ES_json_size},
        #endif
        #ifdef I18N_es_VE
        {"/assets/i18n/es-VE.json", "application/json", true, true, i18n_es_VE_json_data, i18n_es_VE_json_size},
        #endif
        #ifdef I18N_fi_FI
        {"/assets/i18n/fi-FI.json", "application/json", true, true, i18n_fi_FI_json_data, i18n_fi_FI_json_size},
        #endif
        #ifdef I18N_fr_FR
        {"/assets/i18n/fr-FR.json", "application/json", true, true, i18n_fr_FR_json_data, i18n_fr_FR_json_size},
        #endif
        #ifdef I18N_it_IT
        {"/assets/i18n/it-IT.json", "application/json", true, true, i18n_it_IT_json_data, i18n_it_IT_json_size},
        #endif
        #ifdef I18N_ja_JP
        {"/assets/i18n/ja-JP.json", "application/json", true, true, i18n_ja_JP_json_data, i18n_ja_JP_json_size},
        #endif
        #ifdef I18N_ko_KR
        {"/assets/i18n/ko-KR.json", "application/json", true, true, i18n_ko_KR_json_data, i18n_ko_KR_json_size},
        #endif
        #ifdef I18N_nl_NL
        {"/assets/i18n/nl-NL.json", "application/json", true, true, i18n_nl_NL_json_data, i18n_nl_NL_json_size},
        #endif
        #ifdef I18N_pl_PL
        {"/assets/i18n/pl-PL.json", "application/json", true, true, i18n_pl_PL_json_data, i18n_pl_PL_json_size},
        #endif
        #ifdef I18N_ru_RU
        {"/assets/i18n/ru-RU.json", "application/json", true, true, i18n_ru_RU_json_data, i18n_ru_RU_json_size},
        #endif
        #ifdef I18N_zh_Hans
        {"/assets/i18n/zh-Hans.json", "application/json", true, true, i18n_zh_Hans_json_data, i18n_zh_Hans_json_size},
        #endif
        {NULL, NULL, false, false, NULL, 0}
    };
    //decode uri
    sds uri_decoded = sds_urldecode(sdsempty(), uri, sdslen(uri), false);
    if (sdslen(uri_decoded) == 0) {
        webserver_send_error(nc, 500, "Failed to decode uri");
        FREE_SDS(uri_decoded);
        return false;
    }
    //find fileinfo
    const struct embedded_file *p = NULL;
    for (p = embedded_files; p->uri != NULL; p++) {
        if (strcmp(p->uri, uri_decoded) == 0){
            break;
        }
    }

    if (p->uri != NULL) {
        //send header
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
            EXTRA_HEADERS_SAFE
            "%s"
            "Content-Length: %d\r\n"
            "Content-Type: %s\r\n"
            "%s\r\n",
            (p->cache == true ? EXTRA_HEADERS_CACHE : ""),
            p->size,
            p->mimetype,
            (p->compressed == true ? EXTRA_HEADER_CONTENT_ENCODING : "")
        );
        //send data
        mg_send(nc, p->data, p->size);
        webserver_handle_connection_close(nc);
        FREE_SDS(uri_decoded);
        return true;
    }
    sds errormsg = sdscatfmt(sdsempty(), "Embedded asset \"%S\" not found", uri_decoded);
    webserver_send_error(nc, 404, errormsg);
    FREE_SDS(errormsg);
    FREE_SDS(uri_decoded);
    return false;
}
#endif
