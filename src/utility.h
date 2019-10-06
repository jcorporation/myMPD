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

#ifndef __UTILITY_H__
#define __UTILITY_H__

sds jsonrpc_start_notify(sds buffer, const char *method);
sds jsonrpc_end_notify(sds buffer);
sds jsonrpc_start_result(sds buffer, const char *method, int id);
sds jsonrpc_end_result(sds buffer);
sds jsonrpc_respond_ok(sds buffer, const char *method, int id);
sds jsonrpc_respond_message(sds buffer, const char *method, int id, const char *message, bool error);
sds jsonrpc_respond_message_notify(sds buffer, const char *message, bool error);
sds jsonrpc_start_phrase(sds buffer, const char *method, int id, const char *message, bool error);
sds jsonrpc_start_phrase_notify(sds buffer, const char *message, bool error);
sds jsonrpc_end_phrase(sds buffer);
sds tojson_char(sds buffer, const char *key, const char *value, bool comma);
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma);
sds tojson_bool(sds buffer, const char *key, bool value, bool comma);
sds tojson_long(sds buffer, const char *key, long value, bool comma);
sds tojson_float(sds buffer, const char *key, float value, bool comma);
int testdir(const char *name, const char *dirname, bool create);
int randrange(int n);
bool validate_string(const char *data);
int replacechar(char *str, const char orig, const char rep);

#define FREE_PTR(PTR) do { \
    if (PTR != NULL) \
        free(PTR); \
    PTR = NULL; \
} while (0)

#endif
