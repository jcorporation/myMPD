"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

const params = {
    "offset": {
            "type": "uint",
            "example": 0,
            "desc": "start offset of the returned list"
    },
    "limit": {
        "type": "uint",
        "example": 50,
        "desc": "maximum number of elements to return"
    },
    "sort": {
        "type": "text",
        "example": "Title",
        "desc": "tag to sort the result"
    },
    "sortdesc": {
        "type": "bool",
        "example": false,
        "desc": "false = ascending, true = descending sort"
    },
    "cols": {
        "type": "array",
        "example": "[\"Artist\", \"Album\", \"Title\"]",
        "desc": "array of columns to return"
    },
    "expression": {
        "type": "text",
        "example": "((any contains 'tabula'))",
        "desc": "MPD search expression"
    },
    "searchstr": {
        "type": "text",
        "example": "tabula",
        "desc": "string to search"
    },
    "uri": {
        "type": "text",
        "example": "Testfiles/Sp.mp3",
        "desc": "relativ song uri"
    },
    "filter": {
        "type": "text",
        "example": "Title",
        "desc": "tag to search or \"any\" for all tags"
    },
    "from": {
        "type": "uint",
        "example": 2,
        "desc": "From position",
    },
    "to": { 
        "type": "uint",
        "example": 1,
        "desc": "To position"
    },
    "plist": {
        "type": "text",
        "example": "test_plist",
        "desc": "MPD playlist name"
    },
    "sortShuffle": {
        "type": "text",
        "example": "shuffle",
        "desc": "blank = no sorting, shuffle = shuffle, tagname = sort by tag"
    },
    "songId": {
        "type": "uint",
        "example": 1,
        "desc": "MPD queue song id"
    }
};

