/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/mongoose/mongoose.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "web_server_utility.h"

#ifndef DEBUG
//embedded files for release build
#include "web_server_embedded_files.c"
#endif

bool rm_mk_dir(sds dir_name, bool create) {
    if (create == true) { 
        int rc = mkdir(dir_name, 0700);
        if (rc != 0 && errno != EEXIST) {
            LOG_ERROR("Can not create directory %s: %s", dir_name, strerror(errno));
            return false;
        }
    }
    else { 
        int rc = rmdir(dir_name);
        if (rc != 0 && errno != ENOENT) {
            LOG_ERROR("Can not remove directory %s: %s", dir_name, strerror(errno));
            return false;
        }
    }
    return true;
}

void manage_emptydir(sds varlibdir, bool pics, bool smartplaylists, bool music, bool playlists) {
    sds dir_name = sdscatfmt(sdsempty(), "%s/empty/pics", varlibdir);
    rm_mk_dir(dir_name, pics);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/smartplaylists", varlibdir);
    rm_mk_dir(dir_name, smartplaylists);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/music", varlibdir);
    rm_mk_dir(dir_name, music);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/playlists", varlibdir);
    rm_mk_dir(dir_name, playlists);
    sdsfree(dir_name);
}

void populate_dummy_hm(struct http_message *hm) {
    hm->message = mg_mk_str("");
    hm->body = mg_mk_str("");
    hm->method = mg_mk_str("GET");
    hm->uri = mg_mk_str("");
    hm->proto = mg_mk_str("HTTP/1.1");
    hm->resp_code = 200;
    hm->resp_status_msg = mg_mk_str("OK");
    hm->query_string = mg_mk_str("");
    hm->header_names[0] = mg_mk_str("");
    hm->header_values[0] = mg_mk_str("");
}

sds *split_coverimage_names(const char *coverimage_name, sds *coverimage_names, int *count) {
    int j;
    coverimage_names = sdssplitlen(coverimage_name, strlen(coverimage_name), ",", 1, count);
    for (j = 0; j < *count; j++) {
        sdstrim(coverimage_names[j], " ");
    }
    return coverimage_names;
}

