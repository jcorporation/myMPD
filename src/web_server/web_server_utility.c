/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "web_server_utility.h"

#include "../log.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "mympd_config_defs.h"

#ifndef DEBUG
//embedded files for release build
#include "web_server_embedded_files.c"
#endif

//private definitions
static bool rm_mk_dir(sds dir_name, bool create);
static bool check_ipv4_acl(const char *acl, const uint32_t remote_ip);

/*
static bool check_ipv6_acl(const char *acl, const uint8_t remote_ip[16]);
static bool compare_ipv6_with_mask(const uint8_t addr1[16], const int addr2[16], 
    const int mask[16]);
static void create_ipv6_mask(int *netmask, int mask);
*/

//public functions
void free_mg_user_data(struct t_mg_user_data *mg_user_data) {
    sdsfree(mg_user_data->browse_document_root);
    sdsfree(mg_user_data->pics_document_root);
    sdsfree(mg_user_data->smartpls_document_root);
    sdsfree(mg_user_data->music_directory);
    sdsfree(mg_user_data->playlist_directory);
    sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
    sdsfree(mg_user_data->stream_uri);
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

bool check_ip_acl(const char *acl, struct mg_addr *peer) {
    /*
    if (peer->is_ip6 == true) {
        //ipv6
        return check_ipv6_acl(acl, peer->ip6);
    }
    */
    //ipv4
    uint32_t remote_ip = ntohl(peer->ip);
    return check_ipv4_acl(acl, remote_ip);
}

void manage_emptydir(sds workdir, bool pics, bool smartplaylists, bool music, bool playlists) {
    sds dir_name = sdscatfmt(sdsempty(), "%s/empty/pics", workdir);
    rm_mk_dir(dir_name, pics);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/smartplaylists", workdir);
    rm_mk_dir(dir_name, smartplaylists);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/music", workdir);
    rm_mk_dir(dir_name, music);
    
    dir_name = sdscrop(dir_name);
    dir_name = sdscatfmt(dir_name, "%s/empty/playlists", workdir);
    rm_mk_dir(dir_name, playlists);
    sdsfree(dir_name);
}

//create an empty dummy message struct, used for async responses
void populate_dummy_hm(struct mg_http_message *hm) {
    hm->message = mg_str("");
    hm->body = mg_str("");
    hm->method = mg_str("GET");
    hm->uri = mg_str("");
    hm->query = mg_str("");
    hm->proto = mg_str("HTTP/1.1");
    //add accept-encoding header to deliver gziped embedded files
    //browsers without gzip support are not supported by myMPD
    hm->headers[0].name = mg_str("Accept-Encoding");
    hm->headers[0].value = mg_str("gzip");
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
    mg_http_reply(nc, code, "Content-Type: text/html\n\n", "<html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        msg);
    if (code >= 400) {
        MYMPD_LOG_ERROR(msg);
    }
}

void http_send_header_ok(struct mg_connection *nc, size_t len, const char *headers) {
    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
      "%s"
      "Content-Length: %lu\r\n\r\n",
      headers, len);
}

void http_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers) {
    http_send_header_ok(nc, len, headers);
    mg_send(nc, data, len);
}

void http_send_header_redirect(struct mg_connection *nc, const char *location) {
    mg_printf(nc, "HTTP/1.1 301 Moved Permanently\r\n"
      "Location: %s\r\n"
      "Content-Length: 0\r\n\r\n", 
      location);
}

void serve_na_image(struct mg_connection *nc, struct mg_http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-notavailable");
}

void serve_stream_image(struct mg_connection *nc, struct mg_http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-stream");
}

void serve_asset_image(struct mg_connection *nc, struct mg_http_message *hm, const char *name) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;
    
    sds asset_image = sdscatfmt(sdsempty(), "%s/pics/%s", config->workdir, name);
    sds mime_type;
    asset_image = find_image_file(asset_image);
    if (sdslen(asset_image) > 0) {
        mime_type = get_mime_type_by_ext(asset_image);
        mg_http_serve_file(nc, hm, asset_image, mime_type, EXTRA_HEADERS_CACHE);
    }
    else {
        asset_image = sdscrop(asset_image);
        #ifdef DEBUG
        asset_image = sdscatfmt(asset_image, "%s/assets/%s.svg", DOC_ROOT, name);
        mime_type = get_mime_type_by_ext(asset_image);
        mg_http_serve_file(nc, hm, asset_image, "image/svg+xml", EXTRA_HEADERS_CACHE);
        #else
        asset_image = sdscatfmt(asset_image, "/assets/%s.svg", name);
        mime_type = sdsempty();
        serve_embedded_files(nc, asset_image, hm);
        #endif
    }
    MYMPD_LOG_DEBUG("Serving file \"%s\" (%s)", asset_image, mime_type);
    sdsfree(asset_image);
    sdsfree(mime_type);
}

