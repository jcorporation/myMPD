// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include "quote.h"

#include <stddef.h>

/**
 * Append a string to the buffer, and escape special characters.
 */
static char *
escape(char *dest, char *end, const char *value)
{
	while (*value != 0) {
		char ch = *value++;

		if (dest >= end)
			return NULL;

		if (ch == '"' || ch == '\\') {
			*dest++ = '\\';

			if (dest >= end)
				return NULL;
		}

		*dest++ = ch;
	}

	return dest;
}

char *
quote(char *dest, char *end, const char *value)
{
	if (dest >= end)
		return NULL;

	*dest++ = '"';

	dest = escape(dest, end, value);

	if (dest == NULL || dest >= end)
		return NULL;

	*dest++ = '"';

	return dest;
}
