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

//public functions

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

struct mg_str mg_str_strip_parent(struct mg_str *path, int count) {
    //removes parent dir
    int i = 0;
    count++;
    while (path->len > 0) {
        if (path->ptr[0] == '/') {
            i++;
            if (i == count) {
                break;
            }
        }
        path->len--;
        path->ptr++;
    }
    return *path;
}

static const char *image_file_extensions[] = {
    "webp", "jpg", "jpeg", "png", "svg", "avif",
    "WEBP", "JPG", "JPEG", "PNG", "SVG", "AVIF",
    NULL};

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

void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers) {
    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
        "%s"
        "Content-Length: %lu\r\n\r\n",
        headers, len);
}

void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers) {
    MYMPD_LOG_DEBUG("Sending %lu bytes to %lu", (unsigned long)len, nc->id);
    webserver_send_header_ok(nc, len, headers);
    mg_send(nc, data, len);
    webserver_handle_connection_close(nc);
}

void webserver_send_header_redirect(struct mg_connection *nc, const char *location) {
    MYMPD_LOG_DEBUG("Sending 301 Moved Permanently \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n\r\n",
        location);
    webserver_handle_connection_close(nc);
}

void webserver_send_header_found(struct mg_connection *nc, const char *location) {
    MYMPD_LOG_DEBUG("Sending 302 Found \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n\r\n",
        location);
    webserver_handle_connection_close(nc);
}

void webserver_handle_connection_close(struct mg_connection *nc) {
    if (nc->label[2] == 'C') {
        MYMPD_LOG_DEBUG("Set connection %lu to is_draining", nc->id);
        nc->is_draining = 1;
    }
}

void webserver_serve_na_image(struct mg_connection *nc) {
    webserver_send_header_found(nc, "assets/coverimage-notavailable.svg");
}

void webserver_serve_stream_image(struct mg_connection *nc) {
    webserver_send_header_found(nc, "assets/coverimage-stream.svg");
}

void webserver_serve_asset_image(struct mg_connection *nc, struct mg_http_message *hm, const char *name) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    sds asset_image = sdscatfmt(sdsempty(), "%S/pics/%s", config->workdir, name);
    asset_image = webserver_find_image_file(asset_image);
    if (sdslen(asset_image) > 0) {
        const char *mime_type = get_mime_type_by_ext(asset_image);
        static struct mg_http_serve_opts s_http_server_opts;
        s_http_server_opts.root_dir = mg_user_data->browse_directory;
        s_http_server_opts.extra_headers = EXTRA_HEADERS_SAFE_CACHE;
        s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
        mg_http_serve_file(nc, hm, asset_image, &s_http_server_opts);
        webserver_handle_connection_close(nc);
        MYMPD_LOG_DEBUG("Serving custom asset image \"%s\" (%s)", asset_image, mime_type);
    }
    else {
        sdsclear(asset_image);
        #ifndef EMBEDDED_ASSETS
            asset_image = sdscatfmt(asset_image, "%s/assets/%s.svg", DOC_ROOT, name);
            static struct mg_http_serve_opts s_http_server_opts;
            s_http_server_opts.root_dir = mg_user_data->browse_directory;
            s_http_server_opts.extra_headers = EXTRA_HEADERS_SAFE_CACHE;
            s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
            mg_http_serve_file(nc, hm, asset_image, &s_http_server_opts);
            webserver_handle_connection_close(nc);
        #else
            asset_image = sdscatfmt(asset_image, "/assets/%s.svg", name);
            webserver_serve_embedded_files(nc, asset_image);
        #endif
        MYMPD_LOG_DEBUG("Serving asset image \"%s\" (image/svg+xml)", asset_image);
    }
    FREE_SDS(asset_image);
}

#ifdef EMBEDDED_ASSETS
struct embedded_file {
    const char *uri;
    const size_t uri_len;
    const char *mimetype;
    bool compressed;
    bool cache;
    const unsigned char *data;
    const unsigned size;
};

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
        {"/assets/i18n/cn-CHS.json", 24, "application/json", true, true, i18n_cn_CHS_json_data, i18n_cn_CHS_json_size},
        {"/assets/i18n/de-DE.json", 23, "application/json", true, true, i18n_de_DE_json_data, i18n_de_DE_json_size},
        {"/assets/i18n/en-US.json", 23, "application/json", true, true, i18n_en_US_json_data, i18n_en_US_json_size},
        {"/assets/i18n/ko-KR.json", 23, "application/json", true, true, i18n_ko_KR_json_data, i18n_ko_KR_json_size},
        {"/assets/i18n/nl-NL.json", 23, "application/json", true, true, i18n_nl_NL_json_data, i18n_nl_NL_json_size},
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
