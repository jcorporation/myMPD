"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module apidoc_js */

/**
 * API parameter types
 * @type {object}
 */
const APItypes = {
    "array": "Array",
    "bool": "Boolean",
    "float": "Float",
    "int": "Integer",
    "object": "Object",
    "string": "String",
    "uint": "Unsigned integer"
};

/** 
 * API parameters
 * @type {object}
 */
const APIparams = {
    "offset": {
        "type": APItypes.uint,
        "example": 0,
        "desc": "Start offset of the returned list"
    },
    "limit": {
        "type": APItypes.uint,
        "example": 50,
        "desc": "Maximum number of elements to return"
    },
    "sort": {
        "type": APItypes.string,
        "example": "Title",
        "desc": "Tag to sort the result"
    },
    "sortdesc": {
        "type": APItypes.bool,
        "example": false,
        "desc": "false = ascending, true = descending sort"
    },
    "fields": {
        "type": APItypes.array,
        "example": "[\"Artist\", \"Album\", \"Title\"]",
        "desc": "Array of fields to return"
    },
    "expression": {
        "type": APItypes.string,
        "example": "((any contains 'ende'))",
        "desc": "MPD search expression"
    },
    "searchstr": {
        "type": APItypes.string,
        "example": "ende",
        "desc": "String to search"
    },
    "uri": {
        "type": APItypes.string,
        "example": "Alben/Einstürzende_Neubauten/Ende_Neu/01.Was_ist_ist.mp3",
        "desc": "Relativ song uri"
    },
    "streamUri": {
        "type": APItypes.string,
        "example": "https://liveradio.swr.de/sw282p3/swr1bw/play.mp3",
        "desc": "Stream uri"
    },
    "uris": {
        "type": APItypes.array,
        "example": "[\"Alben/Einstürzende_Neubauten/Ende_Neu/01.Was_ist_ist.mp3\"]",
        "desc": "Relativ song uris"
    },
    "filter": {
        "type": APItypes.string,
        "example": "Title",
        "desc": "Tag to search or \"any\" for all tags"
    },
    "from": {
        "type": APItypes.uint,
        "example": 2,
        "desc": "From position",
    },
    "to": {
        "type": APItypes.uint,
        "example": 1,
        "desc": "To position"
    },
    "whence": {
        "type": APItypes.uint,
        "example": 0,
        "desc": "How to interpret the to parameter: 0 = absolute, 1 = after, 2 = before current song"
    },
    "plist": {
        "type": APItypes.string,
        "example": "test_plist",
        "desc": "MPD playlist name"
    },
    "plists": {
        "type": APItypes.array,
        "example": "[\"test_plist\"]",
        "desc": "MPD playlist names"
    },
    "sortShuffle": {
        "type": APItypes.string,
        "example": "shuffle",
        "desc": "blank = no sorting, shuffle = shuffle, tagname = sort by tag"
    },
    "songId": {
        "type": APItypes.uint,
        "example": 1,
        "desc": "MPD queue song id"
    },
    "songIds": {
        "type": APItypes.array,
        "example": "[1,2]",
        "desc": "Array of MPD queue song ids"
    },
    "timerid": {
        "type": APItypes.uint,
        "example": 101,
        "desc": "Timer id, must be gt 100"
    },
    "script": {
        "type": APItypes.string,
        "example": "testscript",
        "desc": "Name of the script"
    },
    "scriptArguments": {
        "type": APItypes.object,
        "example": "{\"argname1\": \"argvalue1\"}",
        "desc": "Script arguments"
    },
    "triggerId": {
        "type": APItypes.uint,
        "example": 1,
        "desc": "Id of the trigger"
    },
    "partition": {
        "type": APItypes.string,
        "example": "default",
        "desc": "MPD partition"
    },
    "pos": {
        "type": APItypes.uint,
        "example": 2,
        "desc": "Position"
    },
    "positions": {
        "type": APItypes.array,
        "example": "[5,2]",
        "desc": "Positions, must be ordered descending"
    },
    "play": {
        "type": APItypes.bool,
        "example": true,
        "desc": "true = play first inserted song"
    },
    "radiobrowserUUID": {
        "type": APItypes.string,
        "example": "d8f01eea-26be-4e3d-871d-7596e3ab8fb1",
        "desc": "Station UUID from radio-browser.info"
    },
    "preset": {
        "type": APItypes.string,
        "example": "default",
        "desc": "Name of the preset"
    },
    "albumids": {
        "type": APItypes.array,
        "example": "[\"17515028-bd97-47f5-ba1c-38504141af82\"]",
        "desc": "myMPD album ids"
    },
    "albumid": {
        "type": APItypes.string,
        "example": "17515028-bd97-47f5-ba1c-38504141af82",
        "desc": "myMPD album id"
    },
    "disc": {
        "type": APItypes.string,
        "example": "1",
        "desc": "album disc"
    },
    "maxentries": {
        "type": APItypes.uint,
        "example": 200,
        "desc": "Maximum entries"
    },
    "tagValues": {
        "type": APItypes.object,
        "example": "{\"Title\": \"title\", \"Artist\": \"artist\"}",
        "desc": "MPD tag name as key and its value."
    },
    "channel": {
        "type": APItypes.string,
        "example": "mpdscribble",
        "desc": "MPD channel name"
    }
};

/**
 * API methods
 * @type {object}
 */