#ifndef DEBUG
struct embedded_file {
    const char *uri;
    const size_t uri_len;
    const char *mimetype;
    bool compressed;
    bool cache;
    const unsigned char *data;
    const unsigned size;
};

bool serve_embedded_files(struct mg_connection *nc, sds uri, struct mg_http_message *hm) {
    const struct embedded_file embedded_files[] = {
        {"/", 1, "text/html; charset=utf-8", true, false, index_html_data, index_html_size},
        {"/css/combined.css", 17, "text/css; charset=utf-8", true, false, combined_css_data, combined_css_size},
        {"/js/combined.js", 15, "application/javascript; charset=utf-8", true, false, combined_js_data, combined_js_size},
        {"/sw.js", 6, "application/javascript; charset=utf-8", true, false, sw_js_data, sw_js_size},
        {"/mympd.webmanifest", 18, "application/manifest+json", true, false, mympd_webmanifest_data, mympd_webmanifest_size},
        {"/assets/coverimage-notavailable.svg", 35, "image/svg+xml", true, true, coverimage_notavailable_svg_data, coverimage_notavailable_svg_size},
        {"/assets/MaterialIcons-Regular.woff2", 35, "font/woff2", false, true, MaterialIcons_Regular_woff2_data, MaterialIcons_Regular_woff2_size},
        {"/assets/coverimage-stream.svg", 29, "image/svg+xml", true, true, coverimage_stream_svg_data, coverimage_stream_svg_size},
        {"/assets/coverimage-loading.svg", 30, "image/svg+xml", true, true, coverimage_loading_svg_data, coverimage_loading_svg_size},
        {"/assets/coverimage-booklet.svg", 30, "image/svg+xml", true, true, coverimage_booklet_svg_data, coverimage_booklet_svg_size},
        {"/assets/coverimage-mympd.svg", 28, "image/svg+xml", true, true, coverimage_mympd_svg_data, coverimage_mympd_svg_size},
        {"/assets/mympd-background-dark.svg", 33, "image/svg+xml", true, true, mympd_background_dark_svg_data, mympd_background_dark_svg_size},
        {"/assets/mympd-background-light.svg", 34, "image/svg+xml", true, true, mympd_background_light_svg_data, mympd_background_light_svg_size},
        {"/assets/mympd-background-default.svg", 36, "image/svg+xml", true, true, mympd_background_default_svg_data, mympd_background_default_svg_size},
        {"/assets/favicon.ico", 19, "image/vnd.microsoft.icon", false, true, favicon_ico_data, favicon_ico_size},
        {"/assets/appicon-192.png", 23, "image/png", false, true, appicon_192_png_data, appicon_192_png_size},
        {"/assets/appicon-512.png", 23, "image/png", false, true, appicon_512_png_data, appicon_512_png_size},
        {NULL, 0, NULL, false, false, NULL, 0}
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
    
    if (p->uri != NULL) {
        //respond with error if browser don't support compression and asset is compressed
        if (p->compressed == true) {
            struct mg_str *header_encoding = mg_http_get_header(hm, "Accept-Encoding");
            if (header_encoding == NULL || mg_strstr(*header_encoding, mg_str_n("gzip", 4)) == NULL) {
                nc->is_draining = 1;
                send_error(nc, 406, "Browser does not support gzip compression");
                return false;
            }
        }
        //send header
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
                      EXTRA_HEADERS
                      "%s"
                      "Content-Length: %u\r\n"
                      "Content-Type: %s\r\n"
                      "%s\r\n",
                      (p->cache == true ? EXTRA_HEADERS_CACHE : ""),
                      p->size,
                      p->mimetype,
                      (p->compressed == true ? "Content-Encoding: gzip\r\n" : "")
                 );
        //send data
        mg_send(nc, p->data, p->size);
        return true;
    }
    else {
        sds errormsg = sdscatfmt(sdsempty(), "Embedded asset \"%s\" not found", uri);
        send_error(nc, 404, errormsg);
        sdsfree(errormsg);
    }
    return false;
}
#endif

