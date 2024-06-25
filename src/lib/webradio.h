/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_WEBRADIO_H
#define MYMPD_LIB_WEBRADIO_H

#include "dist/rax/rax.h"
#include "dist/sds/sds.h"
#include "src/lib/config_def.h"
#include "src/lib/list.h"

struct t_webradios {
    rax *db;
    rax *idx_uris;
};

struct t_webradio_data {
    sds name;
    sds image;
    sds homepage;
    sds country;
    sds state;
    sds description;
    struct t_list uris;
    struct t_list genres;
    struct t_list languages;
};

struct t_webradio_data *webradio_data_new(void);
void webradio_data_free(struct t_webradio_data *data);

struct t_webradios *webradios_new(void);
void webradios_free(struct t_webradios *webradios);
bool webradios_save_to_disk(struct t_config *config, struct t_webradios *webradios, const char *filename);
bool webradios_read_from_disk(struct t_config *config, struct t_webradios *webradios, const char *filename);

#endif
