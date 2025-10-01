// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#ifndef LIBMPDCLIENT_RESOLVER_H
#define LIBMPDCLIENT_RESOLVER_H

#include <stddef.h>

struct resolver;

struct resolver_address {
	int family;
	int protocol;
	size_t addrlen;
	const struct sockaddr *addr;
};

struct resolver *
resolver_new(const char *host, unsigned port);

void
resolver_free(struct resolver *resolver);

const struct resolver_address *
resolver_next(struct resolver *resolver);

#endif
