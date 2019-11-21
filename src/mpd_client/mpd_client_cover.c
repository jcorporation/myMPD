/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef EMBEDDED_LIBMPDCLIENT
    #include <mpd/client.h>
#else
    #include <mpd/client.h>
#endif

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../plugins.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

