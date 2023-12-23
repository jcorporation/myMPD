// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include "config.h"
#include "kvlist.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct mpd_kvlist_item {
	struct mpd_kvlist_item *next;
	char *key, *value;
};

static void
mpd_kvlist_item_free(struct mpd_kvlist_item *i)
{
	assert(i != NULL);

	free(i->key);
	free(i->value);
	free(i);
}

void
mpd_kvlist_init(struct mpd_kvlist *l)
{
	assert(l != NULL);

	l->head = NULL;
	l->tail_r = &l->head;
	l->cursor = NULL;
}

void
mpd_kvlist_deinit(struct mpd_kvlist *l)
{
	assert(l != NULL);

	while (l->head != NULL) {
		struct mpd_kvlist_item *i = l->head;
		l->head = i->next;
		mpd_kvlist_item_free(i);
	}
}

#ifndef HAVE_STRNDUP
static char *
strndup(const char *s, size_t length)
{
	char *p = malloc(length + 1);
	if (p != NULL) {
		memcpy(p, s, length);
		p[length] = 0;
	}

	return p;
}
#endif

void
mpd_kvlist_add(struct mpd_kvlist *l, const char *key, size_t key_length,
	       const char *value)
{
	assert(l != NULL);
	assert(l->tail_r != NULL);
	assert(key != NULL);
	assert(value != NULL);

	struct mpd_kvlist_item *i = malloc(sizeof(*i));
	if (i == NULL)
		return;

	i->next = NULL;
	i->key = strndup(key, key_length);
	i->value = strdup(value);
	if (i->key == NULL || i->value == NULL) {
		mpd_kvlist_item_free(i);
		return;
	}

	*l->tail_r = i;
	l->tail_r = &i->next;
}

const char *
mpd_kvlist_get(const struct mpd_kvlist *l, const char *name)
{
	for (const struct mpd_kvlist_item *i = l->head; i != NULL; i = i->next)
		if (strcmp(name, i->key) == 0)
			return i->value;

	return NULL;
}

static const struct mpd_pair *
mpd_kvlist_item_to_pair(struct mpd_pair *buffer,
			const struct mpd_kvlist_item *item)
{
	assert(buffer != NULL);
	assert(item != NULL);

	buffer->name = item->key;
	buffer->value = item->value;
	return buffer;
}

const struct mpd_pair *
mpd_kvlist_first(struct mpd_kvlist *l)
{
	assert(l != NULL);

	if (l->head == NULL)
		return NULL;

	l->cursor = l->head;
	return mpd_kvlist_item_to_pair(&l->pair, l->cursor);
}

const struct mpd_pair *
mpd_kvlist_next(struct mpd_kvlist *l)
{
	assert(l != NULL);
	assert(l->cursor != NULL);

	if (l->cursor->next == NULL)
		return NULL;

	l->cursor = l->cursor->next;
	return mpd_kvlist_item_to_pair(&l->pair, l->cursor);
}
