/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/webradio.h"

/**
 * Searches webradio favorites and WebradioDB by uri and returns the cover image uri
 * @param webradio_favorites Pointer to webradio favorites
 * @param webradiodb Pointer to WebradioDB
 * @param buffer Already allocates sds string to append the response
 * @param uri Uri to search
 * @return sds Pointer to buffer
 */
sds webserver_webradio_get_cover_uri(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb,
        sds buffer, sds uri)
{
    if (webradios_get_read_lock(webradio_favorites) == false) {
        return sdscat(buffer, "/assets/coverimage-stream");
    }
    if (webradios_get_read_lock(webradiodb) == false) {
        webradios_release_lock(webradio_favorites);
        return sdscat(buffer, "/assets/coverimage-stream");
    }

    struct t_webradio_data *webradio = webradio_by_uri(webradio_favorites, webradiodb, uri);
    if (webradio != NULL) {
        buffer = webradio_get_cover_uri(webradio, buffer);
    }
    else {
        buffer = sdscat(buffer, "/assets/coverimage-stream");
    }

    webradios_release_lock(webradio_favorites);
    webradios_release_lock(webradiodb);
    return buffer;
}

/**
 * Searches webradio favorites and WebradioDB by uri and returns an extm3u
 * @param webradio_favorites Pointer to webradio favorites
 * @param webradiodb Pointer to WebradioDB
 * @param buffer Already allocates sds string to append the response
 * @param uri Uri to search
 * @return sds Pointer to buffer
 */
sds webserver_webradio_get_extm3u(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb,
        sds buffer, sds uri)
{
    if (webradios_get_read_lock(webradio_favorites) == false) {
        return sdscat(buffer, "/assets/coverimage-stream");
    }
    if (webradios_get_read_lock(webradiodb) == false) {
        webradios_release_lock(webradio_favorites);
        return sdscat(buffer, "/assets/coverimage-stream");
    }

    buffer = webradio_get_extm3u(webradio_favorites, webradiodb, buffer, uri);

    webradios_release_lock(webradio_favorites);
    webradios_release_lock(webradiodb);
    return buffer;
}