const APImethods = {
    "MYMPD_API_CACHES_CREATE": {
        "desc": "Recreates the myMPD caches for albums and stickers.",
        "params": {
            "force": {
                "type": APItypes.bool,
                "example": false,
                "desc": "true = forces an update"
            }
        }
    },
    "MYMPD_API_DATABASE_SEARCH": {
        "desc": "Searches for songs in the database.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_DATABASE_UPDATE": {
        "desc": "Updates the database.",
        "params": {
            "uri": {
                "type": APItypes.string,
                "example": "Testfiles",
                "desc": "Root directory for update"
            }
        }
    },
    "MYMPD_API_DATABASE_RESCAN": {
        "desc": "Rescans the database.",
        "params": {
            "uri": {
                "type": APItypes.string,
                "example": "Testfiles",
                "desc": "Root directory for rescan"
            }
        }
    },
    "MYMPD_API_DATABASE_FILESYSTEM_LIST": {
        "desc": "Lists directories, songs and playlists.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "searchstr": APIparams.searchstr,
            "path": {
                "type": APItypes.string,
                "example": "Alben",
                "desc": "Directory or playlist to list"
            },
            "type": {
                "type": APItypes.string,
                "example": "dir",
                "desc": "dir or plist"
            },
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_DATABASE_ALBUM_DETAIL": {
        "desc": "Displays songs of an album.",
        "params": {
            "albumid": APIparams.albumid,
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_DATABASE_ALBUM_LIST": {
        "desc": "Lists unique albums.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_DATABASE_TAG_LIST": {
        "desc": "Lists unique tag values.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "searchstr": APIparams.searchstr,
            "tag": {
                "type": APItypes.string,
                "example": "Genre",
                "desc": "Tag to display"
            },
            "sortdesc": APIparams.sortdesc
        }
    },
    "MYMPD_API_STATS": {
        "desc": "Shows MPD database statistics.",
        "params": {}
    },
    "MYMPD_API_SONG_DETAILS": {
        "desc": "Shows all details of a song.",
        "params": {
            "uri": APIparams.uri
        }
    },
    "MYMPD_API_SONG_COMMENTS": {
        "desc": "Shows comments of uri.",
        "params": {
            "uri": APIparams.uri
        }
    },
    "MYMPD_API_SONG_FINGERPRINT": {
        "desc": "Calculates the chromaprint fingerprint",
        "params": {
            "uri": APIparams.uri
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
        "async": true,
        "params": {
            "plist": {
                "type": APItypes.string,
                "example": "Database",
                "desc": "Name of mpd playlist or \"Database\""
            },
            "quantity": {
                "type": APItypes.uint,
                "example": 10,
                "desc": "Number of songs or albums to add"
            },
            "mode": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "1 = add songs, 2 = add albums"
            }
        }
    },
    "MYMPD_API_QUEUE_SAVE": {
        "desc": "Saves the queue as playlist.",
        "params": {
            "plist": {
                "type": APItypes.string,
                "example": "test_plist",
                "desc": "Playlist name"
            },
            "mode": {
                "type": APItypes.string,
                "example": "create",
                "desc": "Save mode: create, append, replace"
            }
        }
    },
    "MYMPD_API_QUEUE_SEARCH": {
        "desc": "Searches the queue.",
        "params": {
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_QUEUE_RM_IDS": {
        "desc": "Removes defined entries from the queue.",
        "params": {
            "songIds": APIparams.songIds
        }
    },
    "MYMPD_API_QUEUE_RM_RANGE": {
        "desc": "Removes a range from the queue.",
        "params": {
            "start": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Start queue position",
            },
            "end": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "End queue position, use -1 for open end"
            }
        }
    },
    "MYMPD_API_QUEUE_MOVE_POSITION": {
        "desc": "Moves entries in the queue.",
        "params": {
            "from": APIparams.from,
            "to": APIparams.to
        }
    },
    "MYMPD_API_QUEUE_MOVE_RELATIVE": {
        "desc": "Moves entries in the queue.",
        "params": {
            "songIds": APIparams.songIds,
            "to": APIparams.to,
            "whence": APIparams.whence
        }
    },
    "MYMPD_API_QUEUE_INSERT_PLAYLISTS": {
        "desc": "Adds the playlist to distinct position in the queue.",
        "params": {
            "plists": APIparams.plists,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_INSERT_URIS": {
        "desc": "Adds uris to distinct position in the queue.",
        "params": {
            "uris": APIparams.uris,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_INSERT_URI_TAGS": {
        "desc": "Adds an uri to distinct position in the queue and set tags.",
        "params": {
            "uri": APIparams.streamUri,
            "tags": APIparams.tagValues,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_INSERT_SEARCH": {
        "desc": "Adds the search result to distinct position in the queue.",
        "params": {
            "expression": APIparams.expression,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_INSERT_ALBUMS": {
        "desc": "Adds the albums to distinct position in the queue.",
        "params": {
            "albumids": APIparams.albumids,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_INSERT_ALBUM_DISC": {
        "desc": "Adds one discs from an album to distinct position in the queue.",
        "params": {
            "albumid": APIparams.albumid,
            "disc": APIparams.disc,
            "to": APIparams.to,
            "whence": APIparams.whence,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_PLAYLISTS": {
        "desc": "Appends the playlists to the queue.",
        "params": {
            "plists": APIparams.plists,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_URIS": {
        "desc": "Appends uris to the queue.",
        "params": {
            "uris": APIparams.uris,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_URI_TAGS": {
        "desc": "Appends an uri to the queue and set tags.",
        "params": {
            "uri": APIparams.streamUri,
            "tags": APIparams.tagValues,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_SEARCH": {
        "desc": "Appends the search result to the queue.",
        "params": {
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_ALBUMS": {
        "desc": "Appends the albums to the queue.",
        "params": {
            "albumids": APIparams.albumids,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_APPEND_ALBUM_DISC": {
        "desc": "Appends on disc from an album to the queue.",
        "params": {
            "albumid": APIparams.albumid,
            "disc": APIparams.disc,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_PLAYLISTS": {
        "desc": "Replaces the queue with the playlists.",
        "params": {
            "plists": APIparams.plists,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_URIS": {
        "desc": "Replaces the queue with uris.",
        "params": {
            "uris": APIparams.uris,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_URI_TAGS": {
        "desc": "Replaces the queue with uri and set tags.",
        "params": {
            "uri": APIparams.streamUri,
            "tags": APIparams.tagValues,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_SEARCH": {
        "desc": "Replaces the queue with search result.",
        "params": {
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_ALBUMS": {
        "desc": "Replaces the queue with albums.",
        "params": {
            "albumids": APIparams.albumids,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_REPLACE_ALBUM_DISC": {
        "desc": "Replaces the queue with one disc from an album.",
        "params": {
            "albumid": APIparams.albumid,
            "disc": APIparams.disc,
            "play": APIparams.play
        }
    },
    "MYMPD_API_QUEUE_SHUFFLE": {
        "desc": "Shuffles the queue.",
        "params": {}
    },
    "MYMPD_API_QUEUE_PRIO_SET": {
        "desc": "Set highest prio for specified song ids in the queue.",
        "params": {
            "songIds": APIparams.songIds,
            "priority": {
                "type": APItypes.uint,
                "example": 10,
                "desc": "Priority of song in queue, max is 255."
            }
        }
    },
    "MYMPD_API_QUEUE_PRIO_SET_HIGHEST": {
        "desc": "Set highest priority for specified song ids in the queue.",
        "params": {
            "songIds": APIparams.songIds
        }
    },
    "MYMPD_API_LAST_PLAYED_LIST": {
        "desc": "Lists the last played songs.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "fields": APIparams.fields,
            "expression": APIparams.expression
        }
    },
    "MYMPD_API_PLAYLIST_RM": {
        "desc": "Removes the MPD playlists.",
        "params": {
            "plists": APIparams.plists
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_CLEAR": {
        "desc": "Clears the MPD playlist.",
        "params": {
            "plist": APIparams.plist
        }
    },
    "MYMPD_API_PLAYLIST_RENAME": {
        "desc": "Renames the MPD playlist.",
        "params": {
            "plist": {
                "type": APItypes.string,
                "example": "test_plist",
                "desc": "MPD playlist to rename"
            },
            "newName": {
                "type": APItypes.string,
                "example": "test_plist_renamed",
                "desc": "New MPD playlist name"
            }
        }
    },
    "MYMPD_API_PLAYLIST_COPY": {
        "desc": "Copies or moves source playlists to a destination playlist.",
        "params": {
            "srcPlists": {
                "type": APItypes.array,
                "example": "[\"test_plist\"]",
                "desc": "Source MPD playlists to copy"
            },
            "dstPlist": {
                "type": APItypes.string,
                "example": "test_plist_to_copy",
                "desc": "Destination MPD playlist name"
            },
            "mode": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "0=copy-append, 1=copy-insert, 2=copy-replace, 3=move-append, 4=move-insert"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST": {
        "desc": "Moves entries from one playlist to another.",
        "params": {
            "srcPlist": {
                "type": APItypes.string,
                "example": "test_plist",
                "desc": "Source MPD playlists to copy songs positions from"
            },
            "dstPlist": {
                "type": APItypes.string,
                "example": "test_plist_to_move",
                "desc": "Destination MPD playlist name"
            },
            "positions": APIparams.positions,
            "mode": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "0=append, 1=insert"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION": {
        "desc": "Moves a song in the playlist.",
        "params": {
            "plist": APIparams.plist,
            "from": APIparams.from,
            "to": APIparams.to
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_APPEND_URIS": {
        "desc": "Appends uris to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "uris": APIparams.uris
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS": {
        "desc": "Appends albums to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "albumids": APIparams.albumids
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_DISC": {
        "desc": "Appends one disc from an album to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "albumid": APIparams.albumid,
            "disc": APIparams.disc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS": {
        "desc": "Inserts uris to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "uris": APIparams.uris,
            "to": APIparams.to
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS": {
        "desc": "Insert albums to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "albumids": APIparams.albumids,
            "to": APIparams.to
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_DISC": {
        "desc": "Inserts one disc from an album to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "albumid": APIparams.albumid,
            "disc": APIparams.disc,
            "to": APIparams.to
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS": {
        "desc": "Replaces the playlist content with uris.",
        "params": {
            "plist": APIparams.plist,
            "uris": APIparams.uris
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS": {
        "desc": "Replaces the playlist content with albums.",
        "params": {
            "plist": APIparams.plist,
            "albumids": APIparams.albumids
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_DISC": {
        "desc": "Replaces the playlist content with one disc from an album.",
        "params": {
            "plist": APIparams.plist,
            "albumid": APIparams.albumid,
            "disc": APIparams.disc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH": {
        "desc": "Inserts the search result into the playlist.",
        "params": {
            "plist": APIparams.plist,
            "expression": APIparams.expression,
            "to": APIparams.to,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH": {
        "desc": "Appends the search result to the playlist.",
        "params": {
            "plist": APIparams.plist,
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH": {
        "desc": "Replaces the playlist content with the search result",
        "params": {
            "plist": APIparams.plist,
            "expression": APIparams.expression,
            "sort": APIparams.sort,
            "sortdesc": APIparams.sortdesc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_RM_POSITIONS": {
        "desc": "Removes entries from the playlist. Positions must be sorted descending.",
        "params": {
            "plist": APIparams.plist,
            "positions": APIparams.positions
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_RM_RANGE": {
        "desc": "Removes a range from the playlist.",
        "params": {
            "plist": APIparams.plist,
            "start": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Start playlist position",
            },
            "end": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "End playlist position, use -1 for open end"
            }
        }
    },
    "MYMPD_API_PLAYLIST_RM_ALL": {
        "desc": "Batch removes playlists.",
        "protected": true,
        "params": {
            "plistType": {
                "type": APItypes.string,
                "example": "deleteEmptyPlaylists",
                "desc": "valid values are: \"deleteEmptyPlaylists\", \"deleteSmartPlaylists\", \"deleteAllPlaylists\""
            }
        }
    },
    "MYMPD_API_PLAYLIST_LIST": {
        "desc": "Lists all MPD playlists.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "searchstr": APIparams.searchstr,
            "type": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "0 = all playlists, 1 = static playlists, 2 = smart playlists"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_LIST": {
        "desc": "Lists songs in the playlist.",
        "params": {
            "plist": APIparams.plist,
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "expression": APIparams.expression,
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_ENUMERATE": {
        "desc": "Enumerates the playlist and returns the count and total length.",
        "params": {
            "plist": APIparams.plist
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_SHUFFLE": {
        "desc": "Shuffles the playlist.",
        "params": {
            "plist": APIparams.plist
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_SORT": {
        "desc": "Sorts the playlist.",
        "params": {
            "plist": APIparams.plist,
            "tag": {
                "type": APItypes.string,
                "example": "Artist",
                "desc": "Tag to sort"
            },
            "sortdesc": APIparams.sortdesc
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_DEDUP": {
        "desc": "Deduplicates the playlist.",
        "params": {
            "plist": APIparams.plist,
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove duplicate entries, false = count number of duplicate entries"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL": {
        "desc": "Deduplicates all playlists.",
        "params": {
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove duplicate entries, false = count number of duplicate entries"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_VALIDATE": {
        "desc": "Validates the playlist and removes invalid entries.",
        "params": {
            "plist": APIparams.plist,
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove invalid entries, false = count number of invalid entries"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL": {
        "desc": "Validates all playlist and removes invalid entries.",
        "params": {
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove invalid entries, false = count number of invalid entries"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP": {
        "desc": "Validates and deduplicates the playlist and removes invalid entries.",
        "params": {
            "plist": APIparams.plist,
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove invalid entries, false = count number of invalid entries"
            }
        }
    },
    "MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL": {
        "desc": "Validates and deduplicates all playlists and removes invalid entries.",
        "params": {
            "remove": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = remove invalid entries, false = count number of invalid entries"
            }
        }
    },
    "MYMPD_API_SMARTPLS_UPDATE_ALL": {
        "desc": "Updates all smart playlists.",
        "async": true,
        "params": {
            "force": {
                "type": APItypes.bool,
                "example": false,
                "desc": "true = forces an update"
            }
        }
    },
    "MYMPD_API_SMARTPLS_UPDATE": {
        "desc": "Updates the smart playlist.",
        "async": true,
        "params": {
            "plist": APIparams.plist
        }
    },
    "MYMPD_API_SMARTPLS_NEWEST_SAVE": {
        "desc": "Saves a smart playlist of type newest songs.",
        "params": {
            "plist": APIparams.plist,
            "timerange": {
                "type": APItypes.uint,
                "example": 604800,
                "desc": "timerange in seconds"
            },
            "sort": APIparams.sortShuffle,
            "sortdesc": APIparams.sortdesc,
            "maxentries": APIparams.maxentries
        }
    },
    "MYMPD_API_SMARTPLS_STICKER_SAVE": {
        "desc": "Saves a sticker search as a smart playlist.",
        "params": {
            "plist": APIparams.plist,
            "sticker": {
                "type": APItypes.string,
                "example": "like",
                "desc": "Sticker name"
            },
            "value": {
                "type": APItypes.string,
                "example": "2",
                "desc": "Sticker value"
            },
            "op": {
                "type": APItypes.string,
                "example": "=",
                "desc": "Compare operator: =, <, >"
            },
            "sort": APIparams.sortShuffle,
            "sortdesc": APIparams.sortdesc,
            "maxentries": APIparams.maxentries
        }
    },
    "MYMPD_API_SMARTPLS_SEARCH_SAVE": {
        "desc": "Saves a search expression as a smart playlist.",
        "params": {
            "plist": APIparams.plist,
            "expression": APIparams.expression,
            "sort": APIparams.sortShuffle,
            "sortdesc": APIparams.sortdesc,
            "maxentries": APIparams.maxentries
        }
    },
    "MYMPD_API_SMARTPLS_GET": {
        "desc": "Gets the smart playlist options.",
        "params": {
            "plist": APIparams.plist
        }
    },
    "MYMPD_API_PLAYER_PLAY_SONG": {
        "desc": "Starts playing the specified song.",
        "params": {
            "songId": APIparams.songId
        }
    },
    "MYMPD_API_PLAYER_VOLUME_CHANGE": {
        "desc": "Changes the volume.",
        "params": {
            "volume": {
                "type": APItypes.int,
                "example": 5,
                "desc": "Volume percent"
            }
        }
    },
    "MYMPD_API_PLAYER_VOLUME_SET": {
        "desc": "Sets the volume.",
        "params": {
            "volume": {
                "type": APItypes.uint,
                "example": 50,
                "desc": "Volume percent"
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
                "type": APItypes.int,
                "example": 5,
                "desc": "seconds to seek"
            },
            "relative": {
                "type": APItypes.bool,
                "example": true,
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
    "MYMPD_API_PLAYER_OUTPUT_GET": {
        "desc": "Get the details of a MPD output.",
        "params": {
            "outputName": {
                "type": APItypes.string,
                "example": "http",
                "desc": "MPD output name"
            }
        }
    },
    "MYMPD_API_PLAYER_OUTPUT_LIST": {
        "desc": "Lists the MPD outputs.",
        "params": {}
    },
    "MYMPD_API_PLAYER_OUTPUT_TOGGLE": {
        "desc": "Toggles the output state.",
        "params": {
            "outputId": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "MPD output id"
            },
            "enabled": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Enabled state"
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
    "MYMPD_API_PLAYER_CLEARERROR": {
        "desc": "Clears the current error message.",
        "params": {}
    },
    "MYMPD_API_LIKE": {
        "desc": "Sets the like status of a song.",
        "params": {
            "uri": APIparams.uri,
            "like": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "0 = dislike, 1 = neutral, 2 = like"
            }
        }
    },
    "MYMPD_API_RATING": {
        "desc": "Sets the stars rating of a song.",
        "params": {
            "uri": APIparams.uri,
            "rating": {
                "type": APItypes.uint,
                "example": 5,
                "desc": "0 - 10 stars"
            }
        }
    },
    "MYMPD_API_MOUNT_LIST": {
        "desc": "Lists the MPD mounts.",
        "params": {}
    },
    "MYMPD_API_MOUNT_NEIGHBOR_LIST": {
        "desc": "Lists the neighbors.",
        "params": {}
    },
    "MYMPD_API_MOUNT_MOUNT": {
        "desc": "Mounts a network path.",
        "protected": true,
        "params": {
            "mountUrl": {
                "type": APItypes.string,
                "example": "nfs://192.168.1.1/music",
                "desc": "URL to mount."
            },
            "mountPoint": {
                "type": APItypes.string,
                "example": "nas",
                "desc": "Path to mount the URL"
            }
        }
    },
    "MYMPD_API_MOUNT_UNMOUNT": {
        "desc": "Unmounts a mounted network path.",
        "protected": true,
        "params": {
            "mountPoint": {
                "type": APItypes.string,
                "example": "nas",
                "desc": "Path to unmount"
            }
        }
    },
    "MYMPD_API_MOUNT_URLHANDLER_LIST": {
        "desc": "Lists all known url handlers of MPD.",
        "params": {}
    },
    "MYMPD_API_CONNECTION_SAVE": {
        "desc": "Saves the MPD connection parameters.",
        "protected": true,
        "params": {
            "mpdHost": {
                "type": APItypes.string,
                "example": "/run/mpd/socket",
                "desc": "MPD host or socket"
            },
            "mpdPort": {
                "type": APItypes.uint,
                "example": 6600,
                "desc": "MPD port to use"
            },
            "mpdPass": {
                "type": APItypes.string,
                "example": "dontsetpassword",
                "desc": "MPD password to use, set it to 'dontsetpassword' to not change the password"
            },
            "mpdTimeout": {
                "type": APItypes.uint,
                "example": 120000,
                "desc": "MPD timeout in ms"
            },
            "mpdKeepalive": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Enables tcp keepalives"
            },
            "musicDirectory": {
                "type": APItypes.string,
                "example": "auto",
                "desc": "MPD music directory" +
                        "\"auto\" = autodetect (needs socket connection), " +
                        "\"none\" = no music directory, " +
                        "or absolute path of music directory"
            },
            "playlistDirectory": {
                "type": APItypes.string,
                "example": "auto",
                "desc": "MPD playlist directory" +
                        "\"auto\" = autodetect (needs socket connection), " +
                        "\"none\" = no playlist directory, " +
                        "or absolute path of playlist directory"
            },
            "mpdBinarylimit": {
                "type": APItypes.uint,
                "example": 8192,
                "desc": "chunk size in bytes for binary data"
            },
            "stickerdbMpdHost": {
                "type": APItypes.string,
                "example": "/run/mpd/socket",
                "desc": "MPD host or socket (sticker database)"
            },
            "stickerdbMpdPort": {
                "type": APItypes.uint,
                "example": 6600,
                "desc": "MPD port to use (sticker database)"
            },
            "stickerdbMpdPass": {
                "type": APItypes.string,
                "example": "dontsetpassword",
                "desc": "MPD password to use, set it to 'dontsetpassword' to not change the password (sticker database)"
            },
            "stickerdbMpdTimeout": {
                "type": APItypes.uint,
                "example": 120000,
                "desc": "MPD timeout in ms (sticker database)"
            },
            "stickerdbMpdKeepalive": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Enables tcp keepalives (sticker database)"
            }
        }
    },
    "MYMPD_API_SETTINGS_GET": {
        "desc": "Gets all myMPD and MPD settings.",
        "params": {}
    },
    "MYMPD_API_SETTINGS_SET": {
        "desc": "Sets myMPD settings.",
        "protected": true,
        "params": {
            "coverimageNames": {
                "type": APItypes.string,
                "example": "folder,cover",
                "desc": "Comma separated list of coverimages, basenames or full names."
            },
            "thumbnailNames": {
                "type": APItypes.string,
                "example": "folder-sm,cover-sm",
                "desc": "Comma separated list of coverimage thumbnails, basenames or full names."
            },
            "lastPlayedCount": {
                "type": APItypes.uint,
                "example": 200,
                "desc": "Length of the last played list"
            },
            "smartpls": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Enabled the smart playlists feature"
            },
            "smartplsPrefix": {
                "type": APItypes.string,
                "example": "myMPDsmart",
                "desc": "Prefix for generated smart playlists"
            },
            "smartplsInterval": {
                "type": APItypes.uint,
                "example": 14400,
                "desc": "Interval for smart playlists generation in seconds"
            },
            "smartplsSort": {
                "type": APItypes.string,
                "example": "",
                "desc": "Sort settings for generated smart playlists, blank = no sort, \"shuffle\" or tag name."
            },
            "smartplsGenerateTagList": {
                "type": APItypes.string,
                "example": "Genre",
                "desc": "Generates smart playlists per value of selected taglist"
            },
            "tagList": {
                "type": APItypes.string,
                "example": "Artist,Album,AlbumArtist,Title,Track,Genre,Disc",
                "desc": "Comma separated list of MPD tags to use"
            },
            "tagListSearch": {
                "type": APItypes.string,
                "example": "Artist,Album,AlbumArtist,Title,Genre",
                "desc": "Comma separated list of MPD tags for search"
            },
            "tagListBrowse": {
                "type": APItypes.string,
                "example": "Artist,Album,AlbumArtist,Genre",
                "desc": "Comma separated list of MPD tags to browse"
            },
            "bookletName": {
                "type": APItypes.string,
                "example": "booklet.pdf",
                "desc": "Name of booklet files"
            },
            "volumeMin": {
                "type": APItypes.uint,
                "example": 10,
                "desc": "Minimum volume"
            },
            "volumeMax": {
                "type": APItypes.uint,
                "example": 90,
                "desc": "Maximum volume"
            },
            "volumeStep": {
                "type": APItypes.uint,
                "example": 5,
                "desc": "Step for volume changes"
            },
            "lyricsUsltExt": {
                "type": APItypes.string,
                "example": "txt",
                "desc": "File extension for unsynced lyrics"
            },
            "lyricsSyltExt": {
                "type": APItypes.string,
                "example": "lrc",
                "desc": "File extension for synced lyrics"
            },
            "lyricsVorbisUslt": {
                "type": APItypes.string,
                "example": "LYRICS",
                "desc": "Vorbis tag for unsynced lyrics"
            },
            "lyricsVorbisSylt": {
                "type": APItypes.string,
                "example": "SYNCEDLYRICS",
                "desc": "Vorbis tag for synced lyrics"
            },
            "webuiSettings": {
                "params": {
                    "clickSong": {
                        "type": APItypes.string,
                        "example": "append",
                        "desc": "Action for click on song: append, appendPlay, replace, replacePlay, insertAfterCurrent, view"
                    },
                    "clickRadiobrowser": {
                        "type": APItypes.string,
                        "example": "view",
                        "desc": "Action for click on playlist: append, appendPlay, replace, replacePlay, insertAfterCurrent, add"
                    },
                    "clickRadioFavorites": {
                        "type": APItypes.string,
                        "example": "view",
                        "desc": "Action for click on playlist: append, appendPlay, replace, replacePlay, insertAfterCurrent, edit"
                    },
                    "clickQueueSong": {
                        "type": APItypes.string,
                        "example": "play",
                        "desc": "Action for click on song in queue: play, view"
                    },
                    "clickPlaylist": {
                        "type": APItypes.string,
                        "example": "view",
                        "desc": "Action for click on playlist: append, appendPlay, replace, replacePlay, insertAfterCurrent, view"
                    },
                    "clickFilesystemPlaylist": {
                        "type": APItypes.string,
                        "example": "view",
                        "desc": "Action for click on playlist in filesystem view: append, appendPlay, replace, replacePlay, insertAfterCurrent, view"
                    },
                    "clickQuickPlay": {
                        "type": APItypes.string,
                        "example": "replace",
                        "desc": "Action for click on quick play button: append, appendPlay, replace, replacePlay, insertAfterCurrent"
                    },
                    "notificationPlayer": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Enable notifications for player events."
                    },
                    "notificationQueue": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable notifications for queue events."
                    },
                    "notificationGeneral": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable notifications for general events."
                    },
                    "notificationDatabase": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable notifications for database events."
                    },
                    "notificationPlaylist": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable notifications for playlist events."
                    },
                    "notificationScript": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable notifications for script events."
                    },
                    "notifyPage": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable on page notifications"
                    },
                    "notifyWeb": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Enable web notifications"
                    },
                    "mediaSession": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enable media session support"
                    },
                    "footerPlaybackControls": {
                        "type": APItypes.string,
                        "example": "both",
                        "desc": "\"pause\", \"stop\" or \"both\" for pause and stop"
                    },
                    "footerSettingsPlayback": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Shows playback settings button in footer."
                    },
                    "footerVolumeLevel": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Displays the volume level in the footer."
                    },
                    "footerNotifications": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Displays a notification icon in the footer."
                    },
                    "showHelp": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Displays help texts."
                    },
                    "maxElementsPerPage": {
                        "type": APItypes.uint,
                        "example": 50,
                        "desc": "Max. elements for lists: 25, 50, 100, 200 or 0 for unlimited"
                    },
                    "smallWidthTagRows": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Display tags in rows for small displays."
                    },
                    "quickPlayButton": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Show quick play button"
                    },
                    "quickRemoveButton": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Show quick remove button"
                    },
                    "compactGrids": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Disables line-breaks in descriptions."
                    },
                    "showBackButton": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Shows a history back button in the navigation bar."
                    },
                    "enableHome": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enables the home screen"
                    },
                    "enableScripting": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enables scripting"
                    },
                    "enableTrigger": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enables trigger"
                    },
                    "enableTimer": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enables timer"
                    },
                    "enableMounts": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Enables mounts"
                    },
                    "enableLocalPlayback": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Enables local playback of mpd http stream."
                    },
                    "enablePartitions": {
                        "type": APItypes.bool,
                        "example": false,
                        "desc": "Enables partitions"
                    },
                    "enableLyrics": {
                        "type": APItypes.string,
                        "example": true,
                        "desc": "Enable Lyrics"
                    },
                    "theme": {
                        "type": APItypes.string,
                        "example": "dark",
                        "desc": "\"dark\", \"light\" or \"auto\""
                    },
                    "gridSize": {
                        "type": APItypes.int,
                        "example": 175,
                        "desc": "Width for grids."
                    },
                    "bgCover": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Display the coverimage as background."
                    },
                    "bgCssFilter": {
                        "type": APItypes.string,
                        "example": "grayscale(100%) opacity(10%)",
                        "desc": "CSS filter for background coverimage."
                    },
                    "bgColor": {
                        "type": APItypes.string,
                        "example": "#000000",
                        "desc": "Background color"
                    },
                    "bgImage": {
                        "type": APItypes.string,
                        "example": "",
                        "desc": "Uri for background image"
                    },
                    "locale": {
                        "type": APItypes.string,
                        "example": "de-DE",
                        "desc": "Language code or \"auto\" for browser default."
                    },
                    "startupView": {
                        "type": APItypes.string,
                        "example": "Home",
                        "desc": "Startup view"
                    },
                    "musicbrainzLinks": {
                        "type": APItypes.bool,
                        "example": true,
                        "desc": "Shows links to MusicBrainz website in the playback and album views."
                    },
                    "browseDatabaseAlbumListSort": {
                        "type": APItypes.string,
                        "example": "Added",
                        "desc": "Default sort tag for the album list."
                    }
                }
            }
        }
    },
    "MYMPD_API_PLAYER_OPTIONS_SET": {
        "desc": "Sets MPD and jukebox options.",
        "params": {
            "name": APIparams.preset,
            "consume": {
                "type": APItypes.string,
                "example": "1",
                "desc": "MPD consume mode: \"0\", \"1\", \"oneshot\""
            },
            "random": {
                "type": APItypes.bool,
                "example": false,
                "desc": "MPD random mode."
            },
            "single": {
                "type": APItypes.string,
                "example": "1",
                "desc": "MPD single mode: \"0\", \"1\", \"oneshot\""
            },
            "repeat": {
                "type": APItypes.bool,
                "example": false,
                "desc": "MPD repeat mode."
            },
            "replaygain": {
                "type": APItypes.string,
                "example": "off",
                "desc": "MPD replaygain mode: \"off\", \"auto\", \"track\", \"album\""
            },
            "crossfade": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "MPD crossfade in seconds"
            },
            "mixrampDb": {
                "type": APItypes.float,
                "example": 0,
                "desc": "Mixramp threshold in dB"
            },
            "mixrampDelay": {
                "type": APItypes.float,
                "example": 0,
                "desc": "Mixrampdelay in seconds"
            },
            "jukeboxMode": {
                "type": APItypes.string,
                "example": "off",
                "desc": "Jukebox modes: \"off\", \"song\", \"album\""
            },
            "jukeboxPlaylist": {
                "type": APItypes.string,
                "example": "Database",
                "desc": "Playlist for jukebox or \"Database\" for whole database."
            },
            "jukeboxQueueLength": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "Minimum queue length to maintain."
            },
            "jukeboxLastPlayed": {
                "type": APItypes.uint,
                "example": 24,
                "desc": "Add only songs that are not played x hours before."
            },
            "jukeboxUniqueTag": {
                "type": APItypes.string,
                "example": "Album",
                "desc": "Tag for which unique values are enforced in the queue."
            },
            "jukeboxIgnoreHated": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Ignores hated songs."
            },
            "jukeboxFilterInclude": APIparams.expression,
            "jukeboxFilterExclude": APIparams.expression,
            "jukeboxMinSongDuration": {
                "type": APItypes.uint,
                "example": 60,
                "desc": "Only songs with this minimum length will be considered."
            },
            "autoPlay": {
                "type": APItypes.bool,
                "example": false,
                "desc": "Start playing if a song is added to the queue."
            }
        }
    },
    "MYMPD_API_PRESET_RM": {
        "desc": "Deletes a preset.",
        "params": {
            "name": APIparams.preset
        }
    },
    "MYMPD_API_PRESET_APPLY": {
        "desc": "Applies a preset.",
        "params": {
            "name": APIparams.preset
        }
    },
    "MYMPD_API_VIEW_SAVE": {
        "desc": "Saves options for a view.",
        "params": {
            "view": {
                "type": APItypes.string,
                "example": "viewQueueCurrent",
                "desc": "Valid values: viewQueueCurrent, viewQueueLastPlayed, viewSearch, viewBrowseDatabaseAlbumDetail, viewBrowseDatabaseAlbumList, viewBrowsePlaylistDetail, viewBrowseFilesystem, viewPlayback, viewQueueJukeboxAlbum, viewQueueJukeboxSong, viewBrowseRadioWebradiodb, viewBrowseRadioRadiobrowser"
            },
            "mode": {
                "type": APItypes.string,
                "example": "table",
                "desc": "View mode: table or grid"
            },
            "fields": APIparams.fields
        }
    },
    "MYMPD_API_TIMER_SAVE": {
        "desc": "Saves a timer.",
        "protected": true,
        "params": {
            "timerid": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Timer id, 0 to create a new timer."
            },
            "name": {
                "type": APItypes.string,
                "example": "example timer",
                "desc": "Name of the timer"
            },
            "interval": {
                "type": APItypes.int,
                "example": 86400,
                "desc": "Timer interval in seconds, 0 = one shote and deactivate, -1 = one shot and remove"
            },
            "enabled": {
                "type": APItypes.bool,
                "example": true,
                "desc": "Enables or disables the timer"
            },
            "startHour": {
                "type": APItypes.uint,
                "example": 7,
                "desc": "Start hour of the timer, valid values are 0-23"
            },
            "startMinute": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Start minute of the timer, valid values are 0-59"
            },
            "weekdays": {
                "type": APItypes.array,
                "example": "[false,false,false,false,false,true,true]",
                "desc": "Boolean array for weekdays, starting at monday"
            },
            "action": {
                "type": APItypes.string,
                "example": "player",
                "desc": "Timer action, valid values: player, script"
            },
            "subaction": {
                "type": APItypes.string,
                "example": "startplay",
                "desc": "Action = player: startplay, stopplay; Action = script: Script name"
            },
            "volume": {
                "type": APItypes.uint,
                "example": 50,
                "desc": "Volume in percent"
            },
            "playlist": {
                "type": APItypes.string,
                "example": "Database",
                "desc": "Playlist to use, valid values: \"Database\" or MPD playlist name"
            },
            "preset": APIparams.preset,
            "arguments": {
                "type": APItypes.object,
                "example": "{\"arg1\": \"value1\"}",
                "desc": "Script arguments"
            }
        }
    },
    "MYMPD_API_TIMER_LIST": {
        "desc": "Lists all timers"
    },
    "MYMPD_API_TIMER_GET": {
        "desc": "Gets options from a timer",
        "params": {
            "timerid": APIparams.timerid
        }
    },
    "MYMPD_API_TIMER_RM": {
        "desc": "Removes a timer",
        "protected": true,
        "params": {
            "timerid": APIparams.timerid
        }
    },
    "MYMPD_API_TIMER_TOGGLE": {
        "desc": "Toggles a timers enabled state",
        "protected": true,
        "params": {
            "timerid": APIparams.timerid
        }
    },
    "MYMPD_API_CHANNEL_LIST": {
        "desc": "Lists all channels",
        "params": {}
    },
    "MYMPD_API_CHANNEL_SUBSCRIBE": {
        "desc": "Subscribes a channel",
        "params": {
            "channel": APIparams.channel
        }
    },
    "MYMPD_API_CHANNEL_UNSUBSCRIBE": {
        "desc": "Unsubscribes a channel",
        "params": {
            "channel": APIparams.channel
        }
    },
    "MYMPD_API_CHANNEL_MESSAGE_SEND": {
        "desc": "Sends a message to a MPD channel",
        "params": {
            "channel": APIparams.channel,
            "message": {
                "type": APItypes.string,
                "example": "love",
                "desc": "Message to send"
            }
        }
    },
    "MYMPD_API_CHANNEL_MESSAGES_READ": {
        "desc": "Receives all messages from all subscribed channels.",
        "params": {}
    },
    "MYMPD_API_SCRIPT_VALIDATE": {
        "desc": "Validates (precompiles) a script",
        "params": {
            "script": APIparams.script,
            "content": {
                "type": APItypes.string,
                "example": "return \"test\"",
                "desc": "The lua script itself"
            }
        }
    },
    "MYMPD_API_SCRIPT_SAVE": {
        "desc": "Saves a script",
        "protected": true,
        "params": {
            "script": APIparams.script,
            "oldscript": {
                "type": APItypes.string,
                "example": "testscript",
                "desc": "Name of the old script to rename"
            },
            "order": {
                "type": APItypes.uint,
                "example": 1,
                "desc": "Order for the scripts in main menu, 0 = disable listing in main menu"
            },
            "content": {
                "type": APItypes.string,
                "example": "return \"test\"",
                "desc": "The lua script itself"
            },
            "arguments": {
                "type": APItypes.array,
                "example": "[\"argname1\",\"argname2\"]",
                "desc": "Array of parameters for this script"
            }
        }
    },
    "MYMPD_API_SCRIPT_LIST": {
        "desc": "Lists all scripts",
        "params": {
            "all": {
                "type": APItypes.bool,
                "example": true,
                "desc": "true = lists all scripts, false = lists all scripts with order > 0"
            }
        }
    },
    "MYMPD_API_SCRIPT_GET": {
        "desc": "Gets options from a timer",
        "params": {
            "script": APIparams.script
        }
    },
    "MYMPD_API_SCRIPT_RM": {
        "desc": "Removes a script",
        "protected": true,
        "params": {
            "script": APIparams.script
        }
    },
    "MYMPD_API_SCRIPT_EXECUTE": {
        "desc": "Executes a script",
        "params": {
            "script": APIparams.script,
            "event": {
                "type": APItypes.string,
                "example": "user",
                "desc": "One of: extern, http, timer, trigger or user"
            },
            "arguments": APIparams.scriptArguments
        }
    },
    "MYMPD_API_SCRIPT_VAR_DELETE": {
        "desc": "Deletes a script variable",
        "protected": true,
        "params": {
            "key": {
                "type": APItypes.string,
                "example": "key1",
                "desc": "Variable name"
            }
        }
    },
    "MYMPD_API_SCRIPT_VAR_LIST": {
        "desc": "Lists all script variables",
        "protected": true,
        "params": {}
    },
    "MYMPD_API_SCRIPT_VAR_SET": {
        "desc": "Saves a script variable",
        "protected": true,
        "params": {
            "key": {
                "type": APItypes.string,
                "example": "key1",
                "desc": "Variable name"
            },
            "value": {
                "type": APItypes.string,
                "example": "value1",
                "desc": "Variable value"
            }
        }
    },
    "MYMPD_API_PARTITION_LIST": {
        "desc": "Lists all MPD partitions",
        "params": {}
    },
    "MYMPD_API_PARTITION_NEW": {
        "desc": "Creates a new MPD partition",
        "protected": true,
        "params": {
            "name": APIparams.partition
        }
    },
    "MYMPD_API_PARTITION_SAVE": {
        "desc": "Saves MPD partition settings",
        "protected": true,
        "params": {
            "highlightColor": {
                "type": APItypes.string,
                "example": "#28a745",
                "desc": "Highlight color for this partition"
            },
            "mpdStreamPort": {
                "type": APItypes.uint,
                "example": 8000,
                "desc": "Port of MPD http stream for local playback"
            },
            "streamUri": {
                "type": APItypes.string,
                "example": "http://custom/stream/uri",
                "desc": "Custom stream uri, overrides automatic stream uri calculation (MPD host + mpdStreamPort)"
            }
        }
    },
    "MYMPD_API_PARTITION_RM": {
        "desc": "Removes a mpd partition.",
        "protected": true,
        "params": {
            "name": APIparams.partition
        }
    },
    "MYMPD_API_PARTITION_OUTPUT_MOVE": {
        "desc": "Moves this output to current MPD partition",
        "protected": true,
        "params": {
            "outputs": {
                "type": APItypes.array,
                "example": "[\"output1\", \"output2\"]",
                "desc": "Outputs to move to current partition"
            }
        }
    },
    "MYMPD_API_TRIGGER_LIST": {
        "desc": "Lists all triggers"
    },
    "MYMPD_API_TRIGGER_GET": {
        "desc": "Get the options from a trigger",
        "params": {
            "id": APIparams.triggerId
        }
    },
    "MYMPD_API_TRIGGER_SAVE": {
        "desc": "Saves a trigger",
        "protected": true,
        "params": {
            "id": APIparams.triggerId,
            "name": {
                "type": APItypes.string,
                "example": "test trigger",
                "desc": "Name of the trigger"
            },
            "event": {
                "type": APItypes.int,
                "example": 1,
                "desc": "Event id that executes this triggers script"
            },
            "script": {
                "type": APItypes.string,
                "example": "test script",
                "desc": "Script to execute"
            },
            "partition": APIparams.partition,
            "arguments": APIparams.scriptArguments
        }
    },
    "MYMPD_API_TRIGGER_RM": {
        "desc": "Deletes a trigger",
        "protected": true,
        "params": {
            "id": APIparams.triggerId
        }
    },
    "MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET": {
        "desc": "Sets an MPD output attribute",
        "protected": true,
        "params": {
            "outputId": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "MPD output id"
            },
            "attributes": {
                "type" : APItypes.object,
                "example": "{\"allowed_formats\": \"\"}",
                "desc": "Key/value pairs to set attributes"
            }
        }
    },
    "MYMPD_API_HOME_ICON_LIST": {
        "desc": "Lists all home icons",
        "params": {}
    },
    "MYMPD_API_HOME_ICON_RM": {
        "desc": "Deletes a home icon",
        "params": {
            "pos": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Icon number to delete"
            }
        }
    },
    "MYMPD_API_HOME_ICON_MOVE": {
        "desc": "Move home icon position",
        "params": {
            "from": APIparams.from,
            "to": APIparams.to
        }
    },
    "MYMPD_API_HOME_ICON_GET": {
        "desc": "Gets details for a home icon",
        "params": {
            "pos": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Icon number to get"
            }
        }
    },
    "MYMPD_API_HOME_ICON_SAVE": {
        "desc": "Saves a home icon",
        "params": {
            "replace": {
                "type": APItypes.bool,
                "example": false,
                "desc": "Replace icon at pos oldpos"
            },
            "oldpos": {
                "type": APItypes.uint,
                "example": 0,
                "desc": "Position of home icon to replace"
            },
            "name": {
                "type": APItypes.string,
                "example": "test home icon",
                "desc": "Name of the home icon"
            },
            "ligature": {
                "type": APItypes.string,
                "example": "new_releases",
                "desc": "Ligature to use"
            },
            "bgcolor": {
                "type": APItypes.string,
                "example": "#ffee00",
                "desc": "Background color"
            },
            "color": {
                "type": APItypes.string,
                "example": "#ffee00",
                "desc": "Color for ligature"
            },
            "image": {
                "type": APItypes.string,
                "example": "home-icon-1.png",
                "desc": "relative path for an image (/browse/pics/ is the root)"
            },
            "cmd": {
                "type": APItypes.string,
                "example": "replaceQueue",
                "desc": "Valid values: replaceQueue = replace queue with a playlist, appGoto = goto a view, execScriptFromOptions = execute script"
            },
            "options": {
                "type": APItypes.array,
                "example": "[\"plist\",\"nas/Webradios/swr1.m3u\",\"swr1.m3u\"]",
                "desc": "Array of cmd options" +
                        "for replaceQueue: [\"plist\",\"nas/Webradios/swr1.m3u\",\"swr1.m3u\"], " +
                        "for appGoto: [\"Browse\",\"Database\",\"List\",\"0\",\"AlbumArtist\",\"-Last-Modified\",\"Album\",\"\"], "+
                        "for execScriptFromOptions: [\"Scriptname\",\"scriptarg1\"]"
            }
        }
    },
    "MYMPD_API_PICTURE_LIST": {
        "desc": "Lists all pictures in the /pics/<type> directory.",
        "params": {
            "type": {
                "type": APItypes.string,
                "example": "thumbs",
                "desc": "Subfolder of pics directory."
            }
        }
    },
    "MYMPD_API_JUKEBOX_LENGTH": {
        "desc": "Returns the length of the jukebox queue.",
        "params": {}
    },
    "MYMPD_API_JUKEBOX_APPEND_URIS": {
        "desc": "Appends songs the the jukebox queue. This is only allowed in jukebox script mode.",
        "params": {
            "uris": APIparams.uris
        }
    },
    "MYMPD_API_JUKEBOX_LIST": {
        "desc": "Lists the internal jukebox queue.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "fields": APIparams.fields,
            "expression": APIparams.expression
        }
    },
    "MYMPD_API_JUKEBOX_RM": {
        "desc": "Removes a song or album from the jukebox queue.",
        "params": {
            "positions": APIparams.positions
        }
    },
    "MYMPD_API_JUKEBOX_CLEAR": {
        "desc": "Clears the jukebox queue.",
        "params": {}
    },
    "MYMPD_API_JUKEBOX_CLEARERROR": {
        "desc": "Clears the jukebox error state.",
        "params": {}
    },
    "MYMPD_API_JUKEBOX_RESTART": {
        "desc": "Restarts the jukebox.",
        "params": {}
    },
    "MYMPD_API_LYRICS_GET": {
        "desc": "Gets all lyrics from uri.",
        "params": {
            "uri": APIparams.uri
        }
    },
    "MYMPD_API_SESSION_LOGIN": {
        "desc": "Get a session ticket with supplied pin.",
        "params": {
            "pin": {
                "type": APItypes.string,
                "example": "1234",
                "desc": "The myMPD settings pin, configured with mympd -p."
            }
        }
    },
    "MYMPD_API_SESSION_LOGOUT": {
        "desc": "Removes the session from the session table.",
        "protected": true,
        "params": {}
    },
    "MYMPD_API_SESSION_VALIDATE": {
        "desc": "Validates the session table.",
        "protected": true,
        "params": {}
    },
    "MYMPD_API_CACHE_DISK_CLEAR": {
        "desc": "Clears the caches on disk.",
        "params": {}
    },
    "MYMPD_API_CACHE_DISK_CROP": {
        "desc": "Crops the caches on disk.",
        "params": {}
    },
    "MYMPD_API_LOGLEVEL": {
        "desc": "Sets the loglevel.",
        "protected": true,
        "params": {
            "loglevel": {
                "type": APItypes.uint,
                "example": 7,
                "desc": "https://jcorporation.github.io/myMPD/configuration/logging"
            }
        }
    },
    "MYMPD_API_WEBRADIO_FAVORITE_LIST": {
        "desc": "Lists webradio favorites.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "searchstr": APIparams.searchstr
        }
    },
    "MYMPD_API_WEBRADIO_FAVORITE_SAVE": {
        "desc": "Lists saved webradios.",
        "params": {
            "name": {
                "type": APItypes.string,
                "example": "swr1",
                "desc": "Name of the webradio favorite to delete."
            },
            "streamUri": {
                "type": APItypes.string,
                "example": "https://liveradio.swr.de/sw282p3/swr1bw/play.mp3",
                "desc": "New URI of the webradio stream."
            },
            "streamUriOld": {
                "type": APItypes.string,
                "example": "https://liveradio.swr.de/sw282p3/swr1bw/play.mp3",
                "desc": "Old URI of the webradio stream."
            },
            "image": {
                "type": APItypes.string,
                "example": "http://www.swr.de/streampic.jpg",
                "desc": "Picture for the webradio."
            },
            "genre": {
                "type": APItypes.string,
                "example": "Pop Rock",
                "desc": "Genre or other tags."
            },
            "homepage": {
                "type": APItypes.string,
                "example": "http://swr1.de",
                "desc": "Webradio homepage"
            },
            "country": {
                "type": APItypes.string,
                "example": "Germany",
                "desc": "Country"
            },
            "state": {
                "type": APItypes.string,
                "example": "Bayern",
                "desc": "State or Region"
            },
            "language": {
                "type": APItypes.string,
                "example": "German",
                "desc": "Language"
            },
            "description": {
                "type": APItypes.string,
                "example": "Short description",
                "desc": "Short description"
            },
            "codec": {
                "type": APItypes.string,
                "example": "MP3",
                "desc": "Codec of the stream."
            },
            "bitrate": {
                "type": APItypes.uint,
                "example": 128,
                "desc": "Bitrate of the stream in kbit."
            }
        }
    },
    "MYMPD_API_WEBRADIO_FAVORITE_GET": {
        "desc": "Gets a webradio favorite.",
        "params": {
            "filename": {
                "type": APItypes.string,
                "example": "https___liveradio_swr_de_sw282p3_swr1bw_play_mp3.m3u",
                "desc": "Name of the webradio favorite to get."
            }
        }
    },
    "MYMPD_API_WEBRADIO_FAVORITE_RM": {
        "desc": "Deletes webradio favorites.",
        "params": {
            "filenames": {
                "type": APItypes.array,
                "example": "[\"https___liveradio_swr_de_sw282p3_swr1bw_play_mp3.m3u\"]",
                "desc": "Filenames of the webradio favorites to delete."
            }
        }
    },
    "MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT": {
        "desc": "Returns radio-browser.info station details.",
        "params": {
            "uuid": APIparams.radiobrowserUUID
        }
    },
    "MYMPD_API_CLOUD_RADIOBROWSER_NEWEST": {
        "desc": "Lists the last changed/added stations.",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit
        }
    },
    "MYMPD_API_CLOUD_RADIOBROWSER_SEARCH": {
        "desc": "Searches radio-browser.info",
        "params": {
            "offset": APIparams.offset,
            "limit": APIparams.limit,
            "tags": {
                "type": APItypes.string,
                "example": "pop",
                "desc": "Tag to filter"
            },
            "country": {
                "type": APItypes.string,
                "example": "Germany",
                "desc": "Country to filter"
            },
            "language": {
                "type": APItypes.string,
                "example": "German",
                "desc": "Language to filter"
            },
            "searchstr": APIparams.searchstr
        }
    },
    "MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST": {
        "desc": "Returns radio-browser.info endpoints.",
        "params": {}
    },
    "MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL": {
        "desc": "Returns radio-browser.info station details.",
        "params": {
            "uuid": APIparams.radiobrowserUUID
        }
    },
    "MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET": {
        "desc": "Gets the full WebradioDB.",
        "params": {}
    }
};
