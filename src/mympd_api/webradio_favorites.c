/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD webradio favorites API
 */

#include "compile_time.h"
#include "src/mympd_api/webradio_favorites.h"

#include "dist/rax/rax.h"
#include "src/lib/log.h"

#include <dirent.h>
#include <string.h>
#include <time.h>

/**
 * Saves a webradio favorite. Removes the old_name and overwrites favorites with same name.
 * @param webradio_favorites Webradio favorites struct
 * @param webradio webradio struct to save
 * @param old_name old webradio name
 * @return true on success, else false
 */
bool mympd_api_webradio_favorite_save(struct t_webradios *webradio_favorites, struct t_webradio_data *webradio, sds old_name) {
    if (webradio->uris.head == NULL) {
        return false;
    }

    struct t_list old_names;
    list_init(&old_names);
    if (sdslen(old_name) > 0) {
        list_push(&old_names, old_name, 0, NULL, NULL);
        void *data;
        if (raxFind(webradio_favorites->idx_uris, (unsigned char *)old_name, strlen(old_name), &data) == 1) {
            // preserve added timestamp
            struct t_webradio_data *old_radio = (struct t_webradio_data *)data;
            webradio->added = old_radio->added;
        }
    }

    list_push(&old_names, webradio->name, 0, NULL, NULL);
    mympd_api_webradio_favorite_delete(webradio_favorites, &old_names);
    list_clear(&old_names);

    webradio->last_modified = time(NULL);
    if (webradio->added == -1) {
        webradio->added = webradio->last_modified;
    }

    if (raxTryInsert(webradio_favorites->db, (unsigned char *)webradio->name, sdslen(webradio->name), webradio, NULL) == 1) {
        // write uri index
        raxTryInsert(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), webradio, NULL);
        return true;
    }
    MYMPD_LOG_ERROR("NULL", "Failure saving webradio favorite");
    webradio_data_free(webradio);
    return false;
}

/**
 * Deletes webradio favorites
 * @param webradio_favorites Webradio favorites struct
 * @param names webradio ids to delete
 * @returns Number of removed favorites
 */
int mympd_api_webradio_favorite_delete(struct t_webradios *webradio_favorites, struct t_list *names) {
    struct t_list_node *current = names->head;
    int deleted = 0;
    while (current != NULL) {
        void *data = NULL;
        if (raxRemove(webradio_favorites->db, (unsigned char *)current->key, sdslen(current->key), &data) == 1) {
            deleted++;
            struct t_webradio_data *webradio = (struct t_webradio_data *)data;
            // remove uri index
            raxRemove(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), NULL);
            webradio_data_free(webradio);
        }
        current = current->next;
    }
    return deleted;
}
