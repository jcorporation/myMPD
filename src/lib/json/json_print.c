/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Json print implementation for sds
 */

#include "compile_time.h"
#include "src/lib/json/json_print.h"

#include "src/lib/list.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * This unit provides functions for json printing
 */

/**
 * Appends a comma on demand.
 * Comma is omitted on start of string or end of string is already a comma.
 * @param buffer sds string to append
 * @return pointer to buffer
 */
sds json_comma(sds buffer) {
    size_t len = sdslen(buffer);
    if (len == 0 ||
        buffer[len - 1] == ',')
    {
        return buffer;
    }
    return sdscatlen(buffer, ",", 1);
}

/**
 * Prints a json key/value pair for already encoded values
 * value is printed raw without any encoding done
 * @param buffer sds string to append
 * @param key json key
 * @param value raw data
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_raw(sds buffer, const char *key, const char *value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for 0-terminated char values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value to encode as json
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_char(sds buffer, const char *key, const char *value, bool comma) {
    //treat NULL values as empty
    size_t len = value != NULL ? strlen(value) : 0;
    return tojson_char_len(buffer, key, value, len, comma);
}

/**
 * Prints a json key/value pair for sds values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value as sds string to encode as json
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_sds(sds buffer, const char *key, sds value, bool comma) {
    return tojson_char_len(buffer, key, value, sdslen(value), comma);
}

/**
 * Prints a json key/value pair for not 0-terminated values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value as sds string to encode as json
 * @param len length of value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    if (value != NULL) {
        buffer = sds_catjson(buffer, value, len);
    }
    else {
        buffer = sdscatlen(buffer, "\"\"", 2);
    }
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for a bool value
 * @param buffer sds string to append
 * @param key json key
 * @param value bool value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_bool(sds buffer, const char *key, bool value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value == true ? "true" : "false");
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an int value
 * @param buffer sds string to append
 * @param key json key
 * @param value integer value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_int(sds buffer, const char *key, int value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%i", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an unsigned
 * @param buffer sds string to append
 * @param key json key
 * @param value unsigned integer value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_uint(sds buffer, const char *key, unsigned value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%u", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for a time_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value time_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_time(sds buffer, const char *key, time_t value, bool comma) {
    return tojson_int64(buffer, key, (int64_t)value, comma);
}

/**
 * Prints a json key/value pair for a float value
 * @param buffer sds string to append
 * @param key json key
 * @param value double value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_float(sds buffer, const char *key, float value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%.2f", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an int64_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value int64_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_int64(sds buffer, const char *key, int64_t value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%I", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an uint64_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value uint64_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_uint64(sds buffer, const char *key, uint64_t value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%U", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints the keys of a list as a json array
 * Leading and ending square brackets are added
 * @param s sds string to append
 * @param l pointer to list to add keys from
 * @return pointer to s
 */
sds list_to_json_array(sds s, struct t_list *l) {
    s = sdscatlen(s, "[", 1);
    struct t_list_node *current = l->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            s = sdscatlen(s, ",", 1);
        }
        s = sds_catjson(s, current->key, sdslen(current->key));
        current = current->next;
    }
    s = sdscatlen(s, "]", 1);
    return s;
}
