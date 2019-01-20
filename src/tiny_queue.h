/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   This linked list implementation is based on: https://github.com/joshkunz/ashuffle
   
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
#ifndef __TINY_QUEUE_H__
#define __TINY_QUEUE_H__

typedef struct tiny_msg_t {
    void *data;
    struct tiny_msg_t *next;
} tiny_msg_t;

typedef struct tiny_queue_t {
    unsigned length;
    struct tiny_msg_t *head;
    struct tiny_msg_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t wakeup;
} tiny_queue_t;

tiny_queue_t *tiny_queue_create(void);
void tiny_queue_free(tiny_queue_t *queue);
int tiny_queue_push(struct tiny_queue_t *queue, void *data);
void *tiny_queue_shift(struct tiny_queue_t *queue, int timeout);
int tiny_queue_length(struct tiny_queue_t *queue, int timeout);
#endif
