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

#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include "tiny_queue.h"

tiny_queue_t *web_server_queue;

struct work_request_t {
    unsigned long conn_id;  // needed to identify the connection where to send the reply
    char data[1000];
    int length;
} work_request_t;

struct work_result_t {
    unsigned long conn_id;  // needed to identify the connection where to send the reply
    char data[MAX_SIZE];
    int length;
} work_result_t;

void *web_server_loop(void *arg);
bool web_server_init(void *arg);
void web_server_free(void *arg);

#endif
