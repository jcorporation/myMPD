/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_WEBRADIO_H
#define MYMPD_LIB_WEBRADIO_H

#include "dist/rax/rax.h"
#include "dist/sds/sds.h"
#include "src/lib/config_def.h"
#include "src/lib/list.h"

/**
 * Webradio Types
 */
enum webradio_type {
    WEBRADIO_WEBRADIODB,
    WEBRADIO_FAVORITE
};

/**
 * Wraps the indexes of webradios
 */
struct t_webradios {
    rax *db;         //!< Index by name
    rax *idx_uris;   //!< Index by uri
};

/**
 * Holds the webradio data
 */
struct t_webradio_data {
    sds name;                   //!< Station name
    sds image;                  //!< Station image
    sds homepage;               //!< Homepage
    sds country;                //!< Country
    sds state;                  //!< State or region
    sds description;            //!< Short description
    struct t_list uris;         //!< List of Uris (uri, bitrate and codec)
    struct t_list genres;       //!< List of genres
    struct t_list languages;    //!< List of languages
    enum webradio_type type;    //!< Type of the webradio
};

struct t_webradio_data *webradio_data_new(enum webradio_type type);
void webradio_data_free(struct t_webradio_data *data);
sds webradio_get_cover_uri(struct t_webradio_data *webradio, sds buffer);
const char *webradio_type_name(enum webradio_type type);
sds webradio_to_extm3u(struct t_webradio_data *webradio, sds buffer, const char *uri);

struct t_webradios *webradios_new(void);
void webradios_free(struct t_webradios *webradios);
bool webradios_save_to_disk(struct t_config *config, struct t_webradios *webradios, const char *filename);
bool webradios_read_from_disk(struct t_config *config, struct t_webradios *webradios, const char *filename, enum webradio_type type);

#endif
