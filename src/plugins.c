/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of: ympd (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   ympd project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
