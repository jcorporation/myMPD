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
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"

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
                s = sdscrop(s);
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

sds sdsreplacelen(sds s, const char *value, size_t len) {
    s = sdscrop(s);
    s = sdscatlen(s, value, len);
    return s;
}

sds sdsreplace(sds s, const char *value) {
    s = sdscrop(s);
    if (value != NULL) {
        s = sdscat(s, value);
    }
    return s;
}

sds sdscrop(sds s) {
    if (s == NULL) {
        return sdsempty();
    }
    else if (sdslen(s) > 0) {
        sdsfree(s);
        return sdsempty();
    }
    else {
        return s;
    }
}
