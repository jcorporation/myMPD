// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_BUFFER_H
#define MPD_BUFFER_H

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>

/**
 * A fixed 4kB buffer which can be appended at the end, and consumed
 * at the beginning.
 */
struct mpd_buffer {
	/** the next buffer position to write to */
	unsigned write;

	/** the next buffer position to read from */
	unsigned read;

	/** the actual buffer */
	unsigned char data[BUFFER_SIZE];
};

/**
 * Initialize an empty buffer.
 */
static inline void
mpd_buffer_init(struct mpd_buffer *buffer)
{
	buffer->read = 0;
	buffer->write = 0;
}

/**
 * Move the start of the valid data to the beginning of the allocated
 * buffer.
 */
static inline void
mpd_buffer_move(struct mpd_buffer *buffer)
{
	memmove(buffer->data, buffer->data + buffer->read,
		buffer->write - buffer->read);

	buffer->write -= buffer->read;
	buffer->read = 0;
}

/**
 * Determines how many bytes can be written to the buffer returned by
 * mpd_buffer_write().
 */
static inline size_t
mpd_buffer_room(const struct mpd_buffer *buffer)
{
	assert(buffer->write <= sizeof(buffer->data));
	assert(buffer->read <= buffer->write);

	return sizeof(buffer->data) - (buffer->write - buffer->read);
}

/**
 * Checks if the buffer is full, i.e. nothing can be written.
 */
static inline bool
mpd_buffer_full(const struct mpd_buffer *buffer)
{
	return mpd_buffer_room(buffer) == 0;
}

/**
 * Returns a pointer to write new data into.  After you have done
 * that, call mpd_buffer_expand().
 */
static inline void *
mpd_buffer_write(struct mpd_buffer *buffer)
{
	assert(mpd_buffer_room(buffer) > 0);

	mpd_buffer_move(buffer);
	return buffer->data + buffer->write;
}

/**
 * Moves the "write" pointer.
 */
static inline void
mpd_buffer_expand(struct mpd_buffer *buffer, size_t nbytes)
{
	assert(mpd_buffer_room(buffer) >= nbytes);

	buffer->write += (unsigned)nbytes;
}

/**
 * Determines how many bytes can be read from the pointer returned by
 * mpd_buffer_read().
 */
static inline size_t
mpd_buffer_size(const struct mpd_buffer *buffer)
{
	assert(buffer->write <= sizeof(buffer->data));
	assert(buffer->read <= buffer->write);

	return buffer->write - buffer->read;
}

/**
 * Returns a pointer to the head of the filled buffer.  It is legal to
 * modify the returned buffer, for zero-copy parsing.
 */
static inline void *
mpd_buffer_read(struct mpd_buffer *buffer)
{
	assert(mpd_buffer_size(buffer) > 0);

	return buffer->data + buffer->read;
}

/**
 * Marks bytes at the beginning of the buffer as "consumed".
 */
static inline void
mpd_buffer_consume(struct mpd_buffer *buffer, size_t nbytes)
{
	assert(nbytes <= mpd_buffer_size(buffer));

	buffer->read += (unsigned)nbytes;
}

#endif
