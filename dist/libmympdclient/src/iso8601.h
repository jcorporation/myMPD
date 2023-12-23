// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_ISO8601_H
#define MPD_ISO8601_H

#include <stdbool.h>
#include <time.h>

/**
 * Parses an ISO8601 time stamp to a #time_t POSIX UTC time stamp.
 *
 * @param input the ISO8601 time stamp in the form
 * "YYYY-MM-DDTHH:MM:SS"; it is silently assumed that the time zone is
 * UTC ("Z")
 * @return the POSIX UTC time stamp, or 0 on error
 */
time_t
iso8601_datetime_parse(const char *input);

/**
 * Formats a POSIX UTC time stamp into an ISO8601 string.
 *
 * @param buffer the destination string buffer
 * @param size the size of the buffer, including the null terminator
 * @return true on success, false on failure
 */
bool
iso8601_datetime_format(char *buffer, size_t size, time_t t);

#endif
