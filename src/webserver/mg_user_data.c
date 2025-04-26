/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#include "compile_time.h"
#include "src/webserver/mg_user_data.h"

#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

/**
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 */
void mg_user_data_free(struct t_mg_user_data *mg_user_data) {
    FREE_SDS(mg_user_data->browse_directory);
    FREE_SDS(mg_user_data->music_directory);
    sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
    sdsfreesplitres(mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
    list_clear(&mg_user_data->stream_uris);
    list_clear(&mg_user_data->session_list);
    FREE_SDS(mg_user_data->placeholder_booklet);
    FREE_SDS(mg_user_data->placeholder_mympd);
    FREE_SDS(mg_user_data->placeholder_na);
    FREE_SDS(mg_user_data->placeholder_stream);
    FREE_SDS(mg_user_data->placeholder_playlist);
    FREE_SDS(mg_user_data->placeholder_smartpls);
    FREE_SDS(mg_user_data->placeholder_folder);
    FREE_SDS(mg_user_data->placeholder_transparent);
    FREE_SDS(mg_user_data->cert_content);
    FREE_SDS(mg_user_data->key_content);
    FREE_PTR(mg_user_data);
}

/**
 * Frees the members of mg_user_data struct and the struct itself
 * @param mg_user_data pointer to mg_user_data struct
 */
void mg_user_data_free_void(void *mg_user_data) {
    mg_user_data_free((struct t_mg_user_data *)mg_user_data);
}
