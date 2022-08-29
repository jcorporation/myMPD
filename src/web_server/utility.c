/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"

#ifdef EMBEDDED_ASSETS
//embedded files for release build
#include "embedded_files.c"
#endif

/**
 * Public functions
 */

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
    FREE_SDS(mg_user_data->stream_uri);
    list_clear(&mg_user_data->session_list);
    FREE_PTR(mg_user_data);
    return NULL;
}

/**
 * Image file extensions to detect
 */
static const char *image_file_extensions[] = {
    "webp", "jpg", "jpeg", "png", "svg", "avif",
    "WEBP", "JPG", "JPEG", "PNG", "SVG", "AVIF",
    NULL};

/**
 * Finds the first image with basefilename by trying out extentions
 * @param basefilename basefilename to append extensions
 * @return pointer to basefilename
 */
sds webserver_find_image_file(sds basefilename) {
    MYMPD_LOG_DEBUG("Searching image file for basename \"%s\"", basefilename);
    const char **p = image_file_extensions;
    sds testfilename = sdsempty();
    while (*p != NULL) {
        testfilename = sdscatfmt(testfilename, "%S.%s", basefilename, *p);
        if (access(testfilename, F_OK) == 0) { /* Flawfinder: ignore */
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
        MYMPD_LOG_ERROR("HTTP %d: %s", code, msg);
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
    MYMPD_LOG_DEBUG("Sending %lu bytes to %lu", (unsigned long)len, nc->id);
    webserver_send_header_ok(nc, len, headers);
    mg_send(nc, data, len);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a 301 moved permamently header
 * @param nc mongoose connection
 * @param location destination for the redirect
 */
void webserver_send_header_redirect(struct mg_connection *nc, const char *location) {
    MYMPD_LOG_DEBUG("Sending 301 Moved Permanently \"%s\" to %lu", location, nc->id);
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
    MYMPD_LOG_DEBUG("Sending 302 Found \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n\r\n",
        location);
    webserver_handle_connection_close(nc);
}

/**
 * Drains the connection if connection is set to close
 * @param nc mongoose connection
 */
void webserver_handle_connection_close(struct mg_connection *nc) {
    if (nc->label[2] == 'C') {
        MYMPD_LOG_DEBUG("Set connection %lu to is_draining", nc->id);
        nc->is_draining = 1;
    }
    nc->is_resp = 0;
}

/**
 * Redirects to the not available image
 * @param nc mongoose connection
 */
void webserver_serve_na_image(struct mg_connection *nc) {
    webserver_send_header_found(nc, "assets/coverimage-notavailable.svg");
}

/**
 * Redirects to the default stream image
 * @param nc mongoose connection
 */
void webserver_serve_stream_image(struct mg_connection *nc) {
    webserver_send_header_found(nc, "assets/coverimage-stream.svg");
}

#ifdef EMBEDDED_ASSETS
/**
 * Struct holding embedded file information
 */
struct embedded_file {
    const char *uri;
    const size_t uri_len;
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
        {"/", 1, "text/html; charset=utf-8", true, false, index_html_data, index_html_size},
        {"/css/combined.css", 17, "text/css; charset=utf-8", true, false, combined_css_data, combined_css_size},
        {"/js/combined.js", 15, "application/javascript; charset=utf-8", true, false, combined_js_data, combined_js_size},
        {"/sw.js", 6, "application/javascript; charset=utf-8", true, false, sw_js_data, sw_js_size},
        {"/mympd.webmanifest", 18, "application/manifest+json", true, false, mympd_webmanifest_data, mympd_webmanifest_size},
        {"/assets/coverimage-notavailable.svg", 35, "image/svg+xml", true, true, coverimage_notavailable_svg_data, coverimage_notavailable_svg_size},
        {"/assets/MaterialIcons-Regular.woff2", 35, "font/woff2", true, true, MaterialIcons_Regular_woff2_data, MaterialIcons_Regular_woff2_size},
        {"/assets/coverimage-stream.svg", 29, "image/svg+xml", true, true, coverimage_stream_svg_data, coverimage_stream_svg_size},
        {"/assets/coverimage-loading.svg", 30, "image/svg+xml", true, true, coverimage_loading_svg_data, coverimage_loading_svg_size},
        {"/assets/coverimage-booklet.svg", 30, "image/svg+xml", true, true, coverimage_booklet_svg_data, coverimage_booklet_svg_size},
        {"/assets/coverimage-mympd.svg", 28, "image/svg+xml", true, true, coverimage_mympd_svg_data, coverimage_mympd_svg_size},
        {"/assets/mympd-background-dark.svg", 33, "image/svg+xml", true, true, mympd_background_dark_svg_data, mympd_background_dark_svg_size},
        {"/assets/mympd-background-light.svg", 34, "image/svg+xml", true, true, mympd_background_light_svg_data, mympd_background_light_svg_size},
        {"/assets/appicon-192.png", 23, "image/png", false, true, appicon_192_png_data, appicon_192_png_size},
        {"/assets/appicon-512.png", 23, "image/png", false, true, appicon_512_png_data, appicon_512_png_size},
        {"/assets/ligatures.json", 22, "application/json", true, true, ligatures_json_data, ligatures_json_size},
        #ifdef I18N_de_DE
            {"/assets/i18n/de-DE.json", 23, "application/json", true, true, i18n_de_DE_json_data, i18n_de_DE_json_size},
        #endif
        #ifdef I18N_en_US
        {"/assets/i18n/en-US.json", 23, "application/json", true, true, i18n_en_US_json_data, i18n_en_US_json_size},
        #endif
        #ifdef I18N_es_VE
        {"/assets/i18n/es-VE.json", 23, "application/json", true, true, i18n_es_VE_json_data, i18n_es_VE_json_size},
        #endif
        #ifdef I18N_fi_FI
        {"/assets/i18n/fi-FI.json", 23, "application/json", true, true, i18n_fi_FI_json_data, i18n_fi_FI_json_size},
        #endif
        #ifdef I18N_fr_FR
        {"/assets/i18n/fr-FR.json", 23, "application/json", true, true, i18n_fr_FR_json_data, i18n_fr_FR_json_size},
        #endif
        #ifdef I18N_it_IT
        {"/assets/i18n/it-IT.json", 23, "application/json", true, true, i18n_it_IT_json_data, i18n_it_IT_json_size},
        #endif
        #ifdef I18N_ko_KR
        {"/assets/i18n/ko-KR.json", 23, "application/json", true, true, i18n_ko_KR_json_data, i18n_ko_KR_json_size},
        #endif
        #ifdef I18N_nl_NL
        {"/assets/i18n/nl-NL.json", 23, "application/json", true, true, i18n_nl_NL_json_data, i18n_nl_NL_json_size},
        #endif
        #ifdef I18N_zh_CN
        {"/assets/i18n/zh-CN.json", 24, "application/json", true, true, i18n_zh_CN_json_data, i18n_zh_CN_json_size},
        #endif
        {NULL, 0, NULL, false, false, NULL, 0}
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
        if (sdslen(uri_decoded) == p->uri_len &&
            strncmp(p->uri, uri_decoded, sdslen(uri_decoded)) == 0)
        {
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
    else {
        sds errormsg = sdscatfmt(sdsempty(), "Embedded asset \"%S\" not found", uri_decoded);
        webserver_send_error(nc, 404, errormsg);
        FREE_SDS(errormsg);
    }
    FREE_SDS(uri_decoded);
    return false;
}
#endif