//private functions
static bool rm_mk_dir(sds dir_name, bool create) {
    if (create == true) { 
        errno = 0;
        int rc = mkdir(dir_name, 0700);
        if (rc != 0 && errno != EEXIST) {
            MYMPD_LOG_ERROR("Can not create directory \"%s\"", dir_name);
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
    }
    else { 
        errno = 0;
        int rc = rmdir(dir_name);
        if (rc != 0 && errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not remove directory \"%s\"", dir_name);
            MYMPD_LOG_ERRNO(errno);
            return false;
        }
    }
    return true;
}

static bool check_ipv4_acl(const char *acl, uint32_t remote_ip) {
    bool allowed = false;
    int count;
    sds *tokens = sdssplitlen(acl, strlen(acl), ",", 1, &count);
    for (int i = 0; i < count; i++) {
        if (strstr(tokens[i], ":") != NULL) {
            //ipv6 skip
            continue;
        }
        int flag = (unsigned char) tokens[i][0];
        char *acl_str = tokens[i];
        acl_str++;
        char *mask_str;
        char *net_str = strtok_r(acl_str, "/", &mask_str);
        if (net_str == NULL || mask_str == NULL) {
            continue;
        }
        uint32_t mask = strtoimax(mask_str, NULL, 10);
        if (mask == 0) {
            //mask of 0 matches always
            allowed = flag == '+' ? true : false;        
            continue;
        }
        if (mask > 32) {
            //invalid mask
            continue;
        }
        uint32_t net;
        if (inet_pton(AF_INET, net_str, &net) != 1) {
            MYMPD_LOG_WARN("Invalid acl entry: \"%s\"", tokens[i]);
            continue;
        }
        uint32_t net_ip = ntohl(net);        
        uint32_t mask_bits = 0xffffffffU << (32 - mask);
        MYMPD_LOG_DEBUG("remote ip: %u, acl: %u", (remote_ip & mask_bits), (net_ip & mask_bits));
        if ((remote_ip & mask_bits) == (net_ip & mask_bits)) {
            allowed = flag == '+' ? true : false;        
        }
    }
    sdsfreesplitres(tokens, count);
    return allowed;
}

/*
static bool check_ipv6_acl(const char *acl, const uint8_t remote_ip[16]) {
    bool allowed = false;
    int count;
    sds *tokens = sdssplitlen(acl, strlen(acl), ",", 1, &count);
    for (int i = 0; i < count; i++) {
        if (strstr(tokens[i], ":") == NULL) {
            //ipv4 skip
            continue;
        }
        int flag = tokens[i][0];
        char *acl_str = tokens[i];
        acl_str++;
        char *mask_str;
        char *net_str = strtok_r(acl_str, "/", &mask_str);
        uint32_t mask = strtoimax(mask_str, NULL, 10);
        if (mask == 0) {
            //mask of 0 matches always
            allowed = flag == '+' ? true : false;        
            continue;
        }
        else if (mask > 128) {
            //invalid mask
            continue;
        }
        int net[16];
        if (inet_pton(AF_INET6, net_str, &net) != 1) {
            MYMPD_LOG_WARN("Invalid acl entry: \"%s\"", tokens[i]);
            continue;
        }
        
        int mask_struct[16];
        create_ipv6_mask(&mask_struct[0], mask);
        if (compare_ipv6_with_mask(remote_ip, net, mask_struct) == true) {
            allowed = flag == '+' ? true : false;        
        }
    }
    sdsfreesplitres(tokens, count);
    return allowed;
}

static bool compare_ipv6_with_mask(const uint8_t addr1[16], const int addr2[16], 
    const int mask[16])
{
    int masked[16];
	for (unsigned i = 0; i < 16; i++) {
	    masked[i] = addr2[i];
	    masked[i] &= mask[i];
    }
	return memcmp(addr1, &masked, sizeof(int[16])) == 0 ? true : false;
}

static void create_ipv6_mask(int *netmask, int mask) {
	memset(netmask, 0, sizeof(int[16]));
	int *p_netmask = netmask;
	while (8 < mask) {
		*p_netmask = 0xff;
		p_netmask++;
		mask -= 8;
	}
	if (mask != 0) {
		*p_netmask = htonl(0xff << (8 - mask));
	}
}
*/
