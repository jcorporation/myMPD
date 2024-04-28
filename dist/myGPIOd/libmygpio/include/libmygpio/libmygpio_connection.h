/*
 SPDX-License-Identifier: GPL-3.0-or-later
 libmygpio (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myGPIOd client library
 *
 * Do not include this header directly. Use libmygpio/libmygpio.h instead.
 */

#ifndef LIBMYGPIO_CONNECTION_H
#define LIBMYGPIO_CONNECTION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct t_mygpio_connection
 * @{
 * The opaque myGPIOd connection object. You can not access it directly.
 * Refer to @ref libmygpio_connection for function that operate on this struct.
 * @}
 */
struct t_mygpio_connection;

/**
 * @defgroup libmygpio_connection myGPIOd connection
 *
 * @brief This module provides functions for myGPIOd connection management.
 *
 * @{
 */

/**
 * myGPIOd connections states
 */
enum mygpio_conn_state {
    MYGPIO_STATE_OK,     //!< OK state
    MYGPIO_STATE_ERROR,  //!< Error state, read the error with mygpio_connection_get_error and clear it with mygpio_connection_clear_error
    MYGPIO_STATE_FATAL   //!< Fatal state, read the error with mygpio_connection_get_error. You must reconnect to recover.
};

/**
 * Creates a new connection to the myGPIOd socket and tries to connect.
 * Check the state with mygpio_connection_get_state.
 * It must be freed by the caller with mygpio_connection_free.
 * @param socket_path Server socket to connect to.
 * @param timeout_ms The read timeout in milliseconds
 * @return Returns the t_mygpio_connection struct on NULL in a out of memory condition.
 */
struct t_mygpio_connection *mygpio_connection_new(const char *socket_path, int timeout_ms);

/**
 * Closes the connection and frees the t_mygpio_connection struct
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 */
void mygpio_connection_free(struct t_mygpio_connection *connection);

/**
 * Gets the server version.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return Unsigned array consisting of major, minor and patch version.
 */
const unsigned *mygpio_connection_get_version(struct t_mygpio_connection *connection);

/**
 * Returns the file descriptor of the underlying socket.
 * You can use it to poll the file descriptor in an external event loop.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return File descriptor
 */
int mygpio_connection_get_fd(struct t_mygpio_connection *connection);

/**
 * Gets the current connection state.
 * Use mygpio_connection_get_error to get the error message and mygpio_connection_clear_error to clear it.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return The connection state
 */
enum mygpio_conn_state mygpio_connection_get_state(struct t_mygpio_connection *connection);

/**
 * Gets the current error message.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return Error message or NULL if no error is present
 */
const char *mygpio_connection_get_error(struct t_mygpio_connection *connection);

/**
 * Clears the current error message.
 * MYGPIO_STATE_FATAL messages can not be cleared.
 * @param connection Pointer to the connection struct returned by mygpio_connection_new.
 * @return true on success, else false
 */
bool mygpio_connection_clear_error(struct t_mygpio_connection *connection);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
