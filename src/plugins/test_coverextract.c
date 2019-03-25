/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char **argv) {
    char image_name[1024];
    char image_mime_type[100];
    char *error;
    void *handle_plugins_coverextract = NULL;
    bool (*plugin_coverextract)(const char *, const char *, char *, const int, char *, const int, const bool);
    
    handle_plugins_coverextract = dlopen("/usr/share/mympd/lib/libmympd_coverextract.so", RTLD_LAZY);
    if (!handle_plugins_coverextract) {
        printf("ERROR loading plugin %s: %s\n", "/usr/share/mympd/lib/libmympd_coverextract.so", dlerror());
        return 1;
    }
    *(void **) (&plugin_coverextract) = dlsym(handle_plugins_coverextract, "coverextract");
    if ((error = dlerror()) != NULL)  {
        printf("ERROR loading plugin %s: %s\n", "/usr/share/mympd/lib/libmympd_coverextract.so", error);
        return 1;
    }
    if (argc == 2) {
        int rc = plugin_coverextract(argv[1], "/var/lib/mympd/covercache", image_name, 1024, image_mime_type, 100, true);
        printf("rc: %s, image_name: %s, image_mime_type: %s\n", rc == true ? "true" : "false", image_name, image_mime_type);
    }
    else {
        printf("Usage: %s mediafile\n", argv[0]);
        return 1;
    }
    dlclose(handle_plugins_coverextract);
    return 0;
}