const cmds = {
    "MYMPD_API_DATABASE_SEARCH_ADV": {
        "desc": "Searches for songs in the database (new interface).",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "expression": params.expression,
            "sort": params.sort,
            "sortdesc": params.sortdesc,
            "plist": {
                "type": "text",
                "example": "queue",
                "desc": "playlist to add results to, use \"queue\" to add search to queue"
            },
            "cols": params.cols,
            "replace": {
                "type": "bool",
                "example": "false",
                "desc": "true = replaces the queue, false = append to qeue"
            }
        }
    },
    "MYMPD_API_DATABASE_SEARCH": {
        "desc": "Searches for songs in the database (deprecated interface).",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "filter": params.filter,
            "searchstr": params.searchstr,
            "plist": {
                "type": "text",
                "example": "queue",
                "desc": "MPD playlist to add results to, use \"queue\" to add search to queue"
            },
            "cols": params.cols,
            "replace": {
                "type": "bool",
                "example": "false",
                "desc": "true = replaces the queue, false = append to qeue"
            }
        }
    },
    "MYMPD_API_DATABASE_UPDATE": {
        "desc": "Updates the database.",
        "params": {
            "uri": {
                "type": "text",
                "example": "Alben",
                "desc": "Root directory for update"
            }
        }
    },
    "MYMPD_API_DATABASE_RESCAN": {
        "desc": "Rescans the database.",
        "params": {
            "uri": {
                "type": "text",
                "example": "Alben",
                "desc": "Root directory for rescan"
            }
        }
    },
    "MYMPD_API_DATABASE_FILESYSTEM_LIST": {
        "desc": "Lists directories, songs and playlists.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "searchstr": params.searchstr,
            "path": {
                "type": "text",
                "example": "Alben",
                "desc": "Directory to list"
            },
            "cols": params.cols
        }
    },
    "MYMPD_API_DATABASE_GET_ALBUMS": {
        "desc": "Lists unique albums.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "expression": params.expression,
            "sort": params.sort,
            "sortdesc": params.sortdesc
        }
    },
    "MYMPD_API_DATABASE_TAG_LIST": {
        "desc": "Lists unique tag values.",
        "params":{
            "offset": params.offset,
            "limit": params.limit,
            "searchstr": params.searchstr,
            "tag": {
                "type": "text",
                "example": "Genre",
                "desc": "Tag to display"
            }
        }
    },
    "MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST": {
        "desc": "Displays songs of an album.",
        "params": {
            "album": {
                "type": "text",
                "example": "Tabula Rasa",
                "desc": "Album to display"
            },
            "albumartist": {
                "type": "text",
                "example": "Einstürzende Neubauten",
                "desc": "Albumartist"
            },
            "cols": params.cols
        }
    },
    "MYMPD_API_DATABASE_STATS": {
        "desc": "Shows MPD database statistics.",
        "params": {}
    },
    "MYMPD_API_DATABASE_SONGDETAILS": {
        "desc": "Shows all details of a song.",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_DATABASE_FINGERPRINT": {
        "desc": "Calculates the chromaprint fingerprint",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_CLEAR": {
        "desc": "Clears the queue.",
        "params": {}
    },
    "MYMPD_API_QUEUE_CROP": {
        "desc": "Crops the queue (removes all songs except playing one)",
        "params": {}
    },
    "MYMPD_API_QUEUE_CROP_OR_CLEAR": {
        "desc": "Clears (if only one song is in queue) or crops the queue",
        "params": {}
    },
    "MYMPD_API_QUEUE_ADD_RANDOM": {
        "desc": "Adds random songs or albums to the queue.",
        "params": {
            "plist": {
                "type": "text",
                "example": "Database",
                "desc": "Name of mpd playlist or \"Database\""
            },
            "quantity": {
                "type": "uint",
                "example": 10,
                "desc": "Number of songs or albums to add"
            },
            "mode": {
                "type": "uint",
                "example": 1,
                "desc": "1 = add songs, 2 = add albums"
            }
        }
    },
    "MYMPD_API_QUEUE_SAVE": {
        "desc": "Saves the queue as playlist.",
        "params": {
            "plist": {
                "type": "text",
                "example": "test_pl",
                "desc": "Playlist name"
            }
        }
    },
    "MYMPD_API_QUEUE_LIST": {
        "desc": "List the songs from the queue.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "cols": params.cols
        }
    },
    "MYMPD_API_QUEUE_SEARCH": {
        "desc": "Searches the queue.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "filter": params.filter,
            "searchstr": params.searchstr,
            "cols": params.cols
        }
    },
    "MYMPD_API_QUEUE_RM_SONG": {
        "desc": "Removes the song from the queue.",
        "params": {
            "songId": params.songId
        }
    },
    "MYMPD_API_QUEUE_RM_RANGE": {
        "desc": "Removes a range from the queue.",
        "params": {
            "start": {
                "type": "uint",
                "example": 0,
                "desc": "Start queue position",
            },
            "end": {
                "type": "uint",
                "example": 1,
                "desc": "End queue position"
            }
        }
    },
    "MYMPD_API_QUEUE_MOVE_SONG": {
        "desc": "Moves a song in the queue.",
        "params": {
            "from": params.from,
            "to": params.to
        }
    },
    "MYMPD_API_QUEUE_ADD_URI_AFTER": {
        "desc": "Adds song(s) to distinct position in queue.",
        "params": {
            "uri": params.uri,
            "to": params.to
        }
    },
    "MYMPD_API_QUEUE_ADD_URI": {
        "desc": "Appends song(s) to the queue.",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_ADD_PLAY_URI": {
        "desc": "Appends song(s) to queue queue and plays it.",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_REPLACE_URI": {
        "desc": "Replaces the queue with song(s).",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_ADD_PLAYLIST": {
        "desc": "Appends the playlist to the queue.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_QUEUE_REPLACE_PLAYLIST": {
        "desc": "Replaces the queue with the playlist.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_QUEUE_SHUFFLE": {
        "desc": "Shuffles the queue.",
        "params": {}
    },
    "MYMPD_API_QUEUE_PRIO_SET_HIGHEST": {
        "desc": "Set highest prio for specified song",
        "params": {
            "songId": params.songId
        }
    },
    "MYMPD_API_QUEUE_LAST_PLAYED": {
        "desc": "Lists the last played songs.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "cols": params.cols
        }
    },
    "MYMPD_API_PLAYLIST_RM": {
        "desc": "Removes the MPD playlist.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_PLAYLIST_CLEAR": {
        "desc": "Clears the MPD playlist.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_PLAYLIST_RENAME": {
        "desc": "Renames the MPD playlist.",
        "params": {
            "plist": {
                "type": "text",
                "example": "test_plist",
                "desc": "MPD playlist to rename"
            },
            "newName": {
                "type": "text",
                "example": "test_plist_renamed",
                "desc": "New MPD playlist name"
            }
        }
    },
    "MYMPD_API_PLAYLIST_MOVE_SONG": {
        "desc": "Moves a song in the playlist.",
        "params": {
            "plist": params.plist,
            "from": params.from,
            "to": params.to
        }
    },
    "MYMPD_API_PLAYLIST_ADD_URI": {
        "desc": "Appens a song to the playlist",
        "params": {
            "plist": params.plist,
            "uri": params.uri
        }
    },
    "MYMPD_API_PLAYLIST_RM_SONG": {
        "desc": "Removes a song from the playlist.",
        "params": {
            "plist": params.plist,
            "pos": {
                "type": "uint",
                "example": 2,
                "desc": "Position of song"
            }
        }
    },
    "MYMPD_API_PLAYLIST_RM_ALL": {
        "desc": "Batch removes playlists.",
        "params": {
            "type": {
                "type": "text",
                "example": "empty",
                "desc": "valid values are: \"deleteEmptyPlaylists\", \"deleteSmartPlaylists\", \"deleteAllPlaylists\""
            }
        }
    },
    "MYMPD_API_PLAYLIST_LIST": {
        "desc": "Lists all MPD playlists.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "searchstr": params.seachstr
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_LIST": {
        "desc": "Lists songs in the playlist.",
        "params": {
            "plist": params.plist,
            "offset": params.offset,
            "limit": params.limit,
            "searchstr": params.searchstr, 
            "cols": params.cols
        }
    },
    "MYMPD_API_PLAYLIST_SHUFFLE": {
        "desc": "Shuffles the playlist.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_PLAYLIST_SORT": {
        "desc": "Sorts the playlist.",
        "params": {
            "plist": params.plist,
            "tag": {
                "type": "text",
                "example": "Artist",
                "desc": "Tag to sort"
            }
        }
    },
    "MYMPD_API_SMARTPLS_UPDATE_ALL": {
        "desc": "Updates all smart playlists.",
        "params": {
            "force": {
                "type": "bool",
                "example": "false",
                "desc": "true = forces an update"
            }
        }
    },
    "MYMPD_API_SMARTPLS_UPDATE": { 
        "desc": "Updates the smart playlist.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_SMARTPLS_NEWEST_SAVE": {
        "desc": "Saves a smart playlist of type newest songs.",
        "params": {
            "plist": params.plist,
            "timerange": {
                "type": "uint",
                "example": 604800,
                "desc":"timerange in seconds"
            },
            "sort": params.shortShuffle
        }
    },
    "MYMPD_API_SMARTPLS_STICKER_SAVE": {
        "desc": "Saves a sticker search as a smart playlist.",
        "params": {
            "plist": params.plist,
            "sticker": {
                "type": "text",
                "example": "like",
                "desc":"Sticker name"
            },
            "maxentries": {
                "type": "uint",
                "example": 200,
                "desc": "maximum entries"
            },
            "minvalue": {
                "type": "uint",
                "example": 2,
                "desc": "minimum integer value"
            },
            "sort": params.shortShuffle
        }
    },
    "MYMPD_API_SMARTPLS_SEARCH_SAVE": {
        "desc": "Saves a search expression as a smart playlist.",
        "params": {
            "plist": params.plist,
            "expression": params.expression,
            "sort": params.shortShuffle
        }
    },
    "MYMPD_API_SMARTPLS_GET": {
        "desc": "Gets the smart playlist options.",
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_PLAYER_PLAY_SONG": {
        "desc": "Starts playing the specified song.",
        "params": {
            "songId": params.songId
        }
    },
    "MYMPD_API_PLAYER_VOLUME_SET": {
        "desc": "Sets the volume.", 
        "params": {
            "volume": {
                "type": "uint",
                "example": 50,
                "desc": "volume percent"
            }
        }
    },
    "MYMPD_API_PLAYER_VOLUME_GET": {
        "desc": "Gets the volume.",
        "params": {}
    },
    "MYMPD_API_PLAYER_PAUSE": {
        "desc": "Pauses the current playing song.",
        "params": {}
    },
    "MYMPD_API_PLAYER_RESUME": {
        "desc": "Resumes the current paused song.",
        "params": {}
    },
    "MYMPD_API_PLAYER_PLAY": {
        "desc": "Starts playing.",
        "params": {}
    },
    "MYMPD_API_PLAYER_STOP": {
        "desc": "Stops playing.",
        "params": {}
    },
    "MYMPD_API_PLAYER_SEEK_CURRENT": {
        "desc": "Seeks the current playing song.",
        "params": {
            "seek": {
                "type": "int",
                "example": 5,
                "desc": "seconds to seek"
            },
            "relative": {
                "type": "bool",
                "example": "true",
                "desc": "true = relative seek, false = goto seek seconds in song"
            }
        }
    },
    "MYMPD_API_PLAYER_NEXT": {
        "desc": "Goto next song in queue.",
        "params": {}
    },
    "MYMPD_API_PLAYER_PREV": {
        "desc": "Goto previous song in queue.",
        "params": {}
    },
    "MYMPD_API_PLAYER_OUTPUT_LIST": {
        "desc": "Lists the MPD outputs.",
        "params": {}
    },
    "MYMPD_API_PLAYER_TOGGLE_OUTPUT": {
        "desc": "Toggles the output state.",
        "params": {
            "outputId": {
                "type": "uint",
                "example": 0,
                "desc": "MPD output id"
            },
            "state": {
                "type": "uint",
                "example": 0,
                "desc": "0 = disable, 1 = enable"
            }
        }
    },
    "MYMPD_API_PLAYER_CURRENT_SONG": {
        "desc": "Shows details of current playing song.",
        "params": {}
    },
    "MYMPD_API_PLAYER_STATE": {
        "desc": "Shows the mpd player state.",
        "params": {}
    },
    "MYMPD_API_LIKE": {
        "desc": "Sets the like status of a song.",
        "params": {
            "uri": params.uri,
            "like": {
                "type": "uint",
                "example": 1,
                "desc": "0 = dislike, 1 = neutral, 2 = like"
            }
        }
    },
    "MYMPD_API_MOUNT_LIST": {
        "desc": "Lists the MPD monts.",
        "params": {}
    },
    "MYMPD_API_MOUNT_NEIGHBOR_LIST": {
        "desc": "Lists the neighbors.",
        "params": {}
    },
    "MYMPD_API_MOUNT_MOUNT": {
        "desc": "Mounts a network path.",
        "params": {
            "mountUrl": {
                "type": "text",
                "example": "nfs://192.168.1.1/music",
                "desc": "URL to mount."
            }, 
            "mountPoint": {
                "type": "text",
                "example": "nas",
                "desc": "Path to mount the URL"
            }
        }
    },
    "MYMPD_API_MOUNT_UNMOUNT": {
        "desc": "Unmounts a mounted network path.",
        "params": {
            "mountPoint": {
                "type": "text",
                "example": "nas",
                "desc": "Path to unmount"
            }
        }
    },
    "MYMPD_API_URLHANDLERS": {
        "desc": "Lists all known url handlers of MPD.",
        "params": {}
    },
    "MYMPD_API_CONNECTION_SAVE": {
        "desc": "Saves the MPD connection parameters.",
        "params": {
            "mpdHost": {
                "type": "text",
                "example": "/run/mpd/socket",
                "desc": "MPD host or socket"
            },
            "mpdPort": {
                "type": "uint",
                "example": "6000",
                "desc": "MPD port to use"
            },
            "musicDirectory": {
                "type": "text",
                "example": "auto",
                "desc": "\"auto\" = autodetect (needs socket connection), " +
                        "\"none\" = no music directory, " +
                        "or absolute path of music directory"
            },
            "playlistDirectory": {
                "type": "text",
                "example": "/var/lib/mpd/playlists",
                "desc": "absolut path of playlist directory"
            },
            "mpdStreamPort": {
                "type": "uint",
                "example": 8000,
                "desc": "port of mpd http stream for local playback"
            },
            "mpdBinarylimit": {
                "type": "uint",
                "example": 8192,
                "desc": "chunk size in bytes for binary data"
            },
            "mpdTimeout": {
                "type": "uint",
                "example": 10000,
                "desc": "MPD timeout in ms"
            }
        }
    },
    "MYMPD_API_SETTINGS_GET": {
        "desc": "Gets all myMPD and MPD settings.",
        "params": {}
    },
    "MYMPD_API_SETTINGS_SET": {
        "desc": "Sets myMPD settings.",
        "params": {
            "coverimageNames": {
                "type": "text",
                "example": "folder,cover",
                "desc": "Comma separated list of coverimages, basenames or full names"
            },
            "lastPlayedCount": {
                "type": "uint",
                "example": 200,
                "desc": "Length of the last played list"
            },
            "smartpls": {
                "type": "bool",
                "example": true,
                "desc": "Enabled the smart playlists feature"
            },
            "smartplsPrefix": {
                "type": "text",
                "example": "myMPDsmart",
                "desc": "Prefix for generated smart playlists"
            },
            "smartplsInterval": {
                "type": "uint",
                "example": 14400,
                "desc": "Interval for smart playlists generation in seconds"
            },
            "smartplsSort": {
                "type": "text",
                "example": "",
                "desc": "Sort settings for generated smart playlists, blank = no sort, \"shuffle\" or tag name" 
            },
            "smartplsGenerateTagList": {
                "type": "text",
                "example": "Genre",
                "desc": "Generates smart playlists per value of selected taglist"
            },
            "tagList": {
                "type": "text",
                "example": "Artist,Album,AlbumArtist,Title,Track,Genre,Disc",
                "desc": "Comma separated list of MPD tags to use"
            },
            "tagListSearch": {
                "type": "text",
                "example": "Artist,Album,AlbumArtist,Title,Genre",
                "desc": "Comma separated list of MPD tags for search"
            },
            "tagListBrowse": {
                "type": "text",
                "example": "Artist,Album,AlbumArtist,Genre",
                "desc": "Comma separated list of MPD tags to browse"
            },
            "bookletName": {
                "type": "text",
                "example": "booklet.pdf",
                "desc": "Name of booklet files"
            },
            "volumeMin": {
                "type": "uint",
                "example": 10,
                "desc": "Minimum volume"
            },
            "volumeMax": {
                "type": "uint",
                "example": 90,
                "desc": "Maximum volume"
            },
            "volumeStep": {
                "type": "uint",
                "example": 5,
                "desc": "Step for volume changes"
            },
            "lyricsUsltExt": {
                "type": "text",
                "example": "txt",
                "desc": "File extension for unsynced lyrics"
            },
            "lyricsSyltExt": {
                "type": "text",
                "example": "lrc",
                "desc": "File extension for synced lyrics"
            },
            "lyricsVorbisUslt": {
                "type": "text",
                "example": "LYRICS",
                "desc": "Vorbis tag for unsynced lyrics"
            },
            "lyricsVorbisSylt": {
                "type": "text",
                "example": "SYNCEDLYRICS",
                "desc": "Vorbis tag for synced lyrics"
            },
            "covercacheKeepDays": {
                "type": "uint",
                "example": 7,
                "desc": "Days before deleting cover cache files."
            },
            "webuiSettings": {
                "type": "object",
                "params": {
                    "clickSong": {
                        "type": "text", 
                        "example": "append",
                        "desc": "Action on click on song: append, replace, view"
                    },
                    "clickQueueSong": {
                        "type": "text",
                        "example": "play",
                        "desc": "Action on click on song in queue: play, view"
                    },
                    "clickPlaylist": {
                        "type": "text",
                        "example": "view",
                        "desc": "Action on click on playlist: append, replace, view"
                    },
                    "clickFolder": {
                        "type": "text",
                        "example": "view",
                        "desc": "Action on click on folder: append, replace, view"
                    },
                    "clickAlbumPlay": {
                        "type": "text",
                        "example": "replace",
                        "desc": "Action on click on album: append, replace"
                    },
                    "notificationPlayer": {
                        "type": "bool",
                        "example": false,
                        "desc": "Enable notifications for player events."
                    },
                    "notificationQueue": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable notifications for queue events."
                    },
                    "notificationGeneral": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable notifications for general events."
                    },
                    "notificationDatabase": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable notifications for database events."
                    },
                    "notificationPlaylist": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable notifications for playlist events."
                    },
                    "notificationScript": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable notifications for script events."
                    },
                    "notifyPage": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable on page notifications"
                    },
                    "notifyWeb": {
                        "type": "bool",
                        "example": false,
                        "desc": "Enable web notifications"
                    },
                    "mediaSession": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enable media session support"
                    },
                    "uiFooterQueueSettings": {
                        "type": "bool",
                        "example": true,
                        "desc": "Shows playback settings button in footer."
                    },
                    "uiFooterPlaybackControls": {
                        "type": "bool",
                        "example": "both",
                        "desc": "\"pause\", \"stop\" or \"both\" for pause and stop"
                    },
                    "uiMaxElementsPerPage": {
                        "type": "uint",
                        "example": 50,
                        "desc": "max. elements for lists: 25, 50, 100, 200 or 0 for unlimited"
                    },
                    "enableHome": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enables the home screen"
                    },
                    "enableScripting": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enables scripting"
                    },
                    "enableTrigger": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enables trigger"
                    },
                    "enableTimer": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enables timer"
                    },
                    "enableMounts": {
                        "type": "bool",
                        "example": true,
                        "desc": "Enables mounts"
                    },
                    "enableLocalPlayback": {
                        "type": "bool",
                        "example": false,
                        "desc": "Enables local playback of mpd http stream"
                    },
                    "enablePartitions": {
                        "type": "bool",
                        "example": false,
                        "desc": "Enables partitions"
                    },
                    "enableLyrics": {
                        "type": "text",
                        "example": true,
                        "desc": "Enable Lyrics"
                    },
                    "uiTheme": {
                        "type": "text",
                        "example": "theme-dark",
                        "desc": "\"theme-dark\", \"theme-light\" or \"theme-default\""
                    },
                    "uiHighlightColor": {
                        "type": "text",
                        "example": "#28a745",
                        "desc": "Highlight color"
                    },
                    "uiCoverimageSize": {
                        "type": "int",
                        "example": 250,
                        "desc": "Size for coverimages"
                    },
                    "uiCoverimageSizeSmall": {
                        "type": "int",
                        "example": 175,
                        "desc": "Size for small cover images"
                    },
                    "uiBgColor": {
                        "type": "text",
                        "example": "#000000",
                        "desc": "Background color"
                    },
                    "uiBgImage": {
                        "type": "text",
                        "example": "",
                        "desc": "Uri for bacckground image"
                    },
                    "uiBgCover": {
                        "type": "bool",
                        "example": true,
                        "desc": "Display the coverimage as background"
                    },
                    "uiBgCssFilter": {
                        "type": "text",
                        "example": "grayscale(100%) opacity(10%)",
                        "desc": "CSS filter for background coverimage"
                    },
                    "uiLocale": {
                        "type": "text",
                        "example": "de-DE",
                        "desc": "Language code or \"auto\" for brwoser default"
                    }
                }
            }
        }
    },
    "MYMPD_API_PLAYER_OPTIONS_SET": {
        "desc": "Sets MPD and jukebox options.",
        "params":{
            "consume": {
                "type": "uint",
                "example": 1,
                "desc": "MPD consume mode: 1=enabled, 0=disabled"
            },
            "random": {
                "type": "uint",
                "example": 0,
                "desc": "MPD randome mode: 1=enabled, 0=disabled"
            },
            "single": {
                "type": "uint",
                "example": 1,
                "desc": "MPD single mode: 2=single oneshot, 1=enabled, 0=disabled"
            },
            "repeat": {
                "type": "uint",
                "example": 1,
                "desc": "MPD repeat mode: 1=enabled, 0=disabled"
            },
            "replaygain": {
                "type": "text",
                "example": "off",
                "desc": "MPD replaygain mode: \"off\", \"auto\", \"track\", \"album\""
            },
            "crossfade": {
                "type": "utin",
                "example": 0,
                "desc": "MPD crossfade in seconds"
            },
            "jukeboxMode": {
                "type": "uint",
                "example": 1,
                "desc": "Jukebox mode: 0=disabled, 1=song, 2=album"
            },
            "jukeboxPlaylist": {
                "type": "text",
                "example": "Database",
                "desc": "Playlist for jukebox or \"Databas\" for whole database."
            },
            "jukeboxQueueLength": {
                "type": "uint",
                "example": 1,
                "desc": "Minimum queue length to maintain."
            },
            "jukeboxLastPlayed": {
                "type": "uint",
                "example": 24,
                "desc": "Add only songs that are not played x hours before."
            },
            "jukeboxUniqueTag": {
                "type": "text",
                "example": "Album",
                "desc": "Tag to maintain unique values in internal jukebox queue."
            },
            "autoPlay": {
                "type": "bool",
                "example": false,
                "desc": "Start playing if a song is adder to queue."
            }
        }
    },
    {"MYMPD_API_COLS_SAVE","params":{"table":"","cols":["Artist","Album","Title"]}},
    {"MYMPD_API_TIMER_SAVE","params":{"timerid":0,"interval":0,"name":"","enabled":false,"startHour":0,"startMinute":0,"action":"","subaction":"","volume":0,"playlist":"","jukeboxMode":0,"weekdays":[false,false,false,false,false,false,false],"arguments":{"arg1":""}}},
    {"MYMPD_API_TIMER_LIST"},
    {"MYMPD_API_TIMER_GET","params":{"timerid":0}},
    {"MYMPD_API_TIMER_RM","params":{"timerid":0}},
    {"MYMPD_API_TIMER_TOGGLE","params":{"timerid":0}},
    {"MYMPD_API_MESSAGE_SEND","params":{"channel":"", "message":""}},
    {"MYMPD_API_SCRIPT_SAVE","params":{"script":"","oldscript":"","order":0,"content":"","arguments":["", ""]}},
    {"MYMPD_API_SCRIPT_EXECUTE","params":{"script":"","arguments":{"arg1": ""}}},
    {"MYMPD_API_SCRIPT_POST_EXECUTE","params":{"script":"","arguments":{"arg1": ""}}},
    {"MYMPD_API_SCRIPT_LIST","params":{"all":true}},
    {"MYMPD_API_SCRIPT_GET","params":{"script":""}},
    {"MYMPD_API_SCRIPT_DELETE","params":{"script":""}},
    {"MYMPD_API_PARTITION_LIST","params":{}},
    {"MYMPD_API_PARTITION_NEW","params":{"name":""}},
    {"MYMPD_API_PARTITION_SWITCH","params":{"name":""}},
    {"MYMPD_API_PARTITION_RM","params":{"name":""}},
    {"MYMPD_API_PARTITION_OUTPUT_MOVE","params":{"name":""}},
    {"MYMPD_API_TRIGGER_LIST","params":{}},
    {"MYMPD_API_TRIGGER_GET","params":{"id":0}},
    {"MYMPD_API_TRIGGER_SAVE","params":{"id":0,"name":"","event":0,"script":""}},
    {"MYMPD_API_TRIGGER_DELETE","params":{"id":0}},
    {"MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET","params":{"outputId":0,"attributes":{"allowed_formats":""}}},
    {"MYMPD_API_HOME_LIST","params":{}},
    {"MYMPD_API_HOME_ICON_DELETE","params":{"pos":0}},
    {"MYMPD_API_HOME_ICON_MOVE","params":{"from":0,"to":0}},
    {"MYMPD_API_HOME_ICON_SAVE","params":{"replace":false,"oldpos":0,"name":"","ligature":"","bgcolor":"","image":"","cmd":"","options":["option1","option2"]}},
    {"MYMPD_API_PICTURE_LIST","params":{}},
    {"MYMPD_API_JUKEBOX_LIST","params":{"offset":"0","limit":100,"cols":["Pos","Title","Artist","Album"]}},
    {"MYMPD_API_JUKEBOX_RM","params":{"pos":0}},
    {"MYMPD_API_LYRICS_GET","params":{"uri":""}}
};
