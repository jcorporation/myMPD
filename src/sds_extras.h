/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __SDS_EXTRAS_H__
#define __SDS_EXTRAS_H__
sds sdscatjson(sds s, const char *p, size_t len);
sds sdsurldecode(sds s, const char *p, size_t len, int is_form_url_encoded);
sds sdscrop(sds s);
sds sdsreplacelen(sds s, const char *value, size_t len);
sds sdsreplace(sds s, const char *value);
#endif
