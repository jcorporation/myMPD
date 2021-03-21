/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_UTILITY_H__
#define __WEB_SERVER_UTILITY_H__

#define EXTRA_HEADERS_DIR "Content-Security-Policy: default-src 'none'; "\
                          "style-src 'self' 'unsafe-inline'; font-src 'self'; script-src 'self' 'unsafe-inline'; img-src 'self' data:; "\
                          "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                          "media-src *; frame-ancestors *; base-uri 'none';\r\n"\
                          "X-Content-Type-Options: nosniff\r\n"\
                          "X-XSS-Protection: 1; mode=block\r\n"\
                          "X-Frame-Options: deny\r\n"

#define EXTRA_HEADERS "Content-Security-Policy: default-src 'none'; "\
                      "style-src 'self'; font-src 'self'; script-src 'self'; img-src 'self' data:; "\
                      "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                      "media-src *; frame-ancestors *; base-uri 'none';\r\n"\
                      "X-Content-Type-Options: nosniff\r\n"\
                      "X-XSS-Protection: 1; mode=block\r\n"\
                      "X-Frame-Options: deny\r\n"

#define EXTRA_HEADERS_CACHE "Cache-Control: max-age=604800\r\n"

#define DIRECTORY_LISTING_CSS "h1{top:0;font-size:inherit;font-weight:inherit}address{bottom:0;font-style:normal}"\
      "h1,address{background-color:#343a40;color:#f8f9fa;padding:1rem;position:fixed;"\
      "box-sizing:border-box;width:100%;margin-top:0}body{margin:5rem 0;background-color:#f7f7f7;"\
      "color:#212529;font-family:sans-serif;font-size:1rem;font-weight:400;line-height:1.5}"\
      "table{border-collapse:collapse;margin:1rem}th{border-bottom:2px solid #dee2e6;"\
      "border-top:1px solid #dee2e6;text-align:left;padding:.3rem;font-family:inherit}"\
      "td{text-align:left;padding:.3rem;font-family:inherit;border-bottom:1px solid #dee2e6}"\
      "td:last-child{text-align:right}a,a:visited,a:active{color:#212529;text-decoration:none}"\
      "a:hover{text-decoration:underline}"

typedef struct t_mg_user_data {
    void *config; //pointer to mympd config
    sds browse_document_root;
    sds pics_document_root;
    sds music_directory;
    sds smartpls_document_root;
    sds playlist_directory;
    sds *coverimage_names;
    int coverimage_names_len;
    bool feat_library;
    bool feat_mpd_albumart;
    int connection_count;
} t_mg_user_data;

#ifndef DEBUG
bool serve_embedded_files(struct mg_connection *nc, sds uri, struct mg_http_message *hm);
#endif
void manage_emptydir(sds varlibdir, bool pics, bool smartplaylists, bool music, bool playlists);
sds *split_coverimage_names(const char *coverimage_name, sds *coverimage_names, int *count);
void send_error(struct mg_connection *nc, int code, const char *msg);
void serve_na_image(struct mg_connection *nc, struct mg_http_message *hm);
void serve_plaintext(struct mg_connection *nc, const char *text);
void serve_stream_image(struct mg_connection *nc, struct mg_http_message *hm);
void serve_asset_image(struct mg_connection *nc, struct mg_http_message *hm, const char *name);
void populate_dummy_hm(struct mg_http_message *hm);
void http_send_header_ok(struct mg_connection *nc, size_t len, const char *headers);
void http_send_header_redirect(struct mg_connection *nc, const char *location);
int check_ip_acl(const char *acl, uint32_t remote_ip);
struct mg_str mg_str_strip_parent(struct mg_str *path, int count);
#endif
