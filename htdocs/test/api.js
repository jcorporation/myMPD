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
            } 
            "mode": {
                "type": "uint",
                "example": 1
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
    "MYMPD_API_QUEUE_RM_TRACK": {
        "desc": "Removes the track from the queue.",
        "params": {
            "trackId": {
                "type": "uint",
                "example": 1,
                "desc": "MPD trackid to remove"
            }
        }
    },
    "MYMPD_API_QUEUE_RM_RANGE": {
        "desc": "Removes a track range from the queue."
        "params": {
            "start": {
                "type": "uint",
                "example": 0,
                "desc": "Start track position",
            },
            "end": {
                "type": "uint",
                "example": 1,
                "desc": "End track position"
            }
        }
    },
    "MYMPD_API_QUEUE_MOVE_TRACK": {
        "desc": "Moves a track in the queue."
        "params": {
            "from": params.from
            "to": params.to
        }
    },
    "MYMPD_API_QUEUE_ADD_TRACK_AFTER": {
        "desc": "Adds song(s) to distinct position in queue.",
        "params": {
            "uri": params.uri,
            "to": params.to
        }
    },
    "MYMPD_API_QUEUE_ADD_TRACK": {
        "desc": "Appends song(s) to the queue.",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_ADD_PLAY_TRACK": {
        "desc": "Appends song(s) to queue queue and plays it.",
        "params": {
            "uri": params.uri
        }
    },
    "MYMPD_API_QUEUE_REPLACE_TRACK": {
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
    "MYMPD_API_QUEUE_LAST_PLAYED": {
        "desc": "Lists the last played songs.",
        "params": {
            "offset": params.offset,
            "limit": params.limit,
            "cols": params.cols
        }
    },
    "MYMPD_API_PLAYLIST_RM": {
        "desc": "Removes the MPD playlist."
        "params": {
            "plist": params.plist
        }
    },
    "MYMPD_API_PLAYLIST_CLEAR": {
        "desc": "Clears the MPD playlist."
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
    "MYMPD_API_PLAYLIST_MOVE_TRACK": {
        "desc": "Moves a song in the playlist.",
        "params": {
            "plist": params.plist,
            "from": params.from,
            "to": params.to
        }
    },
    "MYMPD_API_PLAYLIST_ADD_TRACK": {
        "desc": "Appens a song to the playlist",
        "params": {
            "plist": params.plist,
            "uri": params.uri
        }
    },
    "MYMPD_API_PLAYLIST_RM_TRACK": {
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
        "desc": "Saves a smart playlist of type newest songs",
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
        "desc": "Saves a sticker search as a smart playlist",
        "params": {
            "plist": params.plist,
            "sticker": {
                "type": "text",
                "example": "like"
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
        "desc": "Saves a search expression as a smart playlist",
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
    {"MYMPD_API_PLAYER_PLAY_TRACK","params":{"track":0}},
    {"MYMPD_API_PLAYER_VOLUME_SET","params":{"volume":0}},
    {"MYMPD_API_PLAYER_VOLUME_GET"},
    {"MYMPD_API_PLAYER_PAUSE"},
    {"MYMPD_API_PLAYER_PLAY"},
    {"MYMPD_API_PLAYER_STOP"},
    {"MYMPD_API_PLAYER_SEEK_CURRENT","params":{"seek":0,"relative":false}},
    {"MYMPD_API_PLAYER_SEEK","params":{"songid":0,"seek":0}},
    {"MYMPD_API_PLAYER_NEXT"},
    {"MYMPD_API_PLAYER_PREV"},
    {"MYMPD_API_PLAYER_OUTPUT_LIST"},
    {"MYMPD_API_PLAYER_TOGGLE_OUTPUT","params":{"output":0,"state":0}},
    {"MYMPD_API_PLAYER_CURRENT_SONG"},
    {"MYMPD_API_PLAYER_STATE"},
    {"MYMPD_API_LIKE","params":{"uri":"","like":0}},
    {"MYMPD_API_SETTINGS_GET"},
    {"MYMPD_API_MOUNT_LIST"},
    {"MYMPD_API_MOUNT_NEIGHBOR_LIST"},
    {"MYMPD_API_MOUNT_MOUNT","params":{"mountUrl":"", "mountPoint":""}},
    {"MYMPD_API_MOUNT_UNMOUNT","params":{"mountPoint":""}},
    {"MYMPD_API_URLHANDLERS"},
    {"MYMPD_API_CONNECTION_SAVE","params":{"mpdHost":"/run/mpd/socket","mpdPort":6000,"musicDirectory":"auto","playlistDirectory":"/var/lib/mpd/playlists", "mpdStreamPort":8000, "mpdBinarylimit":8192, "mpdTimeout":10000}},
    {"MYMPD_API_SETTINGS_GET"},
    {"MYMPD_API_SETTINGS_SET","params":{"coverimageNames":"folder,cover","lastPlayedCount":"200","smartpls":true,"smartplsPrefix":"myMPDsmart","smartplsInterval":14400,"smartplsSort":"","taglist":"Artist,Album,AlbumArtist,Title,Track,Genre,Date","searchtaglist":"Album,AlbumArtist,Artist,Genre,Title","browsetaglist":"Album,AlbumArtist,Artist,Genre","generatePlsTags":"Genre","bookletName":"booklet.pdf"}},
    {"MYMPD_API_PLAYER_OPTIONS_SET","params":{"consume":1,"random":0,"single":0,"repeat":0,"replaygain":"off","crossfade":"0","jukeboxMode":1,"jukeboxPlaylist":"Database","jukeboxQueueLength":1,"jukeboxLastPlayed":24,"jukeboxUniqueTag":"Album","autoPlay":false}},
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
    {"MYMPD_API_LYRICS_GET","params":{"uri":""}},
    {"MYMPD_API_QUEUE_PRIO_SET_HIGHEST","params":{"trackid":0}}
};
