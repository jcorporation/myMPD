// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef LIBMPDCLIENT_TAG_H
#define LIBMPDCLIENT_TAG_H

/**
 * @since libmpdclient 2.10 added support for #MPD_TAG_MUSICBRAINZ_RELEASETRACKID.
 * @since libmpdclient 2.11 added support for #MPD_TAG_ARTIST_SORT,
  *                                           #MPD_TAG_ALBUM_ARTIST_SORT.
 * @since libmpdclient 2.17 added support for #MPD_TAG_LABEL,
 *                                            #MPD_TAG_MUSICBRAINZ_WORKID,
 *                                            #MPD_TAG_GROUPING,
 *                                            #MPD_TAG_WORK,
 *                                            #MPD_TAG_CONDUCTOR.
 * @since libmpdclient 2.20 added support for #MPD_TAG_COMPOSER_SORT,
 *                                            #MPD_TAG_ENSEMBLE,
 *                                            #MPD_TAG_MOVEMENT,
 *                                            #MPD_TAG_MOVEMENTNUMBER,
 *                                            #MPD_TAG_LOCATION.
 * @since libmpdclient 2.21 added support for #MPD_TAG_MOOD,
 *                                            #MPD_TAG_TITLE_SORT.
 * @since libmpdclient 2.23 added support for #MPD_TAG_SHOWMOVEMENT.
 */
enum mpd_tag_type
{
	/**
	 * Special value returned by mpd_tag_name_parse() when an
	 * unknown name was passed.
	 */
	MPD_TAG_UNKNOWN = -1,

	MPD_TAG_ARTIST,
	MPD_TAG_ALBUM,
	MPD_TAG_ALBUM_ARTIST,
	MPD_TAG_TITLE,
	MPD_TAG_TRACK,
	MPD_TAG_NAME,
	MPD_TAG_GENRE,
	MPD_TAG_DATE,
	MPD_TAG_COMPOSER,
	MPD_TAG_PERFORMER,
	MPD_TAG_COMMENT,
	MPD_TAG_DISC,

	MPD_TAG_MUSICBRAINZ_ARTISTID,
	MPD_TAG_MUSICBRAINZ_ALBUMID,
	MPD_TAG_MUSICBRAINZ_ALBUMARTISTID,
	MPD_TAG_MUSICBRAINZ_TRACKID,
	MPD_TAG_MUSICBRAINZ_RELEASETRACKID,

	MPD_TAG_ORIGINAL_DATE,

	MPD_TAG_ARTIST_SORT,
	MPD_TAG_ALBUM_ARTIST_SORT,

	MPD_TAG_ALBUM_SORT,
	MPD_TAG_LABEL,
	MPD_TAG_MUSICBRAINZ_WORKID,

	MPD_TAG_GROUPING,
	MPD_TAG_WORK,
	MPD_TAG_CONDUCTOR,

	MPD_TAG_COMPOSER_SORT,
	MPD_TAG_ENSEMBLE,
	MPD_TAG_MOVEMENT,
	MPD_TAG_MOVEMENTNUMBER,
	MPD_TAG_LOCATION,
	MPD_TAG_MOOD,
	MPD_TAG_TITLE_SORT,
	MPD_TAG_MUSICBRAINZ_RELEASEGROUPID,
	MPD_TAG_SHOWMOVEMENT,

	/* IMPORTANT: the ordering of tag types above must be
	   retained, or else the libmpdclient ABI breaks */

	MPD_TAG_COUNT
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Looks up the name of the specified tag.
 *
 * @return the name, or NULL if the tag type is not valid
 */
const char *
mpd_tag_name(enum mpd_tag_type type);

/**
 * Parses a tag name, and returns its #mpd_tag_type value.
 *
 * @return a #mpd_tag_type value, or MPD_TAG_UNKNOWN if the name was
 * not recognized
 */
enum mpd_tag_type
mpd_tag_name_parse(const char *name);

/**
 * Same as mpd_tag_name_parse(), but ignores case.
 *
 * @return a #mpd_tag_type value, or MPD_TAG_UNKNOWN if the name was
 * not recognized
 */
enum mpd_tag_type
mpd_tag_name_iparse(const char *name);

#ifdef __cplusplus
}
#endif

#endif
