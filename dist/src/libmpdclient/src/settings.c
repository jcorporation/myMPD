/* libmpdclient
   (c) 2003-2019 The Music Player Daemon Project
   This project's homepage is: http://www.musicpd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <mpd/settings.h>
#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct mpd_settings {
	char *host;

	unsigned port, timeout_ms;

	char *password;
};

/**
 * Parses the password from the host specification in the form
 * "password@hostname".
 *
 * @param host_p a pointer to the "host" variable, which may be
 * modified by this function
 * @return an allocated password string, or NULL if there was no
 * password
 */
static const char *
mpd_parse_host_password(const char *host, char **password_r)
{
	const char *at;
	char *password;

	assert(password_r != NULL);
	assert(*password_r == NULL);

	if (host == NULL ||
	    /* if the MPD_HOST begins with a '@' then it's not an
	       empty password but an abstract socket */
	    *host == '@')
		return host;

	at = strchr(host, '@');
	if (at == NULL)
		return host;

	password = malloc(at - host + 1);
	if (password != NULL) {
		/* silently ignoring out-of-memory */
		memcpy(password, host, at - host);
		password[at - host] = 0;
		*password_r = password;
	}

	return at + 1;
}

/**
 * Parses the host specification.  If not specified, it attempts to
 * load it from the environment variable MPD_HOST.
 */
static const char *
mpd_check_host(const char *host, char **password_r)
{
	assert(password_r != NULL);
	assert(*password_r == NULL);

	if (host == NULL)
		host = getenv("MPD_HOST");

	if (host != NULL)
		host = mpd_parse_host_password(host, password_r);

	return host;
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
			port = atoi(env_port);
	}

	return port;
}

static unsigned
mpd_default_timeout_ms(void)
{
	const char *timeout_string = getenv("MPD_TIMEOUT");
	if (timeout_string != NULL) {
		int timeout_s = atoi(timeout_string);
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

	settings->password = NULL;

	port = mpd_check_port(port);

	host = mpd_check_host(host, &settings->password);
	if (settings->password == NULL && password != NULL)
		settings->password = strdup(password);

	if (host == NULL) {
#ifdef DEFAULT_SOCKET
		if (port == 0)
			/* default to local socket only if no port was
			   explicitly configured */
			host = DEFAULT_SOCKET;
		else
#endif
			host = DEFAULT_HOST;
	}

	settings->host = strdup(host);

	settings->timeout_ms = timeout_ms != 0
		? timeout_ms
		: mpd_default_timeout_ms();

	settings->port = host[0] == '/' || host[0] == '@'
		? 0 /* no port for local socket */
		: (port != 0 ? port : DEFAULT_PORT);

	return settings;
}

void
mpd_settings_free(struct mpd_settings *settings)
{
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
