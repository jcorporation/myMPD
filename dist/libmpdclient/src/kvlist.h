// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef MPD_KVLIST_H
#define MPD_KVLIST_H

#include <mpd/pair.h>
#include <mpd/compiler.h>

#include <stddef.h>

struct mpd_kvlist {
	struct mpd_kvlist_item *head, **tail_r;

	const struct mpd_kvlist_item *cursor;
	struct mpd_pair pair;
};

void
mpd_kvlist_init(struct mpd_kvlist *l);

void
mpd_kvlist_deinit(struct mpd_kvlist *l);

void
mpd_kvlist_add(struct mpd_kvlist *l, const char *key, size_t key_length,
	       const char *value);

mpd_pure
const char *
mpd_kvlist_get(const struct mpd_kvlist *l, const char *name);

const struct mpd_pair *
mpd_kvlist_first(struct mpd_kvlist *l);

const struct mpd_pair *
mpd_kvlist_next(struct mpd_kvlist *l);

#endif
