// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "ierror.h"
#include "socket.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void
mpd_error_deinit(struct mpd_error_info *error)
{
	assert(error != NULL);

	if (error->code != MPD_ERROR_SUCCESS)
		free(error->message);
}

void
mpd_error_message(struct mpd_error_info *error, const char *message)
{
	assert(error != NULL);
	assert(message != NULL);
	assert(mpd_error_is_defined(error));
	assert(error->message == NULL);

	error->message = strdup(message);
	if (error->message == NULL)
		error->code = MPD_ERROR_OOM;
}

void
mpd_error_message_n(struct mpd_error_info *error,
		    const char *message, size_t length)
{
	assert(error != NULL);
	assert(message != NULL);
	assert(mpd_error_is_defined(error));
	assert(error->message == NULL);

	error->message = malloc(length + 1);
	if (error->message != NULL) {
		memcpy(error->message, message, length);
		error->message[length] = 0;
	} else
		error->code = MPD_ERROR_OOM;
}

void
mpd_error_printf(struct mpd_error_info *error, const char *fmt, ...)
{
	char buffer[1024];
	va_list ap;

	assert(error != NULL);
	assert(fmt != NULL);

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	mpd_error_message(error, buffer);
}

void
mpd_error_system_message(struct mpd_error_info *error, int code)
{
#ifdef _WIN32
	char buffer[1024];
	DWORD nbytes;
#endif

	assert(error != NULL);

	mpd_error_system(error, code);

#ifdef _WIN32
	nbytes = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS |
				FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, code, 0,
				(LPSTR)buffer, sizeof(buffer), NULL);
	mpd_error_message(error, nbytes > 0 ? buffer : "Unknown error");
#else
	mpd_error_message(error, strerror(code));
#endif
}

void
mpd_error_errno(struct mpd_error_info *error)
{
	assert(error != NULL);

	mpd_error_system_message(error, mpd_socket_errno());
}

void
mpd_error_entity(struct mpd_error_info *error)
{
	if (errno == EINVAL) {
		mpd_error_code(error, MPD_ERROR_MALFORMED);
		mpd_error_message(error, "Malformed entity response line");
	} else
		mpd_error_code(error, MPD_ERROR_OOM);
}

bool
mpd_error_copy(struct mpd_error_info *dest, const struct mpd_error_info *src)
{
	assert(dest != NULL);
	assert(src != NULL);

	dest->code = src->code;
	if (src->code == MPD_ERROR_SUCCESS)
		return true;

	if (src->code == MPD_ERROR_SERVER) {
		dest->server = src->server;
		dest->at = src->at;
	} else if (src->code == MPD_ERROR_SYSTEM)
		dest->system = src->system;

	dest->message = src->message != NULL ? strdup(src->message) : NULL;
	return false;
}
