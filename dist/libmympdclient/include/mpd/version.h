// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief MPD client library
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef MPD_VERSION_H
#define MPD_VERSION_H

#define LIBMPDCLIENT_MAJOR_VERSION 2
#define LIBMPDCLIENT_MINOR_VERSION 23
#define LIBMPDCLIENT_PATCH_VERSION 0

/**
 * Preprocessor macro which allows you to check which version of
 * libmpdclient you are compiling with.  It can be used in
 * preprocessor directives.
 *
 * @return true if this libmpdclient version equals or is newer than
 * the specified version number
 * @since libmpdclient 2.1
 */
#define LIBMPDCLIENT_CHECK_VERSION(major, minor, patch) \
	((major) < LIBMPDCLIENT_MAJOR_VERSION || \
	 ((major) == LIBMPDCLIENT_MAJOR_VERSION && \
	  ((minor) < LIBMPDCLIENT_MINOR_VERSION || \
	   ((minor) == LIBMPDCLIENT_MINOR_VERSION && \
	    (patch) <= LIBMPDCLIENT_PATCH_VERSION))))

#define LIBMYMPDCLIENT_MAJOR_VERSION 1
#define LIBMYMPDCLIENT_MINOR_VERSION 0
#define LIBMYMPDCLIENT_PATCH_VERSION 31

/**
 * Preprocessor macro which allows you to check which version of
 * libmympdclient you are compiling with. It can be used in
 * preprocessor directives.
 *
 * @return true if this libmympdclient version equals or is newer than
 * the specified version number
 */
#define LIBMYMPDCLIENT_CHECK_VERSION(major, minor, patch) \
	((major) < LIBMYMPDCLIENT_MAJOR_VERSION || \
	 ((major) == LIBMYMPDCLIENT_MAJOR_VERSION && \
	  ((minor) < LIBMYMPDCLIENT_MINOR_VERSION || \
	   ((minor) == LIBMYMPDCLIENT_MINOR_VERSION && \
	    (patch) <= LIBMYMPDCLIENT_PATCH_VERSION))))

#endif
