/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief EXTM3U import
 */

#include "compile_time.h"
#include "src/lib/m3u.h"

#include "src/lib/convert.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_api/webradio_favorites.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>

// Private definitions
struct t_webradio_data *m3u_to_favorite(sds filename);
static void populate_field(struct t_webradio_data *data, sds fields, sds value);

//Public functions

/**
 * Imports old webradio favorites
 * @param mympd_state Pointer to mympd_state
 * @return true on success, else false
 */
bool webradio_favorite_import(struct t_mympd_state *mympd_state) {
    sds dirname = sdscatfmt(sdsempty(), "%S/webradios", mympd_state->config->workdir);
    errno = 0;
    DIR *webradio_dir = opendir(dirname);
    if (webradio_dir == NULL) {
        if (errno == ENOENT) {
            FREE_SDS(dirname);
            return true;
        }
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(dirname);
        return false;
    }

    struct dirent *next_file;
    sds filename = sdsempty();
    sds old_name = sdsempty();
    while ((next_file = readdir(webradio_dir)) != NULL ) {
        const char *ext = get_extension_from_filename(next_file->d_name);
        if (ext == NULL ||
            strcasecmp(ext, "m3u") != 0)
        {
            continue;
        }
        filename = sdscatfmt(filename, "%S/%s", dirname, next_file->d_name);
        struct t_webradio_data *data = m3u_to_favorite(filename);
        if (data != NULL) {
            MYMPD_LOG_INFO(NULL, "Importing webradio favorite: %s", data->name);
            if (mympd_api_webradio_favorite_save(mympd_state->webradio_favorites, data, old_name) == true) {
                rm_file(filename);
            }
        }
        sdsclear(filename);
    }
    closedir(webradio_dir);
    rm_directory(dirname);
    FREE_SDS(filename);
    FREE_SDS(dirname);
    FREE_SDS(old_name);
    return true;
}

// Private functions

/**
 * Converts a EXTM3U to a webradio favorite json
 * @param filename extm3u file
 * @return Pointer to buffer
 */
struct t_webradio_data *m3u_to_favorite(sds filename) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", filename);
        MYMPD_LOG_ERRNO(NULL, errno);
        return NULL;
    }
    //check ext m3u header
    int nread = 0;
    sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
    if (strcmp(line, "#EXTM3U") != 0) {
        MYMPD_LOG_WARN(NULL, "Invalid ext m3u file");
        FREE_SDS(line);
        (void) fclose(fp);
        return NULL;
    }
    struct t_webradio_data *data = webradio_data_new(WEBRADIO_FAVORITE);
    list_push(&data->uris, "", 0, "", NULL);
    data->name = sdsempty();
    data->image = sdsempty();
    data->homepage = sdsempty();
    data->country = sdsempty();
    data->region = sdsempty();
    data->description = sdsempty();
    sds field = sdsempty();
    sds value = sdsempty();
    nread = 0;
    while ((line = sds_getline(line, fp, LINE_LENGTH_MAX, &nread)) && nread >= 0) {
        if (line[0] == '\0') {
            // skip blank lines
            continue;
        }
        if (line[0] != '#') {
            //stream uri
            data->uris.head->key = sdscat(data->uris.head->key, line);
            continue;
        }
        // skip # char
        int i = 1;
        sdsclear(field);
        sdsclear(value);
        // get field
        while (line[i] != '\0' &&
               line[i] != ':')
        {
            field = sds_catjsonchar(field, line[i]);
            i++;
        }
        // get value
        i++;
        while (line[i] != '\0') {
            value = sds_catchar(value, line[i]);
            i++;
        }
        if (sdslen(field) > 0 && sdslen(value) > 0) {
            populate_field(data, field, value);
        }
    }
    FREE_SDS(line);
    FREE_SDS(field);
    FREE_SDS(value);
    (void) fclose(fp);
    return data;
}

/**
 * Populates a struct t_webradio field
 * @param data Webradio struct
 * @param field M3U field name
 * @param value M3U field value
 */
static void populate_field(struct t_webradio_data *data, sds field, sds value) {
    if (strcmp(field, "PLAYLIST") == 0) {
        data->name = sdscat(data->name, value);
    }
    else if (strcmp(field, "EXTIMG") == 0) {
        data->image = sdscat(data->image, value);
    }
    else if (strcmp(field, "HOMEPAGE") == 0) {
        data->homepage = sdscat(data->homepage, value);
    }
    else if (strcmp(field, "COUNTRY") == 0) {
        data->country = sdscat(data->country, value);
    }
    else if (strcmp(field, "STATE") == 0) {
        data->region = sdscat(data->region, value);
    }
    else if (strcmp(field, "DESCRIPTION") == 0) {
        data->description = sdscat(data->description, value);
    }
    else if (strcmp(field, "LANGUAGE") == 0) {
        list_push(&data->languages, value, 0, NULL, NULL);
    }
    else if (strcmp(field, "GENRE") == 0) {
        list_push(&data->genres, value, 0, NULL, NULL);
    }
    else if (strcmp(field, "CODEC") == 0) {
        data->uris.head->value_p = sdscatsds(data->uris.head->value_p, value);
    }
    else if (strcmp(field, "BITRATE") == 0) {
        str2int64(&data->uris.head->value_i, value);
    }
}
