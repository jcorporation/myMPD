/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_playlists.h"
#include "mpd_worker_utility.h"
#include "mympd_migrate.h"

//private functions
static bool migrate_smartpls_files(const char *workdir);
static bool migrate_smartpls_file(workdir, const char *playlist);

//public functions
bool start_migrate(const char *workdir) {
//    if () {
        MYMPD_LOG_INFO("Migrating configuration");
        migrate_smartpls_files(workdir);
        //TODO: migrate mympd.conf
//    }
}

//private functions
static bool migrate_smartpls_files(const char *workdir) {
    sds dirname = sdscatfmt(sdsempty(), "%s/smartpls", workdir);
    errno = 0;
    DIR *dir = opendir (dirname);
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0) {
                continue;
            }
            migrate_smartpls_file(workdir, ent->d_name);
        }
        closedir (dir);
    }
    else {
        MYMPD_LOG_ERROR("Can't open smart playlist directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(dirname);
        return false;
    }
    sdsfree(dirname);
    return true;
}

static bool migrate_smartpls_file(workdir, const char *playlist) {
    char *smartpltype = NULL;
    int je;
    bool rc = true;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    int int_buf1;
    int int_buf2;
    
    sds filename = sdscatfmt(sdsempty(), "%s/smartpls/%s", workdir, playlist);
    char *content = json_fread(filename);
    if (content == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        sdsfree(filename);
        return false;
    }
    je = json_scanf(content, (int)strlen(content), "{type: %Q }", &smartpltype);
    if (je != 1) {
        MYMPD_LOG_ERROR("Cant read smart playlist type from \"%s\"", filename);
        sdsfree(filename);
        return false;
    }
    if (strcmp(smartpltype, "search") == 0) {
        je = json_scanf(content, (int)strlen(content), "{expression: %Q}", &p_charbuf1);
        if (je == 1) {
            return;
        }
        else {
            MYMPD_LOG_INFO("Converting smart playlist file \"%s\" to new format", filename);
            je = json_scanf(content, (int)strlen(content), "{tag: %Q, searchstr: %Q, sort: %Q}", &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 3) {
                rc = true;
                sds expression = sdsempty();
                if (strcmp(p_charbuf1, "expression") == 0) {
                    expression = sdscat(expression, p_charbuf2);
                    rc = mpd_shared_smartpls_save(workdir, "search", playlist, expression, 0, 0, p_charbuf3);
                }
                else {
                    expression = sdscat(expression, "(");
                    expression = escape_mpd_search_expression(expression, p_charbuf1, "==", p_charbuf2);
                    expression = sdscat(expression, ")");
                    rc = mpd_shared_smartpls_save(workdir, "search", playlist, expression, 0, 0, p_charbuf3);
                }
                if (rc == true) {
                    rc = mpd_worker_smartpls_update_search(mpd_worker_state, playlist, expression);
                }
                sdsfree(expression);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Update of smart playlist \"%s\" failed", playlist);
                }
                FREE_PTR(p_charbuf3);
            }
            else {
                rc = false;
            }
        }
        FREE_PTR(p_charbuf1);
        FREE_PTR(p_charbuf2);
    }
    FREE_PTR(smartpltype);
    FREE_PTR(content);
    sdsfree(filename);
    return rc;
}
