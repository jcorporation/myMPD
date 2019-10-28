/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <signal.h>
#include <stdlib.h>

#include "../dist/src/sds/sds.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"

void free_request(t_work_request *request) {
    if (request != NULL) {
        sdsfree(request->data);
        sdsfree(request->method);
        free(request);
    }
}
