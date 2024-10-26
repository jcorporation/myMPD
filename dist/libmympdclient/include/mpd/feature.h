// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef LIBMPDCLIENT_FEATURE_H
#define LIBMPDCLIENT_FEATURE_H

/**
 * @since libmpdclient 2.23 added support for #MPD_TAG_SHOWMOVEMENT.
 */
enum mpd_protocol_feature
{
	/**
	 * Special value returned by mpd_feature_parse() when an
	 * unknown name was passed.
	 */
	MPD_FEATURE_UNKNOWN = -1,

	MPD_FEATURE_HIDE_PLAYLISTS_IN_ROOT,

	/* IMPORTANT: the ordering of tag types above must be
	   retained, or else the libmpdclient ABI breaks */

	MPD_FEATURE_COUNT
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Looks up the name of the specified protocol feature.
 *
 * @return the name, or NULL if the tag type is not valid
 */
const char *
mpd_feature_name(enum mpd_protocol_feature feature);

/**
 * Parses a protocol feature name, and returns its #mpd_protocol_feature value.
 *
 * @return a #mpd_protocol_feature value, or MPD_FEATURE_UNKNOWN if the name was
 * not recognized
 */
enum mpd_protocol_feature
mpd_feature_name_parse(const char *name);

#ifdef __cplusplus
}
#endif

#endif
