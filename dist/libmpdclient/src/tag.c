// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/tag.h>

#include <assert.h>
#include <string.h>
#include <stdbool.h>

static const char *const mpd_tag_type_names[MPD_TAG_COUNT] =
{
	[MPD_TAG_ARTIST] = "Artist",
	[MPD_TAG_ARTIST_SORT] = "ArtistSort",
	[MPD_TAG_ALBUM] = "Album",
	[MPD_TAG_ALBUM_SORT] = "AlbumSort",
	[MPD_TAG_ALBUM_ARTIST] = "AlbumArtist",
	[MPD_TAG_ALBUM_ARTIST_SORT] = "AlbumArtistSort",
	[MPD_TAG_TITLE] = "Title",
	[MPD_TAG_TRACK] = "Track",
	[MPD_TAG_NAME] = "Name",
	[MPD_TAG_GENRE] = "Genre",
	[MPD_TAG_DATE] = "Date",
	[MPD_TAG_COMPOSER] = "Composer",
	[MPD_TAG_PERFORMER] = "Performer",
	[MPD_TAG_COMMENT] = "Comment",
	[MPD_TAG_DISC] = "Disc",
	[MPD_TAG_LABEL] = "Label",

	[MPD_TAG_MUSICBRAINZ_ARTISTID] = "MUSICBRAINZ_ARTISTID",
	[MPD_TAG_MUSICBRAINZ_ALBUMID] = "MUSICBRAINZ_ALBUMID",
	[MPD_TAG_MUSICBRAINZ_ALBUMARTISTID] = "MUSICBRAINZ_ALBUMARTISTID",
	[MPD_TAG_MUSICBRAINZ_TRACKID] = "MUSICBRAINZ_TRACKID",
	[MPD_TAG_MUSICBRAINZ_RELEASETRACKID] = "MUSICBRAINZ_RELEASETRACKID",
	[MPD_TAG_MUSICBRAINZ_WORKID] = "MUSICBRAINZ_WORKID",

	[MPD_TAG_ORIGINAL_DATE] = "OriginalDate",

	[MPD_TAG_GROUPING] = "Grouping",
	[MPD_TAG_WORK] = "Work",
	[MPD_TAG_CONDUCTOR] = "Conductor",

	[MPD_TAG_COMPOSER_SORT] = "ComposerSort",
	[MPD_TAG_ENSEMBLE] = "Ensemble",
	[MPD_TAG_MOVEMENT] = "Movement",
	[MPD_TAG_MOVEMENTNUMBER] = "MovementNumber",
	[MPD_TAG_LOCATION] = "Location",
	[MPD_TAG_MOOD] = "Mood",
	[MPD_TAG_TITLE_SORT] = "TitleSort",
	[MPD_TAG_MUSICBRAINZ_RELEASEGROUPID] = "MUSICBRAINZ_RELEASEGROUPID",
	[MPD_TAG_SHOWMOVEMENT] = "ShowMovement",
};

const char *
mpd_tag_name(enum mpd_tag_type type)
{
	if ((unsigned)type >= MPD_TAG_COUNT)
		return NULL;

	return mpd_tag_type_names[type];
}

enum mpd_tag_type
mpd_tag_name_parse(const char *name)
{
	assert(name != NULL);

	for (unsigned i = 0; i < MPD_TAG_COUNT; ++i)
		if (strcmp(name, mpd_tag_type_names[i]) == 0)
			return (enum mpd_tag_type)i;

	return MPD_TAG_UNKNOWN;
}

/**
 * This implementation is limited to ASCII letters.  Cheap, and good
 * enough for us: all valid tag names are hard-coded above.
 */
static inline bool
ignore_case_char_equals(const char a, const char b)
{
	return (a & ~0x20) == (b & ~0x20);
}

static bool
ignore_case_string_equals(const char *a, const char *b)
{
	assert(a != NULL);
	assert(b != NULL);

	while (*a != 0) {
		if (!ignore_case_char_equals(*a, *b))
			return false;

		++a;
		++b;
	}

	return *b == 0;
}

enum mpd_tag_type
mpd_tag_name_iparse(const char *name)
{
	assert(name != NULL);

	for (unsigned i = 0; i < MPD_TAG_COUNT; ++i)
		if (ignore_case_string_equals(name, mpd_tag_type_names[i]))
			return (enum mpd_tag_type)i;

	return MPD_TAG_UNKNOWN;
}
