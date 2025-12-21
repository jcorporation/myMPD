Configuration files
===================

You can configure myMPD in different ways:

1. With environment variables

   1. Set environment variables as described below
   2. Start myMPD. myMPD grabs the environment variables and writes it's configuration files accordingly.

2. Use ``mympd-config``

   1. Type ``mympd-config`` to configure myMPD with a ncurses based
      interface.
   2. Start myMPD.

3. Use ``mympd -c``

   1. Type ``mympd -c`` to create the initial configuration in the
      ``/var/lib/mympd/config/`` directory.
   2. Edit the files and start myMPD.

.. hint::
   Use :doc:`systemd-run </030-running>`, if you use a distribution with systemd, e.g.:

   .. code-block:: lua

      systemd-run -p DynamicUser=yes -p StateDirectory=mympd -p CacheDirectory=mympd -E MYMPD_LOGLEVEL=4 -E MYMPD_HTTP=false -E MYMPD_SSL_PORT=1333 mympd -c

General options
---------------

+---------------------------------------+---------+----------------+------------------------------------------------------+
| FILE / ENVIRONMENT                    | TYPE    | DEFAULT        | DESCRIPTION                                          |
+=======================================+=========+================+======================================================+
|| acl                                  | string  |                | ACL to access the myMPD webserver: :doc:`ACL <acl>`, |
|| MYMPD_ACL                            |         |                | allows all hosts in the default configuration.       |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| album_group_tag                      | string  |  ``Date``      | Additional tag to group albums                       |
|| MYMPD_ALBUM_GROUP_TAG                |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| album_mode                           | string  |  ``adv``       | Set the album mode: `adv` or `simple`                |
|| MYMPD_ALBUM_MODE                     |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| album_unknown                        | boolean | ``false``      | Groups songs with empty album tag in a special       |
|| MYMPD_ALBUM_UNKNOWN                  |         |                | `Unknown Album` album.                               |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cache_cover_keep_days                | number  | ``31``         || How long to keep images in the cover cache:         |
|| MYMPD_CACHE_COVER_KEEP_DAYS          |         |                || 0 = disable the cache                               |
|                                       |         |                || -1 =o disable pruning of the cache                  |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cache_http_keep_days                 | number  | ``31``         || How long to keep successful responses in the http   |
|| MYMPD_CACHE_HTTP_KEEP_DAYS           |         |                || client cache:                                       |
|                                       |         |                || 0 = disable the cache                               |
|                                       |         |                || -1 = disable pruning of the cache.                  |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cache_lyrics_keep_days               | number  | ``31``         || How long to keep lyrics in the lyrics cache:        |
|| MYMPD_CACHE_LYRICS_KEEP_DAYS         |         |                || 0 = disable the cache                               |
|                                       |         |                || -1 = disable pruning of thecache                    |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cache_misc_keep_days                 | number  | ``1``          | How long to keep files in the misc cache.            |
|| MYMPD_CACHE_MISC_KEEP_DAYS           |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cache_thumbs_keep_days               | number  | ``31``         || How long to keep images in the thumbnail cache:     |
|| MYMPD_CACHE_THUMBS_KEEP_DAYS         |         |                || 0 = disable the cache                               |
|                                       |         |                || -1 = disable pruning of the cache                   |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| ca_cert_store                        | string  | [2]_           | Path to the system CA certificate store.             |
|| MYMPD_CA_CERT_STORE                  |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| cert_check                           | boolean | ``true``       | Enable certificate checking for outgoing https       |
|| MYMPD_CERT_CHECK                     |         |                | connections.                                         |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| http                                 | boolean | ``true``       | `true` = Enable listening on http_port               |
|| MYMPD_HTTP                           |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| http_host                            | string  | ``[::]``       | IP address to listen on, use ``[::]`` to listen on   |
|| MYMPD_HTTP_HOST                      |         |                | IPv6 and IPv4.                                       |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| http_port                            | number  | ``8080``       | Port to listen for plain http requests. Redirects to |
|| MYMPD_HTTP_PORT                      |         |                | ``ssl_port`` if ``ssl`` is set to ``true``. [1]_     |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| jukebox_queue_length_album           | number  | ``25``         | Length of the internal jukebox queue for albums      |
|| MYMPD_JUKEBOX_QUEUE_LENGTH_ALBUM     |         |                | (5 - 250).                                           |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| jukebox_queue_length_album_min       | number  | ``5``          | Minimum length of the internal jukebox queue for     |
|| MYMPD_JUKEBOX_QUEUE_LENGTH_ALBUM_MIN |         |                | albums (5 - 125).                                    |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| jukebox_queue_length_song            | number  | ``100``        | Desired Length of the internal jukebox queue for     |
|| MYMPD_JUKEBOX_QUEUE_LENGTH_SONG      |         |                | songs (10 - 1000).                                   |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| jukebox_queue_length_song_min        | number  | ``10``         | Minimum length of the internal jukebox queue for     |
|| MYMPD_JUKEBOX_QUEUE_LENGTH_SONG_MIN  |         |                | songs (10 - 500).                                    |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| loglevel                             | number  | ``5``          | :doc:`Logging <logging>`                             |
|| MYMPD_LOGLEVEL                       |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| mympd_uri                            | string  | ``auto``       | ``auto`` or uri to myMPD listening port,             |
|| MYMPD_MYMPD_URI                      |         |                | e.g. ``https://192.168.1.1/mympd``                   |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| pin_hash                             | string  |                | SHA256 hash of pin, create it with ``mympd -p``      |
|| N/A                                  |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| scriptacl                            | string  | ``+127.0.0.1`` | ACL for the myMPD script backend: :doc:`ACL <acl>`,  |
|| MYMPD_SCRIPTACL                      |         |                | The acl above must also grant access.                |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| scripts_external                     | boolean | ``false``      | Allow myMPD to execute external scripts vie the      |
|| MYMPD_SCRIPTS_EXTERNAL               |         |                | `/script-api`-Endpoint.                              |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| stickers                             | boolean | ``true``       | Enables the support for MPD stickers.                |
|| MYMPD_STICKERS                       |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| stickers_pad_int                     | boolean | ``false``      | Enables the padding of integer sticker values        |
|| MYMPD_STICKERS_PAD_INT               |         |                | (12 digits).                                         |
+---------------------------------------+---------+----------------+------------------------------------------------------+
|| webradiodb                           | boolean | ``true``       | Enables the WebradioDB integration.                  |
|| MYMPD_WEBRADIODB                     |         |                |                                                      |
+---------------------------------------+---------+----------------+------------------------------------------------------+

