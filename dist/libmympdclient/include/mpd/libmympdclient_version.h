/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/libmpdclient
*/

/*! \file
 * patched libmpdclient library for usage in myMPD
 *
 * Do not include this header directly.  Use mpd/client.h instead.
 */

#ifndef LIBMYMPDCLIENT_VERSION_H
#define LIBMYMPDCLIENT_VERSION_H

#define LIBMYMPDCLIENT_MAJOR_VERSION 1
#define LIBMYMPDCLIENT_MINOR_VERSION 0
#define LIBMYMPDCLIENT_PATCH_VERSION 16

/**
 * Preprocessor macro which allows you to check which version of
 * libmpdclient you are compiling with.  It can be used in
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
