/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "libmygpio/src/connection.h"

#include "libmygpio/include/libmygpio/libmygpio_protocol.h"
#include "libmygpio/src/protocol.h"
#include "libmygpio/src/socket.h"
#include "libmygpio/src/util.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Creates a new connection and checks the initial server response
 * @param socket_path unix socket path to connect
 * @param timeout connection timeout in ms
 * @return allocated connection struct or NULL if malloc fails
 */
struct t_mygpio_connection *mygpio_connection_new(const char *socket_path, int timeout_ms) {
    struct t_mygpio_connection *connection = malloc(sizeof(struct t_mygpio_connection));
    if (connection == NULL) {
        return NULL;
    }
    libmygpio_buf_init(&connection->buf_in);
    libmygpio_buf_init(&connection->buf_out);
    connection->version[0] = 0;
    connection->version[1] = 0;
    connection->version[2] = 0;
    connection->error = NULL;
    connection->timeout_ms = timeout_ms;
    connection->state = MYGPIO_STATE_OK;
    connection->fd = libmygpio_socket_connect(socket_path);
    if (connection->fd == -1) {
        libmygpio_connection_set_state(connection, MYGPIO_STATE_FATAL, "Connection failed");
        return connection;
    }
    if (libmygpio_recv_response_status(connection) == false ||
        libmygpio_recv_version(connection) == false ||
        mygpio_response_end(connection) == false)
    {
        libmygpio_connection_set_state(connection, MYGPIO_STATE_FATAL, "Handshake failed");
        return connection;
    }
    return connection;
}

/**
 * Closes and frees the connection
 * @param connection connection struct
 */
void mygpio_connection_free(struct t_mygpio_connection *connection) {
    if (connection != NULL) {
        libmygpio_socket_close(connection->fd);
        free(connection);
    }
}

/**
 * Checks the connection state
 * @param connection connection struct
 * @return true on success, else false
 */
bool mygpio_connection_check(struct t_mygpio_connection *connection) {
    return connection->state == MYGPIO_STATE_OK &&
        connection->fd > -1;
}

/**
 * Sets connection state and error message
 * @param connection connection struct
 * @param state state enum
 * @param message error message
 */
void libmygpio_connection_set_state(struct t_mygpio_connection *connection,
        enum mygpio_conn_state state, const char *message)
{
    connection->state = state;
    if (connection->error != NULL) {
        free(connection->error);
    }
    if (message != NULL) {
        LIBMYGPIO_LOG("Server error: %s", message);
        connection->error = strdup(message);
    }
    else {
        connection->error = NULL;
    }
}

/**
 * Gets the connection state
 * @param connection connection struct
 * @return the connection state
 */
enum mygpio_conn_state mygpio_connection_get_state(struct t_mygpio_connection *connection) {
    return connection->state;
}

/**
 * Gets the connection error message
 * @param connection connection struct
 * @return the current error message
 */
const char *mygpio_connection_get_error(struct t_mygpio_connection *connection) {
    return connection->error;
}

/**
 * Clears the error state.
 * @param connection connection struct
 * @return true on success, for fatal errors false
 */
bool mygpio_connection_clear_error(struct t_mygpio_connection *connection) {
    if (connection->state == MYGPIO_STATE_FATAL) {
        return false;
    }
    libmygpio_connection_set_state(connection, MYGPIO_STATE_OK, NULL);
    return true;
}

/**
 * Returns the connection version string
 * @param connection connection struct
 * @return the myGPIOd version
 */
const unsigned *mygpio_connection_get_version(struct t_mygpio_connection *connection) {
    return connection->version;
}

/**
 * Returns the connection file descriptor
 * @param connection connection struct
 * @return file descriptor or -1 if not connected
 */
int mygpio_connection_get_fd(struct t_mygpio_connection *connection) {
    return connection->fd;
}
