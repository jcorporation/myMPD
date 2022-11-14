-- {"order":1,"arguments":[]}

mympd.api("MYMPD_API_PLAYER_STOP");
mympd.api("MYMPD_API_PLAYER_OPTIONS_SET", {
    jukeboxMode = "song",
    jukeboxPlaylist = "Database",
    jukeboxQueueLength = 1,
    jukeboxUniqueTag = "Title",
    jukeboxLastPlayed = 24,
    jukeboxIgnoreHated = true,
    consume = "1"
});
mympd.api("MYMPD_API_QUEUE_CLEAR");
mympd.api("MYMPD_API_PLAYER_PLAY");
