/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "libmygpio/src/connection.h"
#include "libmygpio/src/pair.h"
#include "libmygpio/src/socket.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// private definitions

static struct t_mygpio_pair *libmygpio_parse_pair(const char *line);

// public functions

/**
 * Receives and parses the key/value pair from the response.
 * @param connection connection struct
 * @return the pair or NULL on error or response end
 */
struct t_mygpio_pair *mygpio_recv_pair(struct t_mygpio_connection *connection) {
    if (mygpio_connection_check(connection) == false) {
        return NULL;
    }
    libmygpio_socket_recv_line(connection->fd, &connection->buf_in, 0);
    if (connection->buf_in.len == 0) {
        return NULL;
    }
    if (strcmp(connection->buf_in.buffer, "END") == 0) {
        return NULL;
    }
    return libmygpio_parse_pair(connection->buf_in.buffer);
}

/**
 * Receives and parses the key/value pair from the response.
 * It checks the name.
 * @param connection connection struct
 * @param name desired name of the pair
 * @return the pair or NULL on error or response end
 */
struct t_mygpio_pair *mygpio_recv_pair_name(struct t_mygpio_connection *connection, const char *name) {
    struct t_mygpio_pair *pair = mygpio_recv_pair(connection);
    if (pair == NULL) {
        return NULL;
    }
    if (strcmp(pair->name, name) != 0) {
        mygpio_free_pair(pair);
        return NULL;
    }
    return pair;
}

/**
 * Frees the name/value pair
 * @param pair pair to free
 */
void mygpio_free_pair(struct t_mygpio_pair *pair) {
    pair->name = NULL;
    pair->value = NULL;
    free(pair);
}

// private functions

/**
 * Parses a line to a key/value pair.
 * Name and value are only pointers and are not copied.
 * @param line line to parse
 * @return allocated pair or NULL on error
 */
static struct t_mygpio_pair *libmygpio_parse_pair(const char *line) {
    struct t_mygpio_pair *pair = malloc(sizeof(struct t_mygpio_pair));
    if (pair == NULL) {
        return NULL;
    }
    char *p = strchr(line, ':');
    if (p == NULL) {
        mygpio_free_pair(pair);
        return NULL;
    }
    pair->name = line;
    *p = '\0';
    pair->value = p + 1;
    return pair;
}
