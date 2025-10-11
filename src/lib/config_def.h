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

#include "dist/sds/sds.h"
#include "src/lib/album.h"

#include <stdbool.h>

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
    bool webradiodb;                //!< enable WebradioDB support
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
