// SPDX-License-Identifier: BSD-2-Clause
// Copyright The Music Player Daemon Project

#include <mpd/settings.h>
#include "config.h"
#include "internal.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>


/**
 * This opaque object represents the connection settings used to
 * connect to a MPD server.
 * Call mpd_settings_new() to create a new instance.
 */
struct mpd_settings {
	/**
	 * The hostname, in null-terminated string form.
	 * Can also be a local socket path on UNIX systems.
	 * Should never be null after mpd_settings_new().
	 */
	char *host;

	/**
	 * The port number, as an unsigned integer.
	 * Will be 0 if the host field is a local socket path.
	 */
	unsigned port;

	/**
	 * The timeout in milliseconds, as an unsigned integer. Never zero.
	 */
	unsigned timeout_ms;

	/**
	 * The password used to connect to a MPD server, may be null.
	 */
	char *password;

	/**
	 * A pointer to the next alternative set of settings to try, if any.
	 * Null indicates there are no (more) alternatives to try.
	 */
	struct mpd_settings *next;
};

/**
 * Parses the password from the host specification in the form
 * "password@hostname".
 *
 * @param settings a settings object. both settings->host and
 * 	  settings->password may be modified by this function
 * @return true on success, false on out of memory
 *
 */
static bool
mpd_parse_host_password(struct mpd_settings *settings)
{
	char *at, *oldhost;
	size_t host_len, at_pos;

	assert(settings->password == NULL);

	if (settings->host == NULL ||
	    /* if the MPD_HOST begins with a '@' then it's not an
	       empty password but an abstract socket */
	    *settings->host == '@')
		return true;

	at = strchr(settings->host, '@');
	if (at == NULL)
		return true;

	at_pos = at - settings->host;
	settings->password = malloc(at_pos + 1);
	if (settings->password == NULL)
		return false;

	memcpy(settings->password, settings->host, at_pos);
	(settings->password)[at_pos] = 0;

	/* reallocate host, otherwise free() would not work properly */
	host_len = strlen(settings->host) - at_pos;
	oldhost = settings->host;
	settings->host = malloc(host_len);
	if (settings->host == NULL) {
		settings->host = oldhost;
		return false;
	}

	memcpy(settings->host, &oldhost[at_pos + 1], host_len - 1);
	settings->host[host_len - 1] = 0;
	free(oldhost);
	return true;
}

/**
 * Parses the host specification.  If not specified, it attempts to
 * load it from the environment variable MPD_HOST.
 *
 * @param settings a settings object. both settings->host and
 * settings->password may be modified by this function
 * @return true on success, false on out of memory
 */
static bool
mpd_check_host(struct mpd_settings *settings)
{
	const char *host_getenv = getenv("MPD_HOST");

	assert(settings->password == NULL);

	if (settings->host == NULL && host_getenv != NULL) {
		/* getent should not be freed (mpd_settings_free()); hence we
		 * allocate a new string */
		settings->host = strdup(host_getenv);
		if (settings->host == NULL)
			return false;
	}

	if (settings->host != NULL) {
		if (!mpd_parse_host_password(settings))
			return false;
	}

	return true;
}

/**
 * Parses the port specification.  If not specified (0), it attempts
 * to load it from the environment variable MPD_PORT.
 */
static unsigned
mpd_check_port(unsigned port)
{
	if (port == 0) {
		const char *env_port = getenv("MPD_PORT");
		if (env_port != NULL)
			port = strtoul(env_port, NULL, 10);
	}

	return port;
}

static unsigned
mpd_default_timeout_ms(void)
{
	const char *timeout_string = getenv("MPD_TIMEOUT");
	if (timeout_string != NULL) {
		const unsigned timeout_s = strtoul(timeout_string, NULL, 10);
		if (timeout_s > 0)
			return timeout_s * 1000;
	}

	/* 30s is the default */
	return 30000;
}

struct mpd_settings *
mpd_settings_new(const char *host, unsigned port, unsigned timeout_ms,
		 const char *reserved, const char *password)
{
	(void)reserved;

	struct mpd_settings *settings = malloc(sizeof(*settings));
	if (settings == NULL)
		return settings;

	settings->next = NULL;

	if (host != NULL) {
		settings->host = strdup(host);
		if (settings->host == NULL) {
			free(settings);
			return NULL;
		}
	} else
		settings->host = NULL;

	settings->password = NULL;

	port = mpd_check_port(port);

	if (!mpd_check_host(settings)) {
		mpd_settings_free(settings);
		return NULL;
	}

	if (settings->password == NULL && password != NULL) {
		settings->password = strdup(password);
		if (settings->password == NULL) {
			free(settings->host);
			free(settings);
			return NULL;
		}
	}

	if (settings->host == NULL) {
#ifdef DEFAULT_SOCKET
		if (port == 0) {
			/* default to local socket only if no port was
			   explicitly configured */
#ifdef ENABLE_TCP
			settings->next = mpd_settings_new(DEFAULT_HOST, DEFAULT_PORT, timeout_ms,
							  reserved, password);
			if (settings->next == NULL) {
				mpd_settings_free(settings);
				return NULL;
			}
#endif

			settings->host = strdup(DEFAULT_SOCKET);
		} else
#endif
			settings->host = strdup(DEFAULT_HOST);

		if (settings->host == NULL) {
			free(settings->password);
			free(settings);
			return NULL;
		}
	}

	settings->timeout_ms = timeout_ms != 0
		? timeout_ms
		: mpd_default_timeout_ms();

	settings->port = settings->host[0] == '/' ||
		 settings->host[0] == '@'
		? 0 /* no port for local socket */
		: (port != 0 ? port : DEFAULT_PORT);

	return settings;
}

void
mpd_settings_free(struct mpd_settings *settings)
{
	if (settings->next != NULL)
		mpd_settings_free(settings->next);
	free(settings->host);
	free(settings->password);
	free(settings);
}

const char *
mpd_settings_get_host(const struct mpd_settings *settings)
{
	return settings->host;
}

unsigned
mpd_settings_get_port(const struct mpd_settings *settings)
{
	return settings->port;
}

unsigned
mpd_settings_get_timeout_ms(const struct mpd_settings *settings)
{
	return settings->timeout_ms;
}

const char *
mpd_settings_get_password(const struct mpd_settings *settings)
{
	return settings->password;
}

const struct mpd_settings *
mpd_settings_get_next(const struct mpd_settings *settings)
{
	return settings->next;
}
