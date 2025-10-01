// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#include <mpd/position.h>

const char *
mpd_position_whence_char(enum mpd_position_whence whence)
{
	switch (whence) {
	case MPD_POSITION_AFTER_CURRENT:
		return "+";

	case MPD_POSITION_BEFORE_CURRENT:
		return "-";
	default:
		return "";
	}
}
