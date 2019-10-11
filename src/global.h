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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//signal handler
sig_atomic_t s_signal_received;

//message queue
tiny_queue_t *web_server_queue;
tiny_queue_t *mpd_client_queue;
tiny_queue_t *mympd_api_queue;

typedef struct t_work_request {
    int conn_id; // needed to identify the connection where to send the reply
    int id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
} t_work_request;

typedef struct t_work_result {
    int conn_id; // needed to identify the connection where to send the reply
    sds data;
} t_work_result;

#endif
