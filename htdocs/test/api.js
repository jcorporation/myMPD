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
            "filter": {
                "type": "text",
                "example": "Title",
                "desc": "tag to search"
            },
            "searchstr": params.searchstr,
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
                "desc": "Root directory for update"
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
    {"MYMPD_API_QUEUE_CLEAR"},
    {"MYMPD_API_QUEUE_CROP"},
    {"MYMPD_API_QUEUE_CROP_OR_CLEAR"},
    {"MYMPD_API_QUEUE_ADD_RANDOM","params":{"plist":"Database","quantity":20, "mode":1}},
    {"MYMPD_API_QUEUE_SAVE","params":{"plist":""}},
    {"MYMPD_API_QUEUE_LIST","params":{"offset":0,"limit":100,"cols":["Artist","Album","Title"]}},
    {"MYMPD_API_QUEUE_SEARCH","params":{"offset":0,"limit":100,"filter":"","searchstr":"","cols":["Artist","Album","Title"]}},
    {"MYMPD_API_QUEUE_RM_TRACK","params":{"trackId":0}},
    {"MYMPD_API_QUEUE_RM_RANGE","params":{"start":0,"end":0}},
    {"MYMPD_API_QUEUE_MOVE_TRACK","params":{"from":0,"to":0}},
    {"MYMPD_API_QUEUE_ADD_TRACK_AFTER","params":{"uri":"","to":0}},
    {"MYMPD_API_QUEUE_ADD_TRACK","params":{"uri":""}},
    {"MYMPD_API_QUEUE_ADD_PLAY_TRACK","params":{"uri":""}},
    {"MYMPD_API_QUEUE_REPLACE_TRACK","params":{"uri":""}},
    {"MYMPD_API_QUEUE_ADD_PLAYLIST","params":{"plist":""}},
    {"MYMPD_API_QUEUE_REPLACE_PLAYLIST","params":{"plist":""}},
    {"MYMPD_API_QUEUE_SHUFFLE"},
    {"MYMPD_API_QUEUE_LAST_PLAYED","params":{"offset":0,"limit":100,"cols":["Artist","Album","Title"]}},
    {"MYMPD_API_PLAYLIST_RM","params":{"plist":""}},
    {"MYMPD_API_PLAYLIST_CLEAR","params":{"plist":""}},
    {"MYMPD_API_PLAYLIST_RENAME","params":{"from":"","to":""}},
    {"MYMPD_API_PLAYLIST_MOVE_TRACK","params":{"plist":"","from":0,"to":0}},
    {"MYMPD_API_PLAYLIST_ADD_TRACK","params":{"plist":"","uri":""}},
    {"MYMPD_API_PLAYLIST_RM_TRACK","params":{"plist":"","track":0}},
    {"MYMPD_API_PLAYLIST_RM_ALL", "params":{"type":""}},
    {"MYMPD_API_PLAYLIST_LIST","params":{"offset":0,"limit":100,"searchstr":""}},
    {"MYMPD_API_PLAYLIST_CONTENT_LIST","params":{"plist":"","offset":0,"limit":100,"searchstr":"","cols":["Artist","Album","Title"]}},
    {"MYMPD_API_PLAYLIST_SHUFFLE", "params":{"plist":""}},
    {"MYMPD_API_PLAYLIST_SORT", "params":{"plist":"","tag":""}},
    {"MYMPD_API_SMARTPLS_UPDATE_ALL", "params":{"force":false}},
    {"MYMPD_API_SMARTPLS_UPDATE", "params":{"plist":""}},
    {"MYMPD_API_SMARTPLS_SAVE","params":{"type":"","plist":"","timerange":0,"sort":""}},
    {"MYMPD_API_SMARTPLS_GET","params":{"plist":""}},
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
