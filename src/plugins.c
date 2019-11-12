/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "plugins.h"

bool init_plugins(struct t_config *config) {
    char *error = NULL;
    handle_plugins_coverextract = NULL;
    if (config->plugins_coverextract == true) {
        sds coverextractplugin = sdscatfmt(sdsempty(), "%s/libmympd_coverextract.so", PLUGIN_PATH);
        LOG_INFO("Loading plugin %s", coverextractplugin);
        handle_plugins_coverextract = dlopen(coverextractplugin, RTLD_NOW);
        if (!handle_plugins_coverextract) {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, dlerror());
            sdsfree(coverextractplugin);
            return false;
        }
        *(void **) (&plugin_coverextract) = dlsym(handle_plugins_coverextract, "coverextract");
        if ((error = dlerror()) != NULL)  {
            LOG_ERROR("Can't load plugin %s: %s", coverextractplugin, error);
            sdsfree(coverextractplugin);
            return false;
        }
        sdsfree(coverextractplugin);
    }
    return true;
}

void close_plugins(struct t_config *config) {
    if (config->plugins_coverextract == true && handle_plugins_coverextract != NULL) {
        dlclose(handle_plugins_coverextract);
    }
}
