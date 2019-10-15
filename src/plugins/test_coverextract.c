/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
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