.. [1] If http_port is disabled: The MPD curl plugin must trust the myMPD CA or certificate checking must be disabled. MPD fetches webradio playlists with http(s) from myMPD webserver.

.. [2] myMPD checks following locations for the ca cert store file:

   - ``/etc/ssl/certs/ca-certificates.crt``
   - ``/etc/ssl/certs/ca-bundle.crt``
   - ``/etc/pki/tls/certs/ca-bundle.crt``
   - ``/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem``

SSL options
-----------

+--------------------------------------+---------+-----------+--------------------------------------------------------+
| FILE / ENVIRONMENT                   | TYPE    | DEFAULT   | DESCRIPTION                                            |
+======================================+=========+===========+========================================================+
|| ssl                                 | boolean | ``true``  | ``true``` = enable listening on ssl_port, enables also |
|| MYMPD_SSL                           |         |           | the redirection from http_port to ssl_port.            |
+--------------------------------------+---------+-----------+--------------------------------------------------------+
|| ssl_port                            | number  | ``8443``  | Port to listen for https requests.                     |
|| MYMPD_SSL_PORT                      |         |           |                                                        |
+--------------------------------------+---------+-----------+--------------------------------------------------------+
|| ssl_san                             | string  |           | Additional SAN for certificate creation.               |
|| MYMPD_SSL_SAN                       |         |           |                                                        |
+--------------------------------------+---------+-----------+--------------------------------------------------------+
|| custom_cert                         | boolean | ``false`` | `true` = use custom ssl key and certificate.           |
|| MYMPD_CUSTOM_CERT                   |         |           |                                                        |
+--------------------------------------+---------+-----------+--------------------------------------------------------+
|| ssl_cert                            | string  |           | Path to custom ssl certificate file.                   |
|| MYMPD_SSL_CERT                      |         |           |                                                        |
+--------------------------------------+---------+-----------+--------------------------------------------------------+
|| ssl_key                             | string  |           | Path to custom ssl key file.                           |
|| MYMPD_SSL_KEY                       |         |           |                                                        |
+--------------------------------------+---------+-----------+--------------------------------------------------------+

- More details on :doc:`SSL <ssl>`
