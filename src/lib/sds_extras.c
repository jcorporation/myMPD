/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/sds_extras.h"

#include "dist/sds/sds.h"
#include "dist/utf8/utf8.h"
#include "src/lib/convert.h"

#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string.h>

#define HEXTOI(x) ((x) >= '0' && (x) <= '9' ? (x) - '0' : (x) - 'W')

/**
 * Replacement for basename function.
 * Modifies the sds string in place.
 * @param s sds string to apply basename function
 * @return new pointer to s
 */
sds sds_basename(sds s) {
    if (!s || !*s) {
        sdsclear(s);
        return sdscat(s, ".");
    }

    size_t idx = sdslen(s) - 1;
    int end = -1;
    // remove trailing slash
    for (; idx && s[idx] == '/'; idx--) {
        end--;
    }
    // get non-slash component
    for (; idx && s[idx - 1] != '/'; idx--) {
        // count only
    }
    sdsrange(s, (ssize_t)idx, end);
    return s;
}

/**
 * Replacement for dirname function.
 * Modifies the sds string in place.
 * @param s sds string to apply dirname function
 * @return new pointer to s
 */
sds sds_dirname(sds s) {
    if (!s || !*s) {
        sdsclear(s);
        return sdscat(s, ".");
    }
    size_t idx = sdslen(s) - 1;
    // remove trailing slash
    for (; s[idx] == '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, "/");
        }
    }
    // remove last non-slash component
    for (; s[idx] != '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, ".");
        }
    }
    // remove trailing slash
    for (; s[idx] == '/'; idx--) {
        if (!idx) {
            sdsclear(s);
            return sdscat(s, "/");
        }
    }
    sdsrange(s, 0, (ssize_t)idx);
    return s;
}

/**
 * Splits a comma separated string and trims whitespaces from single values
 * @param s string to split
 * @param count pointer to int representing the count of values
 * @return array of sds strings
 */
sds *sds_split_comma_trim(sds s, int *count) {
    *count = 0;
    sds *values = sdssplitlen(s, (ssize_t)sdslen(s), ",", 1, count);
    for (int i = 0; i < *count; i++) {
        sdstrim(values[i], " ");
    }
    return values;
}

/**
 * Hashes a string with sha1
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash_sha1(const char *p) {
    sds hex_hash = sdsnew(p);
    return sds_hash_sha1_sds(hex_hash);
}

/**
 * Hashes a sds string with sha1 inplace
 * @param s string to hash
 * @return pointer to s
 */
sds sds_hash_sha1_sds(sds s) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)s, sdslen(s), hash);
    sdsclear(s);
    for (unsigned i = 0; i < SHA_DIGEST_LENGTH; i++) {
        s = sdscatprintf(s, "%02x", hash[i]);
    }
    return s;
}

/**
 * Hashes a string with sha256
 * @param p string to hash
 * @return the hash as a newly allocated sds string
 */
sds sds_hash_sha256(const char *p) {
    sds hex_hash = sdsnew(p);
    return sds_hash_sha256_sds(hex_hash);
}

/**
 * Hashes a sds string with sha256 inplace
 * @param s string to hash
 * @return pointer to s
 */
sds sds_hash_sha256_sds(sds s) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)s, sdslen(s), hash);
    sdsclear(s);
    for (unsigned i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        s = sdscatprintf(s, "%02x", hash[i]);
    }
    return s;
}

/**
 * Makes the string lower case (utf8 aware)
 * @param s sds string to modify in place
 */
void sds_utf8_tolower(sds s) {
    utf8_int32_t cp;

    void *pn = utf8codepoint(s, &cp);
    while (cp != 0) {
        const size_t size = utf8codepointsize(cp);
        const utf8_int32_t lwr_cp = utf8lwrcodepoint(cp);
        const size_t lwr_size = utf8codepointsize(lwr_cp);

        if (lwr_cp != cp && lwr_size == size) {
            utf8catcodepoint(s, lwr_cp, lwr_size);
        }

        s = pn;
        pn = utf8codepoint(s, &cp);
    }
}

/**
 * JSON escapes special chars
 * @param c char to escape
 * @return escaped char
 */
static const char *escape_char(char c) {
    switch(c) {
        case '\\': return "\\\\";
        case '"':  return "\\\"";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
    }
    return NULL;
}

/**
 * Append to the sds string "s" a json escaped string
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param p string to append json escaped
 * @param len length of the string to append
 * @return modified sds string
 */
sds sds_catjson_plain(sds s, const char *p, size_t len) {
    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least stringlength + 10 chars. */
    s = sdsMakeRoomFor(s, len + 10);
    size_t i = sdslen(s);
    while (len--) {
        switch(*p) {
            case '\\':
            case '"':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t': {
                if (sdsavail(s) == 0) {
                    s = sdsMakeRoomFor(s, 2);
                }
                const char *escape = escape_char(*p);
                s[i++] = escape[0];
                s[i++] = escape[1];
                sdsinclen(s, 2);
                break;
            }
            //ignore vertical tabulator and alert
            case '\v':
            case '\a':
                //this escapes are not accepted in the unescape function
                break;
            default:
                if (sdsavail(s) == 0) {
                    s = sdsMakeRoomFor(s, 1);
                }
                s[i++] = *p;
                sdsinclen(s, 1);
                break;
        }
        p++;
    }
    // Add null-term
    s[i] = '\0';
    return s;
}

