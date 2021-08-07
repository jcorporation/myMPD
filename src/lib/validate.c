/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "validate.h"

#include <string.h>

bool validate_string(const char *data) {
    if (strchr(data, '/') != NULL || strchr(data, '\n') != NULL || strchr(data, '\r') != NULL ||
        strchr(data, '\t') != NULL ||
        strchr(data, '"') != NULL || strchr(data, '\'') != NULL || strchr(data, '\\') != NULL) {
        return false;
    }
    return true;
}

bool validate_string_not_empty(const char *data) {
    if (data == NULL) { 
        return false;
    }
    if (strlen(data) == 0) {
        return false;
    }
    return validate_string(data);
}

bool validate_string_not_dir(const char *data) {
    bool rc = validate_string_not_empty(data);
    if (rc == true) {
        if (strcmp(data, ".") == 0 || strcmp(data, "..") == 0) {
            rc = false;
        }    
    }
    return rc;
}

bool validate_uri(const char *data) {
    if (strncmp(data, "../", 3) == 0 || strstr(data, "/../") != NULL || strstr(data, "/./") != NULL) {
        return false;
    }
    return true;
}

bool validate_songuri(const char *data) {
    if (data == NULL) {
        return false;
    }
    if (strlen(data) == 0) {
        return false;
    }
    if (strcmp(data, "/") == 0) {
        return false;
    }
    if (strchr(data, '.') == NULL) {
        return false;
    }
    return true;
}

bool is_streamuri(const char *uri) {
    if (uri != NULL && strstr(uri, "://") != NULL) {
        return true;
    }
    return false;
}
