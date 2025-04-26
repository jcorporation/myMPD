/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#ifndef MYMPD_WEBSERVER_MG_USER_DATA_H
#define MYMPD_WEBSERVER_MG_USER_DATA_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/config_def.h"
#include "src/lib/list.h"

#include <stdbool.h>

/**
 * Struct for mg_mgr userdata
 */
struct t_mg_user_data {
    struct t_config *config;                 //!< Pointer to myMPD configuration
    sds browse_directory;                    //!< document root
    sds music_directory;                     //!< mpd music directory
    sds *coverimage_names;                   //!< sds array of coverimage names
    int coverimage_names_len;                //!< length of coverimage_names array
    sds *thumbnail_names;                    //!< sds array of coverimage thumbnail names
    int thumbnail_names_len;                 //!< length of thumbnail_names array
    bool publish_playlists;                  //!< true if mpd playlist directory is configured
    bool publish_music;                      //!< true if mpd music directory is accessible
    int connection_count;                    //!< number of http connections
    struct t_list stream_uris;               //!< uri for the mpd stream reverse proxy
    struct t_list session_list;              //!< list of myMPD sessions (pin protection mode)
    sds placeholder_booklet;                 //!< name of custom booklet image
    sds placeholder_mympd;                   //!< name of custom mympd image
    sds placeholder_na;                      //!< name of custom not available image
    sds placeholder_stream;                  //!< name of custom stream image
    sds placeholder_playlist;                //!< name of custom playlist image
    sds placeholder_smartpls;                //!< name of custom smart playlist image
    sds placeholder_folder;                  //!< name of custom folder image
    sds placeholder_transparent;             //!< name of custom transparent image
    bool mympd_api_started;                  //!< true if the mympd_api thread is ready, else false
    sds cert_content;                        //!< the server certificate
    sds key_content;                         //!< the server key
    struct mg_str cert;                      //!< pointer to ssl cert_content
    struct mg_str key;                       //!< pointer to ssl key_content
    struct t_webradios *webradiodb;          //!< Pointer to WebradioDB in mympd_api thread
    struct t_webradios *webradio_favorites;  //!< Pointer to webradio favorits in mympd_api thread
};

void mg_user_data_free(struct t_mg_user_data *mg_user_data);
void mg_user_data_free_void(void *mg_user_data);

#endif