/**
 * Append to the sds string "s" a quoted json escaped string
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param p string to append json escaped
 * @param len length of the string to append
 * @return modified sds string
 */
sds sds_catjson(sds s, const char *p, size_t len) {
    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least stringlength + 10 chars. */
    s = sdsMakeRoomFor(s, len + 10);
    s = sdscatlen(s, "\"", 1);
    s = sds_catjson_plain(s, p, len);
    return sdscatlen(s, "\"", 1);
}

/**
 * Append to the sds string "s" an json escaped character
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 * @param s sds string
 * @param c char to escape and append
 * @return modified sds string
 */
sds sds_catjsonchar(sds s, const char c) {
    switch(c) {
        case '\\':
        case '"':
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
            s = sdscatlen(s, escape_char(c), 2);
            break;
        //ignore vertical tabulator and alert
        case '\v':
        case '\a':
            //this escapes are not accepted in the unescape function
            break;
        default:
            s = sds_catchar(s, c);
            break;
    }
    return s;
}

/**
 * Appends a char to sds string s, this is faster than using sdscatfmt
 * @param s sds string
 * @param c char to append
 * @return modified sds string
 */
sds sds_catchar(sds s, const char c) {
    // Make sure there is always space for at least 1 char.
    if (sdsavail(s) == 0) {
        s = sdsMakeRoomFor(s, 1);
    }
    size_t i = sdslen(s);
    s[i++] = c;
    sdsinclen(s, 1);
    // Add null-term
    s[i] = '\0';
    return s;
}

/**
 * Json unescapes "src" and appends the result to the sds string "dst"
 * "dst" must be a pointer to a allocated sds string.
 * @param src string to unescape
 * @param slen string length to unescape
 * @param dst pointer to sds string to append the unescaped string
 * @return true on success, false on error
 */
bool sds_json_unescape(const char *src, size_t slen, sds *dst) {
    char *send = (char *) src + slen;
    char *p;
    const char *esc1 = "\"\\/bfnrt";
    const char *esc2 = "\"\\/\b\f\n\r\t";

    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least src length chars. */
    *dst = sdsMakeRoomFor(*dst, slen);

    while (src < send) {
        if (*src == '\\') {
            if (++src >= send) {
                //escape char should not be the last
                return false;
            }
            if (*src == 'u') {
                //skip unicode escapes
                if (send - src < 5) {
                    return false;
                }
                src += 4;
            }
            else if ((p = strchr(esc1, *src)) != NULL) {
                //print unescaped value
                *dst = sds_catchar(*dst, esc2[p - esc1]);
            }
            else {
                //other escapes are not accepted
                return false;
            }
        }
        else {
            *dst = sds_catchar(*dst, *src);
        }
        src++;
    }

    return true;
}

/**
 * Checks for url safe characters
 * @param c char to check
 * @return true if char is url safe else false
 */
static bool is_url_safe(char c) {
    if (isalnum(c) ||
        c == '/' || c == '-' || c == '.' ||
        c == '_')
    {
        return true;
    }
    return false;
}

/**
 * Url encodes a string
 * @param s sds string to append the encoded string
 * @param p string to url encode
 * @param len string length to url encode
 * @return modified sds string
 */
sds sds_urlencode(sds s, const char *p, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (is_url_safe(p[i])) {
            s = sds_catchar(s, p[i]);
        }
        else {
            s = sdscatprintf(s, "%%%hhX", p[i]);
        }
    }
    return s;
}

/**
 * Url decodes a string
 * @param s sds string to append the url decoded string
 * @param p string to url decode
 * @param len string length to url decode
 * @param is_form_url_encoded true decodes + chars to spaces
 * @return modified sds string
 */
sds sds_urldecode(sds s, const char *p, size_t len, bool is_form_url_encoded) {
    size_t i;
    int a;
    int b;

    for (i = 0; i < len; i++) {
        switch(*p) {
            case '%':
                if (i < len - 2 && isxdigit(*(const unsigned char *) (p + 1)) &&
                    isxdigit(*(const unsigned char *) (p + 2)))
                {
                    a = tolower(*(const unsigned char *) (p + 1));
                    b = tolower(*(const unsigned char *) (p + 2));
                    s = sds_catchar(s, (char) ((HEXTOI(a) << 4) | HEXTOI(b)));
                    i += 2;
                    p += 2;
                }
                else {
                    //error - return blank string
                    sdsclear(s);
                    return s;
                }
                break;
            case '+':
                if (is_form_url_encoded == true) {
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

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @param len replacement string length
 * @return modified sds string
 */
sds sds_replacelen(sds s, const char *p, size_t len) {
    if (s != NULL) {
        sdsclear(s);
    }
    else {
        s = sdsempty();
    }
    if (p != NULL) {
        s = sdscatlen(s, p, len);
    }
    return s;
}

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @return modified sds string
 */
sds sds_replace(sds s, const char *p) {
    return sds_replacelen(s, p, strlen(p));
}

/**
 * Converts a bool value to a sds string
 * @param s string to append the value
 * @param v bool value to convert
 * @return modified sds string
 */
sds sds_catbool(sds s, bool v) {
    return v == true ? sdscatlen(s, "true", 4) : sdscatlen(s, "false", 5);
}
