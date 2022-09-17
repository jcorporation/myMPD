/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_CONFIG_DEF_H
#define MYMPD_LIB_CONFIG_DEF_H

#include "compile_time.h"

#include "../dist/sds/sds.h"

#include <stdbool.h>

/**
 * Static myMPD configuration read at startup from files / environment
 */
struct t_config {
    sds user;                 //!< username to drop privileges
    sds workdir;              //!< working directory
    sds cachedir;             //!< cache directory
    sds http_host;            //!< ip to bind the webserver
    sds http_port;            //!< http port to listen
    #ifdef ENABLE_SSL
        bool ssl;             //!< enables ssl
        sds ssl_port;         //!< https port to listen
        sds ssl_cert;         //!< filename of the certificate
        sds ssl_key;          //!< filename of the private key
        bool custom_cert;     //!< false if myMPD uses the self generated certificates
        sds ssl_san;          //!< additonal names for SAN of the self generated certificate
    #endif
    sds acl;                  //!< IPv4 ACL string
    sds scriptacl;            //!< IPv4 ACL string for the /api/script endpoint
    #ifdef ENABLE_LUA
        sds lualibs;          //!< enabled lua libraries
    #endif
    bool log_to_syslog;       //!< enable syslog logging
    int loglevel;             //!< loglevel
    time_t startup_time;      //!< unix timestamp of startup (not configurable)
    bool first_startup;       //!< true if it is the first myMPD startup (not configurable)
    bool bootstrap;           //!< true if bootstrap command line option is set
    sds pin_hash;             //!< hash of the pin
    int covercache_keep_days; //!< expiration time for covercache files
};

#endif
