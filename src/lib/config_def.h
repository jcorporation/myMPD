/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD configuration definitions
 */

#ifndef MYMPD_LIB_CONFIG_DEF_H
#define MYMPD_LIB_CONFIG_DEF_H

#include "compile_time.h"

#include "dist/sds/sds.h"
#include "mpd/tag.h"
#include "src/lib/validate.h"

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
 * Config items
 */
enum config_item {
    CI_ACL = 0,
    CI_ALBUM_GROUP_TAG,
    CI_ALBUM_MODE,
    CI_CA_CERT_STORE,
    CI_CACHE_COVER_KEEP_DAYS,
    CI_CACHE_HTTP_KEEP_DAYS,
    CI_CACHE_LYRICS_KEEP_DAYS,
    CI_CACHE_MISC_KEEP_DAYS,
    CI_CACHE_THUMBS_KEEP_DAYS,
    CI_CERT_CHECK,
    CI_CUSTOM_CERT,
    CI_CUSTOM_CSS,
    CI_CUSTOM_JS,
    CI_HTTP,
    CI_HTTP_HOST,
    CI_HTTP_PORT,
    CI_LOGLEVEL,
    CI_MYMPD_URI,
    CI_PIN_HASH,
    CI_SAVE_CACHES,
    CI_SCRIPTACL,
    CI_SCRIPTS_EXTERNAL,
    CI_SSL,
    CI_SSL_CERT,
    CI_SSL_KEY,
    CI_SSL_PORT,
    CI_SSL_SAN,
    CI_STICKERS,
    CI_STICKERS_PAD_INT,
    CI_WEBRADIODB,
    CI_COUNT
};

/**
 * Config item types
 */
enum config_item_type {
    CIT_B,  //!< Bool
    CIT_I,  //!< Integer
    CIT_S   //!< SDS string
};

/**
 * Default config values
 */
struct t_config_value_default {
    enum config_item_type t;  //!< Config item type
    union {
        const char *s;        //!< SDS string
        int i;                //!< Integer
        bool b;               //!< Bool
    };
};

/**
 * Config values
 */
struct t_config_value {
    enum config_item_type t;  //!< Config item type
    union {
        sds s;                //!< SDS string
        int i;                //!< Integer
        bool b;               //!< Bool
    };
};

/**
 * Config defaults
 */
struct t_config_default {
    const char *file;                     //!< Config filename
    struct t_config_value_default value;  //!< Default config value 
    int min;                              //!< Minimum value for integer
    int max;                              //!< Maximum value for integer
    validate_callback vcb;                //!< Validation callback for strings
};

/**
 * Static myMPD configuration read at startup from files / environment
 */
struct t_config {
    time_t startup_time;            //!< unix timestamp of startup (not configurable)
    // Command line options
    bool bootstrap;                 //!< true if bootstrap command line option is set
    bool log_to_syslog;             //!< enable syslog logging
    sds cachedir;                   //!< cache directory
    sds workdir;                    //!< working directory

    // Configuration
    bool cert_check;                //!< enable certificate checking for outbound http connections
    bool custom_cert;               //!< false if myMPD uses the self generated certificates
    bool http;                      //!< enable listening on plain http_port
    bool save_caches;               //!< true = save caches between restart
    bool ssl;                       //!< enable listening on ssl_port
    bool stickers;                  //!< enable sticker support
    bool stickers_pad_int;          //!< enable the padding of integer sticker values
    bool webradiodb;                //!< enable webradiodb support
    bool scripts_external;          //!< allow execution of external scripts
    int cache_cover_keep_days;      //!< expiration time for cover cache files in days
    int cache_http_keep_days;       //!< expiration time for HTTP cache files in days
    int cache_lyrics_keep_days;     //!< expiration time for lyrics cache files in days
    int cache_misc_keep_days;       //!< expiration time for misc cache files in days
    int cache_thumbs_keep_days;     //!< expiration time for thumbs cache files in days
    int http_port;                  //!< http port to listen
    int loglevel;                   //!< loglevel
    int ssl_port;                   //!< https port to listen
    sds acl;                        //!< IPv4 ACL string
    sds album_mode;                 //!< Album mode
    sds album_group_tag;            //!< Album group tag
    sds ca_certs;                   //!< System CA certificates
    sds ca_cert_store;              //!< System CA certificate store file
    sds custom_css;                 //!< User defined CSS
    sds custom_js;                  //!< User defined JavaScript
    sds http_host;                  //!< ip to bind the webserver
    sds mympd_uri;                  //!< uri to resolve mympd:// uris
    sds pin_hash;                   //!< hash of the pin
    sds scriptacl;                  //!< IPv4 ACL string for the /api/script endpoint
    sds ssl_cert;                   //!< filename of the certificate
    sds ssl_key;                    //!< filename of the private key
    sds ssl_san;                    //!< additional names for SAN of the self generated certificate
    struct t_albums_config albums;  //!< album specific config
};

#endif
