-- {"order":1,"arguments":[]}

mympd_api("MYMPD_API_PLAYER_STOP");
mympd_api("MYMPD_API_PLAYER_OPTIONS_SET", "jukeboxMode", 1, "jukeboxPlaylist", "Database", "jukeboxQueueLength", 1, "jukeboxUniqueTag", "Title", "jukeboxLastPlayed", 24, "consume", 1);
mympd_api("MYMPD_API_QUEUE_CLEAR");
mympd_api("MYMPD_API_PLAYER_PLAY");
