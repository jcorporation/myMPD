-- {"order":1,"arguments":[]}

mympd_api("MPD_API_PLAYER_STOP");
mympd_api("MYMPD_API_SETTINGS_SET", "jukeboxMode", 1, "jukeboxPlaylist", "AllSongs", "jukeboxQueueLength", 1, "jukeboxUniqueTag", "Title", "jukeboxLastPlayed", 24);
mympd_api("MPD_API_QUEUE_CLEAR");
mympd_api("MPD_API_PLAYER_PLAY");
