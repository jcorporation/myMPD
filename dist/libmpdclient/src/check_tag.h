// SPDX-License-Identifier: BSD-3-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_CHECK_TAG_H
#define MPD_CHECK_TAG_H

#include "check_tag.h"

#include <mpd/compiler.h>
#include <mpd/tag.h>

struct mpd_error_info;

/**
 * Wrapper for mpd_tag_name() which sets #error to #MPD_ERROR_ARGUMENT
 * if the #type is invalid.
 */
mpd_pure
const char *
mpd_check_tag_name(enum mpd_tag_type type, struct mpd_error_info *error);

#endif
