/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
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

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

#include "../dist/src/sds/sds.h"
#include "log.h"
#include "utility.h"

sds jsonrpc_start_notify(sds buffer, const char *method) {
    buffer = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"method\":");
    buffer = sdscatjson(buffer, method, strlen(method)); /* Flawfinder: ignore */
    buffer = sdscat(buffer, ",\"params\":{");
    return buffer;
}

sds jsonrpc_end_notify(sds buffer) {
    buffer = sdscatfmt(buffer, "}}");
    return buffer;
}

sds jsonrpc_start_result(sds buffer, const char *method, int id) {
    buffer = sdscatprintf(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"method\":", id);
    buffer = sdscatjson(buffer, method, strlen(method)); /* Flawfinder: ignore */
    buffer = sdscat(buffer, ",\"data\":");
    return buffer;
}

sds jsonrpc_end_result(sds buffer) {
    buffer = sdscatfmt(buffer, "}}");
    return buffer;
}

sds jsonrpc_respond_ok(sds buffer, const char *method, int id) {
    buffer = sdscatprintf(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":{\"method\":", id);
    buffer = sdscatjson(buffer, method, strlen(method)); /* Flawfinder: ignore */
    buffer = sdscat(buffer, ",\"message\":\"ok\"}}");
    return buffer;
}

sds jsonrpc_respond_message(sds buffer, const char *method, int id, const char *message, bool error) {
    buffer = sdscatprintf(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"%s\":{\"method\":", 
        id, (error == true ? "error" : "result"));
    buffer = sdscatjson(buffer, method, strlen(method)); /* Flawfinder: ignore */
    if (error == true) {
        buffer = sdscat(buffer, ",\"code\": -32000");
    }
    buffer = sdscat(buffer, ",\"message\":");
    buffer = sdscatjson(buffer, message, strlen(message)); /* Flawfinder: ignore */
    buffer = sdscatfmt(buffer, "}}");
    return buffer;
}

sds jsonrpc_respond_message_notify(sds buffer, const char *message, bool error) {
    buffer = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"%s\":{", 
        (error == true ? "error" : "result"));
    if (error == true) {
        buffer = sdscat(buffer, "\"code\": -32000,");
    }
    buffer = sdscat(buffer, "\"message\":");
    buffer = sdscatjson(buffer, message, strlen(message)); /* Flawfinder: ignore */
    buffer = sdscatfmt(buffer, "}}");
    return buffer;
}

sds jsonrpc_start_phrase(sds buffer, const char *method, int id, const char *message, bool error) {
    buffer = sdscatprintf(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"%s\":{\"method\":", 
        id, (error == true ? "error" : "result"));
    buffer = sdscatjson(buffer, method, strlen(method)); /* Flawfinder: ignore */
    if (error == true) {
        buffer = sdscat(buffer, ",\"code\": -32000");
    }
    buffer = sdscat(buffer, ",\"message\":");
    buffer = sdscatjson(buffer, message, strlen(message)); /* Flawfinder: ignore */
    buffer = sdscat(buffer, ",\"data\":{");
    return buffer;
}

sds jsonrpc_end_phrase(sds buffer) {
    buffer = sdscat(buffer, "}}}");
    return buffer;
}

sds jsonrpc_start_phrase_notify(sds buffer, const char *message, bool error) {
    buffer = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"%s\":{", 
        (error == true ? "error" : "result"));
    if (error == true) {
        buffer = sdscat(buffer, "\"code\": -32000,");
    }
    buffer = sdscat(buffer, "\"message\":");
    buffer = sdscatjson(buffer, message, strlen(message)); /* Flawfinder: ignore */
    buffer = sdscat(buffer, ",\"data\":{");
    return buffer;
}

sds tojson_char(sds buffer, const char *key, const char *value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    buffer = sdscatjson(buffer, value, strlen(value)); /* Flawfinder: ignore */
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    buffer = sdscatjson(buffer, value, len);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds sdscatjson(sds s, const char *p, size_t len) {
    s = sdscatlen(s,"\"",1);
    while(len--) {
        switch(*p) {
        case '\\':
        case '"':
            s = sdscatprintf(s,"\\%c",*p);
            break;
        case '\n': s = sdscatlen(s,"\\n",2);     break;
        case '\r': s = sdscatlen(s,"\\r",2);     break;
        case '\t': s = sdscatlen(s,"\\t",2);     break;
        case '\a': s = sdscatlen(s,"\\a",2);     break;
        case '\b': s = sdscatlen(s,"\\b",2);     break;
        // Escape < to prevent script execution
        case '<' : s = sdscatlen(s,"\\u003C",6); break;
        default:
            if (isprint(*p))
                s = sdscatprintf(s,"%c",*p);
            else
                s = sdscatprintf(s,"\\u%04X",(unsigned char)*p);
            break;
        }
        p++;
    }
    return sdscatlen(s,"\"",1);
}

sds tojson_bool(sds buffer, const char *key, bool value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value == true ? "true" : "false");
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_long(sds buffer, const char *key, long value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%ld", key, value);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_float(sds buffer, const char *key, float value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%f", key, value);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

int testdir(const char *name, const char *dirname, bool create) {
    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        LOG_INFO("%s: \"%s\"", name, dirname);
        //directory exists
        return 0;
    }
    else {
        if (create == true) {
            if (mkdir(dirname, 0700) != 0) {
                LOG_ERROR("%s: creating \"%s\" failed", name, dirname);
                //directory not exists and creating it failed
                return 2;
            }
            else {
                LOG_INFO("%s: \"%s\" created", name, dirname);
                //directory successfully created
                return 1;
            }
        }
        else {
            LOG_ERROR("%s: \"%s\" don't exists", name, dirname);
            //directory not exists
            return 3;
        }
    }
}

int randrange(int n) {
    return rand() / (RAND_MAX / (n + 1) + 1);
}

bool validate_string(const char *data) {
    if (strchr(data, '/') != NULL || strchr(data, '\n') != NULL || strchr(data, '\r') != NULL ||
        strchr(data, '"') != NULL || strchr(data, '\'') != NULL || strchr(data, '\\') != NULL) {
        return false;
    }
    return true;
}

int replacechar(char *str, const char orig, const char rep) {
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}


sds sdsurldecode(sds s, const char *p, size_t len, int is_form_url_encoded) {
    size_t i;
    int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

    for (i = 0; i < len; i++) {
        switch(*p) {
        case '%':
            if (i < len - 2 && isxdigit(*(const unsigned char *) (p + 1)) &&
                isxdigit(*(const unsigned char *) (p + 2)))
            {
                a = tolower(*(const unsigned char *) (p + 1));
                b = tolower(*(const unsigned char *) (p + 2));
                s = sdscatprintf(s, "%c", (char) ((HEXTOI(a) << 4) | HEXTOI(b)));
                i += 2;
                p += 2;
            } 
            else {
                s = sdscat(sdsempty(), "");
                return s;
            }
            break;
        case '+':
            if (is_form_url_encoded == 1) {
                s = sdscatlen(s, " ", 1);
                break;
            }
            //fall through
        default:
            s = sdscatlen(s, p, 1);
        }
        p++;
    }
    return s;
}
