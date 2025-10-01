// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_QUOTE_H
#define MPD_QUOTE_H

/**
 * Enclose a string in double quotes, and escape special characters.
 *
 * @param dest the destination buffer
 * @param end the end of the destination buffer (pointer to the first
 * invalid byte)
 * @param value the string to quote
 * @return a pointer to the end of the quoted string
 */
char *
quote(char *dest, char *end, const char *value);

#endif
