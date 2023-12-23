// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

/*! \file
 * \brief Compiler specific definitions
 *
 * This file is not part of the official libmpdclient2 API.  It
 * provides access to gcc specific extensions.
 *
 */

#ifndef MPD_COMPILER_H
#define MPD_COMPILER_H

#if !defined(SPARSE) && defined(__GNUC__) && __GNUC__ >= 3

/* GCC 4.x */

#define mpd_unused __attribute__((unused))
#define mpd_malloc __attribute__((malloc))
#define mpd_pure __attribute__((pure))
#define mpd_const __attribute__((const))
#define mpd_sentinel __attribute__((sentinel))

#ifdef __clang__
#define mpd_printf(a,b) __attribute__((format(printf, a, b)))
#else
#define mpd_printf(a,b) __attribute__((format(gnu_printf, a, b)))
#endif

#else

/* generic C compiler */

#define mpd_unused
#define mpd_malloc
#define mpd_pure
#define mpd_const
#define mpd_sentinel
#define mpd_printf(a,b)

#endif

#endif
