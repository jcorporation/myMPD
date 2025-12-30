Accessing myMPD and MPD status information
==========================================

Populates the lua table ``mympd_state`` with configuration values and
current status of myMPD and MPD.

.. code:: lua

   mympd.init()

**Parameters:**

No parameters required.

**Returns:**

====== ========= =====================================
FIELD  TYPE      DESCRIPTION
====== ========= =====================================
rc     integer   response code: 0 = success, 1 = error
result lua table jsonrpc result or error as lua table
====== ========= =====================================

The lua table mympd_state is populated with following fields.

mympd_state
-----------

+-----------+---------------+-------------------------------------------+
| KEY       | TYPE          | DESCRIPTION                               |
+===========+===============+===========================================+
| ``au      | Boolean       | true = enabled, false = disabled          |
| to_play`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``consum  | Integer       | MPD consume state: 0 = off, 1 = on, 2 =   |
| e_state`` |               | oneshot, 3 = unknown                      |
+-----------+---------------+-------------------------------------------+
| ``curren  | String        | myMPD AlbumId                             |
| t_album`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``cr      | Integer       | MPD crossfade option                      |
| ossfade`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``curre   | Table         | Current MPD song                          |
| nt_song`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``elaps   | Integer       | Elapsed time of current song              |
| ed_time`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``juke    | Boolean       | Ignore hated songs for jukebox            |
| box_ignor |               |                                           |
| e_hated`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``juk     | Integer       | Don’t add songs that are played in the    |
| ebox_last |               | last x hours                              |
| _played`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``        | Integer       | Only songs with this minimum length will  |
| jukebox_m |               | be considered.                            |
| ax_song_d |               |                                           |
| uration`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``        | Integer       | If greater then zero: Only songs with     |
| jukebox_m |               | this maximum length will be considered.   |
| in_song_d |               |                                           |
| uration`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``jukeb   | Integer       | Jukebox mode: 0 = off, 1 = song, 2 =      |
| ox_mode`` |               | album, 3 = script                         |
+-----------+---------------+-------------------------------------------+
| ``        | String        | Jukebox playlist: Database or MPD         |
| jukebox_p |               | playlist name                             |
| laylist`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``juke    | Integer       | Number of songs in the queue before the   |
| box_queue |               | jukebox add’s more songs.                 |
| _length`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``        | String        | Build the jukebox queue with this tag as  |
| jukebox_u |               | uniq constraint: Song, Album, Artist      |
| niq_tag`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``li      | String        | ListenBrainz Token                        |
| stenbrain |               |                                           |
| z_token`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``mixra   | Float         | Mixramp delay                             |
| mpdelay`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``mi      | Float         | Mixramp DB                                |
| xrampdb`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| `         | String        | Path to the mpd music directory           |
| `music_di |               |                                           |
| rectory`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``my      | String        | Canonical myMPD uri                       |
| mpd_uri`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| `         | String        | Canonical myMPD uri (http://)             |
| `mympd_ur |               |                                           |
| i_plain`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``next_   | Integer       | Next song id in queue                     |
| song_id`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``next_s  | Integer       | Next song position in queue               |
| ong_pos`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``pla     | Integer       | Player state: 0 = unknown, 1 = stop, 2 =  |
| y_state`` |               | play, 3 = pause                           |
+-----------+---------------+-------------------------------------------+
| ``pl      | String        | path to the mpd playlist directory        |
| aylist_di |               |                                           |
| rectory`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``queue   | Integer       | Length of the queue                       |
| _length`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``queue_  | Integer       | Version of the queue                      |
| version`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| `         | Boolean       | MPD repeat option                         |
| `repeat`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``rep     | Integer       | 0 = off, 1 = track, 2 = album, 3 = auto,  |
| laygain`` |               | 4 = unknown                               |
+-----------+---------------+-------------------------------------------+
| `         | Boolean       | MPD random option                         |
| `random`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``singl   | Integer       | MPD single state: 0 = off, 1 = on, 2 =    |
| e_state`` |               | oneshot, 3 = unknown                      |
+-----------+---------------+-------------------------------------------+
| ``        | Integer       | Song id of current song                   |
| song_id`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``s       | Integer       | Current song position in queue            |
| ong_pos`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``sta     | Integer       | Current song start playing timestamp      |
| rt_time`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| ``tot     | Integer       | Total time of current song                |
| al_time`` |               |                                           |
+-----------+---------------+-------------------------------------------+
| `         | Integer       | 0 - 100 percent                           |
| `volume`` |               |                                           |
+-----------+---------------+-------------------------------------------+

mympd_state.current_song
~~~~~~~~~~~~~~~~~~~~~~~~

============ ============ =========================
KEY          TYPE         DESCRIPTION
============ ============ =========================
``uri``      String       Song uri
``Duration`` Integer      Song duration in seconds
Tag          Tag value(s) MPD tag name with values.
============ ============ =========================
