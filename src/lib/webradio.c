/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/webradio.h"

#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

/**
 * Creates a new webradio data struct
 * @return struct t_webradio_data* 
 */
struct t_webradio_data *webradio_data_new(void) {
    struct t_webradio_data *data = malloc_assert(sizeof(struct t_webradio_data));
    list_init(&data->uris);
    list_init(&data->genres);
    list_init(&data->languages);
    data->name = NULL;
    data->image = NULL;
    data->homepage = NULL;
    data->country = NULL;
    data->state = NULL;
    data->description = NULL;
    return data;
}

/**
 * Frees a webradios data struct
 * @param data struct to free
 */
void webradio_data_free(struct t_webradio_data *data) {
    if (data == NULL) {
        return;
    }
    list_clear(&data->uris);
    list_clear(&data->genres);
    list_clear(&data->languages);
    FREE_SDS(data->name);
    FREE_SDS(data->image);
    FREE_SDS(data->homepage);
    FREE_SDS(data->country);
    FREE_SDS(data->state);
    FREE_SDS(data->description);
    // pointer data itself
    FREE_PTR(data);
}

/**
 * Frees the webradios rax
 * @param webradios rax tree to free
 */
void webradio_free(rax *webradios) {
    raxIterator iter;
    raxStart(&iter, webradios);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        webradio_data_free((struct t_webradio_data *)iter.data);
        iter.data = NULL;
    }
    raxStop(&iter);
    raxFree(webradios);
}

/**
 * Saves the webradios to disk
 * @param config pointer to config
 * @param webradios webradios rax to write
 * @param filename file to write
 * @return true on success, else false
 */
bool webradio_save_to_disk(struct t_config *config, rax *webradios, const char *filename) {
    (void)config;
    (void)webradios;
    (void)filename;
    //TODO: implement
    return true;
}

/**
 * Reads the webradios file from disk
 * @param config pointer to config
 * @param filename file to read
 * @return newly allocated rax with webradios
 */
rax *webradio_read_from_disk(struct t_config *config, const char *filename) {
    (void)config;
    (void)filename;
    //TODO: implement
    return NULL;
}
