// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/parser.h>
#include <mpd/protocol.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct mpd_parser {
#ifndef NDEBUG
	/**
	 * This is used by assertions in the "get" functions below.
	 */
	enum mpd_parser_result result;
#endif

	union {
		bool discrete;

		struct {
			enum mpd_server_error server;
			unsigned at;
			const char *message;
		} error;

		struct {
			const char *name, *value;
		} pair;
	} u;
};

struct mpd_parser *
mpd_parser_new(void)
{
	struct mpd_parser *parser = malloc(sizeof(*parser));
	if (parser == NULL)
		return NULL;

#ifndef NDEBUG
	parser->result = MPD_PARSER_MALFORMED;
#endif

	return parser;
}

void
mpd_parser_free(struct mpd_parser *parser)
{
	free(parser);
}

static inline enum mpd_parser_result
set_result(struct mpd_parser *parser, enum mpd_parser_result result)
{
#ifndef NDEBUG
	/* this value exists only in the debug build, and is used by
	   assertions in the "get" functions below */
	parser->result = result;
#else
	/* suppress "unused" warning */
	(void)parser;
#endif

	return result;
}

enum mpd_parser_result
mpd_parser_feed(struct mpd_parser *parser, char *line)
{
	if (strcmp(line, "OK") == 0) {
		parser->u.discrete = false;
		return set_result(parser, MPD_PARSER_SUCCESS);
	} else if (strcmp(line, "list_OK") == 0) {
		parser->u.discrete = true;
		return set_result(parser, MPD_PARSER_SUCCESS);
	} else if (memcmp(line, "ACK", 3) == 0) {
		char *p, *q;

		parser->u.error.server = MPD_SERVER_ERROR_UNK;
		parser->u.error.at = 0;
		parser->u.error.message = NULL;

		/* parse [ACK@AT] */

		p = strchr(line + 3, '[');
		if (p == NULL)
			return set_result(parser, MPD_PARSER_ERROR);

		parser->u.error.server = strtol(p + 1, &p, 10);
		if (*p == '@')
			parser->u.error.at = strtoul(p + 1, &p, 10);

		q = strchr(p, ']');
		if (q == NULL)
			return set_result(parser, MPD_PARSER_MALFORMED);

		/* skip the {COMMAND} */

		p = q + 1;
		q = strchr(p, '{');
		if (q != NULL) {
			q = strchr(p, '}');
			if (q != NULL)
				p = q + 1;
		}

		/* obtain error message */

		while (*p == ' ')
			++p;

		if (*p != 0)
			parser->u.error.message = p;

		return set_result(parser, MPD_PARSER_ERROR);
	} else {
		/* so this must be a name-value pair */

		char *p;

		p = strchr(line, ':');
		if (p == NULL || p[1] != ' ')
			return set_result(parser, MPD_PARSER_MALFORMED);

		*p = 0;

		parser->u.pair.name = line;
		parser->u.pair.value = p + 2;

		return set_result(parser, MPD_PARSER_PAIR);
	}
}

bool
mpd_parser_is_discrete(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_SUCCESS);

	return parser->u.discrete;
}

enum mpd_server_error
mpd_parser_get_server_error(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_ERROR);

	return parser->u.error.server;
}

unsigned
mpd_parser_get_at(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_ERROR);

	return parser->u.error.at;
}

const char *
mpd_parser_get_message(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_ERROR);

	return parser->u.error.message;
}

const char *
mpd_parser_get_name(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_PAIR);

	return parser->u.pair.name;
}

const char *
mpd_parser_get_value(const struct mpd_parser *parser)
{
	assert(parser->result == MPD_PARSER_PAIR);

	return parser->u.pair.value;
}
