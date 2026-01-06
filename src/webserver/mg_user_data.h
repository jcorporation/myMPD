/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#ifndef MYMPD_WEBSERVER_MG_USER_DATA_H
#define MYMPD_WEBSERVER_MG_USER_DATA_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/config/config_def.h"
#include "src/lib/list.h"
#include "src/lib/lyrics.h"

#include <stdbool.h>

#define MAX_EMBEDDED_FILES 50  //!< Array size for embedded files

/**
 * Struct holding embedded file information
 */
struct t_embedded_file {
    const char *uri;            //!< URI
    const char *mimetype;       //!< Mime type
    bool compressed;            //!< Data is compressed
    bool cache;                 //!< Add cache header in response
    const unsigned char *data;  //!< The data
    unsigned size;              //!< Size of the data
};

/**
 * Struct for mg_mgr userdata
 */
struct t_mg_user_data {
    struct t_config *config;                 //!< Pointer to myMPD configuration
    sds browse_directory;                    //!< document root
    sds music_directory;                     //!< mpd music directory
    sds *image_names_sm;                     //!< sds array of small image names
    int image_names_sm_len;                  //!< length of small image_names array
    sds *image_names_md;                     //!< sds array of medium image names
    int image_names_md_len;                  //!< length of medium image_names array
    sds *image_names_lg;                     //!< sds array of large image names
    int image_names_lg_len;                  //!< length of large image_names array
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
    struct t_webradios *webradio_favorites;  //!< Pointer to webradio favorites in mympd_api thread
    struct t_embedded_file embedded_files[MAX_EMBEDDED_FILES];  //!< Embedded files
    unsigned embedded_file_index;            //!< Index of last embedded_file
    struct t_lyrics lyrics;                  //!< lyrics settings
};

void mg_user_data_free(struct t_mg_user_data *mg_user_data);
void mg_user_data_free_void(void *mg_user_data);

#endif
