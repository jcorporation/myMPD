// This code is copied from mongoose 6.x

// Copyright (c) 2004-2013 Sergey Lyubka
// Copyright (c) 2013-2021 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <https://www.cesanta.com/license>.

#include "mongoose_compat.h"

//private definitions
static int parse_net(const char *spec, uint32_t *net, uint32_t *mask);
static int isbyte(int n);

//public functions
struct mg_str mg_str_strip_parent(struct mg_str *path, int count) {
    //removes parent dir
    int i = 0;
    count++;
    while (path->len > 0) {
        if (path->ptr[0] == '/') {
            i++;
            if (i == count) {
                break;
            }
        }
        path->len--;
        path->ptr++;
    }
    return *path;
}

int mg6_check_ip_acl(const char *acl, uint32_t remote_ip) {
    int allowed, flag;
    uint32_t net, mask;
    struct mg_str vec;
    struct mg_str acl_str = mg_str(acl);

    // If any ACL is set, deny by default
    allowed = (acl == NULL || *acl == '\0') ? '+' : '-';

    while (mg_next_comma_entry(&acl_str, &vec, NULL)) {
        flag = vec.ptr[0];
        if ((flag != '+' && flag != '-') || parse_net(&vec.ptr[1], &net, &mask) == 0) {
            return -1;
        }

        if (net == (remote_ip & mask)) {
            allowed = flag;
        }
    }
    return allowed == '+';
}

//private functions
static int parse_net(const char *spec, uint32_t *net, uint32_t *mask) {
  int n, a, b, c, d, slash = 32, len = 0;

  if ((sscanf(spec, "%d.%d.%d.%d/%d%n", &a, &b, &c, &d, &slash, &n) == 5 ||
       sscanf(spec, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4) &&
      isbyte(a) && isbyte(b) && isbyte(c) && isbyte(d) && slash >= 0 &&
      slash < 33) {
    len = n;
    *net =
        ((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) | d;
    *mask = slash ? 0xffffffffU << (32 - slash) : 0;
  }

  return len;
}

static int isbyte(int n) {
  return n >= 0 && n <= 255;
}
