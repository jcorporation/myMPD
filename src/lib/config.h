/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Configuration handling
 */

#ifndef MYMPD_CONFIG_H
#define MYMPD_CONFIG_H

#include "src/lib/config_def.h"

#include <stdbool.h>

void mympd_config_defaults_initial(struct t_config *config);
bool mympd_config_read(struct t_config *config);
void mympd_config_free(struct t_config *config);
bool mympd_config_rm(struct t_config *config);
bool mympd_read_ca_certificates(struct t_config *config);
void mympd_config_dump_default(void);

#endif
