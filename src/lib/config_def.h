/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Centtral myMPD configuration definitions
 */

#ifndef MYMPD_LIB_CONFIG_DEF_H
#define MYMPD_LIB_CONFIG_DEF_H

#include "compile_time.h"

#include "dist/sds/sds.h"
#include "mpd/tag.h"

#include <stdbool.h>

/**
 * Modes for the album cache
 */
enum album_modes {
    ALBUM_MODE_SIMPLE = 0,
    ALBUM_MODE_ADV
};

/**
 * Holds config for the album cache
 */
struct t_albums_config {
    enum album_modes mode;        //!< enable advanced albums
    enum mpd_tag_type group_tag;  //!< additional group tag for albums
};

/**
 * Static myMPD configuration read at startup from files / environment
 */
struct t_config {
    bool bootstrap;                 //!< true if bootstrap command line option is set
    bool custom_cert;               //!< false if myMPD uses the self generated certificates
    bool first_startup;             //!< true if it is the first myMPD startup (not configurable)
    bool http;                      //!< enable listening on plain http_port
    bool log_to_syslog;             //!< enable syslog logging
    bool save_caches;               //!< true = save caches between restart
    bool ssl;                       //!< enable listening on ssl_port
    bool stickers;                  //!< enable sticker support
    bool stickers_pad_int;          //!< enable the padding of integer sticker values
    bool webradiodb;                //!< enable webradiodb support
    struct t_albums_config albums;  //!< album specific config
    int cache_cover_keep_days;      //!< expiration time for cover cache files in days
    int cache_lyrics_keep_days;     //!< expiration time for lyrics cache files in days
    int cache_thumbs_keep_days;     //!< expiration time for thumbs cache files in days
    int cache_misc_keep_days;       //!< expiration time for misc cache files in days
    int http_port;                  //!< http port to listen
    int loglevel;                   //!< loglevel
    int ssl_port;                   //!< https port to listen
    sds acl;                        //!< IPv4 ACL string
    sds cachedir;                   //!< cache directory
    sds http_host;                  //!< ip to bind the webserver
    sds mympd_uri;                  //!< uri to resolve mympd:// uris
    sds pin_hash;                   //!< hash of the pin
    sds scriptacl;                  //!< IPv4 ACL string for the /api/script endpoint
    sds ssl_cert;                   //!< filename of the certificate
    sds ssl_key;                    //!< filename of the private key
    sds ssl_san;                    //!< additional names for SAN of the self generated certificate
    sds user;                       //!< username to drop privileges
    sds workdir;                    //!< working directory
    time_t startup_time;            //!< unix timestamp of startup (not configurable)
};

#endif
