// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "sync.h"
#include "iasync.h"
#include "socket.h"

#include <mpd/async.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <sys/select.h>
#endif
#include <fcntl.h>

static enum mpd_async_event
mpd_sync_poll(struct mpd_async *async, struct timeval *tv)
{
	int fd;
	fd_set rfds, wfds, efds;
	int ret;
	enum mpd_async_event events;

	fd = mpd_async_get_fd(async);

	while (1) {
		events = mpd_async_events(async);
		if (events == 0)
			return 0;

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		if (events & MPD_ASYNC_EVENT_READ)
			FD_SET(fd, &rfds);
		if (events & MPD_ASYNC_EVENT_WRITE)
			FD_SET(fd, &wfds);
		if (events & (MPD_ASYNC_EVENT_HUP|MPD_ASYNC_EVENT_ERROR))
			FD_SET(fd, &efds);

		ret = select(fd + 1, &rfds, &wfds, &efds, tv);
		if (ret > 0) {
			if (!FD_ISSET(fd, &rfds))
				events &= ~MPD_ASYNC_EVENT_READ;
			if (!FD_ISSET(fd, &wfds))
				events &= ~MPD_ASYNC_EVENT_WRITE;
			if (!FD_ISSET(fd, &efds))
				events &= ~(MPD_ASYNC_EVENT_HUP|
					    MPD_ASYNC_EVENT_ERROR);

			return events;
		}

		if (ret == 0 || !mpd_socket_ignore_errno(mpd_socket_errno()))
			return 0;
	}
}

static bool
mpd_sync_io(struct mpd_async *async, struct timeval *tv)
{
	enum mpd_async_event events = mpd_sync_poll(async, tv);

	if (events)
		return mpd_async_io(async, events);
	else
		return false;
}

bool
mpd_sync_send_command_v(struct mpd_async *async, const struct timeval *tv0,
			const char *command, va_list args)
{
	struct timeval tv, *tvp;
	va_list copy;
	bool success;

	if (tv0 != NULL) {
		tv = *tv0;
		tvp = &tv;
	} else
		tvp = NULL;

	while (true) {
		va_copy(copy, args);
		success = mpd_async_send_command_v(async, command, copy);
		va_end(copy);

		if (success)
			return true;

		/* no characters were written to async buffer and no
		   space will be made available with mpd_sync_io()
		   hence we do not have enough space */
		if ((mpd_async_events(async) & MPD_ASYNC_EVENT_WRITE) == 0) {
			mpd_async_set_error(async, MPD_ERROR_ARGUMENT,
					    "Not enough buffer space for message");
			return false;
		}


		if (!mpd_sync_io(async, tvp))
			return false;
	}
}

bool
mpd_sync_send_command(struct mpd_async *async, const struct timeval *tv,
		      const char *command, ...)
{
	va_list args;
	bool success;

	va_start(args, command);
	success = mpd_sync_send_command_v(async, tv, command, args);
	va_end(args);

	return success;
}

bool
mpd_sync_flush(struct mpd_async *async, const struct timeval *tv0)
{
	struct timeval tv, *tvp;

	if (tv0 != NULL) {
		tv = *tv0;
		tvp = &tv;
	} else
		tvp = NULL;

	if (!mpd_async_io(async, MPD_ASYNC_EVENT_WRITE))
		return false;

	while (true) {
		enum mpd_async_event events = mpd_async_events(async);
		if ((events & MPD_ASYNC_EVENT_WRITE) == 0)
			/* no more pending writes */
			return true;

		if (!mpd_sync_io(async, tvp))
			return false;
	}
}

char *
mpd_sync_recv_line(struct mpd_async *async, const struct timeval *tv0)
{
	struct timeval tv, *tvp;
	char *line;

	if (tv0 != NULL) {
		tv = *tv0;
		tvp = &tv;
	} else
		tvp = NULL;

	while (true) {
		line = mpd_async_recv_line(async);
		if (line != NULL)
			return line;

		if (!mpd_sync_io(async, tvp))
			return NULL;
	}
}

size_t
mpd_sync_recv_raw(struct mpd_async *async, const struct timeval *tv0,
		  void *dest, size_t length)
{
	struct timeval tv, *tvp;

	if (tv0 != NULL) {
		tv = *tv0;
		tvp = &tv;
	} else
		tvp = NULL;

	while (true) {
		size_t nbytes = mpd_async_recv_raw(async, dest, length);
		if (nbytes > 0)
			return nbytes;

		if (!mpd_sync_io(async, tvp))
			return 0;
	}
}