void send_error(struct mg_connection *nc, int code, const char *msg) {
    sds errorpage = sdscatfmt(sdsempty(), "<html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        msg);
    mg_send_head(nc, code, sdslen(errorpage), "Content-Type: text/html");
    mg_send(nc, errorpage, sdslen(errorpage));
    sdsfree(errorpage);
    if (code >= 400) {
        LOG_ERROR(msg);
    }
}

void serve_na_image(struct mg_connection *nc, struct http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-notavailable");
}

void serve_stream_image(struct mg_connection *nc, struct http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-stream");
}

void serve_asset_image(struct mg_connection *nc, struct http_message *hm, const char *name) {
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->user_data;
    t_config *config = (t_config *) mg_user_data->config;
    
    sds asset_image = sdscatfmt(sdsempty(), "%s/pics/%s", config->varlibdir, name);
    sds mime_type;
    if (config->custom_placeholder_images == true) {
        asset_image = find_image_file(asset_image);
    }
    if (config->custom_placeholder_images == true && sdslen(asset_image) > 0) {
        mime_type = get_mime_type_by_ext(asset_image);
        mg_http_serve_file(nc, hm, asset_image, mg_mk_str(mime_type), mg_mk_str(""));
    }
    else {
        asset_image = sdscrop(asset_image);
        #ifdef DEBUG
        asset_image = sdscatfmt(asset_image, "%s/assets/%s.svg", DOC_ROOT, name);
        mime_type = get_mime_type_by_ext(asset_image);
        mg_http_serve_file(nc, hm, asset_image, mg_mk_str("image/svg+xml"), mg_mk_str(""));
        #else
        asset_image = sdscatfmt(asset_image, "/assets/%s.svg", name);
        mime_type = sdsempty();
        serve_embedded_files(nc, asset_image, hm);
        #endif
    }
    LOG_DEBUG("Serving file %s (%s)", asset_image, mime_type);
    sdsfree(asset_image);
    sdsfree(mime_type);
}

#ifndef DEBUG
struct embedded_file {
  const char *uri;
  const size_t uri_len;
  const char *mimetype;
  bool compressed;
  const unsigned char *data;
  const unsigned size;
};

bool serve_embedded_files(struct mg_connection *nc, sds uri, struct http_message *hm) {
    const struct embedded_file embedded_files[] = {
        {"/", 1, "text/html; charset=utf-8", true, index_html_data, index_html_size},
        {"/css/combined.css", 17, "text/css; charset=utf-8", true, combined_css_data, combined_css_size},
        {"/js/combined.js", 15, "application/javascript; charset=utf-8", true, combined_js_data, combined_js_size},
        {"/sw.js", 6, "application/javascript; charset=utf-8", true, sw_js_data, sw_js_size},
        {"/mympd.webmanifest", 18, "application/manifest+json", true, mympd_webmanifest_data, mympd_webmanifest_size},
        {"/assets/coverimage-notavailable.svg", 35, "image/svg+xml", true, coverimage_notavailable_svg_data, coverimage_notavailable_svg_size},
        {"/assets/MaterialIcons-Regular.woff2", 35, "font/woff2", false, MaterialIcons_Regular_woff2_data, MaterialIcons_Regular_woff2_size},
        {"/assets/coverimage-stream.svg", 29, "image/svg+xml", true, coverimage_stream_svg_data, coverimage_stream_svg_size},
        {"/assets/coverimage-loading.svg", 30, "image/svg+xml", true, coverimage_loading_svg_data, coverimage_loading_svg_size},
        {"/assets/favicon.ico", 19, "image/vnd.microsoft.icon", false, favicon_ico_data, favicon_ico_size},
        {"/assets/appicon-192.png", 23, "image/png", false, appicon_192_png_data, appicon_192_png_size},
        {"/assets/appicon-512.png", 23, "image/png", false, appicon_512_png_data, appicon_512_png_size},
        {NULL, 0, NULL, false, NULL, 0}
    };
    //decode uri
    sds uri_decoded = sdsurldecode(sdsempty(), uri, sdslen(uri), 0);
    if (sdslen(uri_decoded) == 0) {
        send_error(nc, 500, "Failed to decode uri");
        sdsfree(uri_decoded);
        return false;
    }
    //find fileinfo
    const struct embedded_file *p = NULL;
    for (p = embedded_files; p->uri != NULL; p++) {
        if (sdslen(uri_decoded) == p->uri_len && strncmp(p->uri, uri_decoded, sdslen(uri_decoded)) == 0) {
            break;
        }
    }
    sdsfree(uri_decoded);
    
    if (p != NULL && p->uri != NULL) {
        //respond with error if browser don't support compression and asset is compressed
        if (p->compressed == true) {
            struct mg_str *header_encoding = mg_get_http_header(hm, "Accept-Encoding");
            if (header_encoding == NULL || mg_strstr(mg_mk_str_n(header_encoding->p, header_encoding->len), mg_mk_str("gzip")) == NULL) {
                send_error(nc, 406, "Browser don't support gzip compression");
                return false;
            }
        }
        //send header
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                      EXTRA_HEADERS"\r\n"
                      "Content-Length: %u\r\n"
                      "Content-Type: %s\r\n"
                      "%s\r\n",
                      p->size,
                      p->mimetype,
                      (p->compressed == true ? "Content-Encoding: gzip\r\n" : "")
                 );
        //send data
        mg_send(nc, p->data, p->size);
//        mg_send(nc, "\r\n", 2);
        return true;
    }
    else {
        sds errormsg = sdscatfmt(sdsempty(), "Embedded asset %s not found", uri);
        send_error(nc, 404, errormsg);
        sdsfree(errormsg);
    }
    return false;
}
#endif
