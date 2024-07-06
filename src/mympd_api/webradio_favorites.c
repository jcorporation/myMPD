/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/webradio_favorites.h"

#include "dist/rax/rax.h"
#include "src/lib/utility.h"

#include <dirent.h>
#include <string.h>

/**
 * Saves a webradio favorite
 * @param webradio_favorites Webradio favorites struct
 * @param webradio webradio struct to save
 * @param old_name old webradio name
 * @return true on success, else false
 */
bool mympd_api_webradio_favorite_save(struct t_webradios *webradio_favorites, struct t_webradio_data *webradio, sds old_name) {
    if (webradio->uris.head == NULL) {
        return false;
    }
    if (sdslen(old_name) > 0) {
        struct t_list old_names;
        list_push(&old_names, old_name, 0, NULL, NULL);
        if (mympd_api_webradio_favorite_delete(webradio_favorites, &old_names) == false) {
            return false;
        }
        list_clear(&old_names);
    }

    sds id = sdsdup(webradio->uris.head->key);
    sanitize_filename(id);
    if (raxTryInsert(webradio_favorites->db, (unsigned char *)id, sdslen(id), webradio, NULL) == 1) {
        // write uri index
        raxTryInsert(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), webradio, NULL);
        return true;
    }
    return false;
}

/**
 * Deletes webradio favorite
 * @param webradio_favorites Webradio favorites struct
 * @param names webradio ids to delete
 * @return true on success, else false
 */
bool mympd_api_webradio_favorite_delete(struct t_webradios *webradio_favorites, struct t_list *names) {
    bool rc = true;
    struct t_list_node *current = names->head;
    while (current != NULL) {
        void *data = NULL;
        if (raxRemove(webradio_favorites->db, (unsigned char *)current->key, sdslen(current->key), &data) == 1) {
            struct t_webradio_data *webradio = (struct t_webradio_data *)data;
            // remove uri index
            raxRemove(webradio_favorites->idx_uris, (unsigned char *)webradio->uris.head->key, sdslen(webradio->uris.head->key), NULL);
            webradio_data_free(webradio);
        }
        else {
            rc = false;
        }
        current = current->next;
    }
    return rc;
}
