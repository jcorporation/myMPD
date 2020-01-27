/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"

static int json_get_utf8_char_len(unsigned char ch) {
  if ((ch & 0x80) == 0) return 1;
  switch (ch & 0xf0) {
    case 0xf0:
      return 4;
    case 0xe0:
      return 3;
    default:
      return 2;
  }
}

sds sdscatjson(sds s, const char *p, size_t len) {
    const char *hex_digits = "0123456789abcdef";
    size_t cl = 0;
    s = sdscatlen(s, "\"", 1);
    while (len--) {
        switch(*p) {
        case '\\':
        case '"':
            s = sdscatprintf(s, "\\%c", *p);
            break;
        case '\n': s = sdscatlen(s, "\\n", 2);     break;
        case '\r': s = sdscatlen(s, "\\r", 2);     break;
        case '\t': s = sdscatlen(s, "\\t", 2);     break;
        case '\b': s = sdscatlen(s, "\\b", 2);     break;
        case '\f': s = sdscatlen(s, "\\f", 2);     break;
        // Escape < to prevent script execution
        case '<' : s = sdscatlen(s, "\\u003C", 6); break;
        //ignore vertical tabulator and alert
        case '\v': 
        case '\a': 
            break;
        default:
            if (isprint(*p)) {
                s = sdscatprintf(s, "%c", *p);
            }
            else if ((cl = json_get_utf8_char_len(*p)) == 1) {
                s = sdscatprintf(s, "\\u00%s%s", &hex_digits[(*p >> 4) % 0xf], &hex_digits[*p % 0xf]);
            } 
            else {
                s = sdscatprintf(s, "%c", *p);
            }
            break;
        }
        p++;
    }
    return sdscatlen(s, "\"", 1);
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
    sdsclear(s);
    return s;
}
