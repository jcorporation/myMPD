"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module clickActions_js */

/**
 * Handler for quick remove button
 * @param {EventTarget} target event target
 * @returns {void}
 */
function clickQuickRemove(target) {
    switch(app.id) {
        case 'QueueCurrent': {
            const songId = getData(target.parentNode.parentNode, 'songid');
            removeFromQueueIDs([songId]);
            break;
        }
        case 'BrowsePlaylistsDetail': {
            const pos = getData(target.parentNode.parentNode, 'songpos');
            const plist = getDataId('BrowsePlaylistsDetailList', 'uri');
            removeFromPlaylist('single', plist, pos);
            break;
        }
        case 'QueueJukebox': {
            const pos = getData(target.parentNode.parentNode, 'pos');
            delQueueJukeboxSong(pos);
            break;
        }
    }
}

/**
 * Handler for quick play button
 * @param {EventTarget} target event target
 * @returns {void}
 */
function clickQuickPlay(target) {
    const type = getData(target.parentNode.parentNode, 'type');
    let uri = getData(target.parentNode.parentNode, 'uri');
    if (type === 'webradio') {
        uri = getRadioFavoriteUri(uri);
    }
    switch(settings.webuiSettings.clickQuickPlay) {
        case 'append': return appendQueue(type, uri);
        case 'appendPlay': return appendPlayQueue(type, uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue(type, uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue(type, uri);
        case 'replace': return replaceQueue(type, uri);
        case 'replacePlay': return replacePlayQueue(type, uri);
    }
}

/**
 * Handler for album play button
 * @param {object} albumArtist album artists
 * @param {string} album album name
 * @returns {void}
 */
function clickAlbumPlay(albumArtist, album) {
    switch(settings.webuiSettings.clickQuickPlay) {
        case 'append': return _addAlbum('appendQueue', albumArtist, album, undefined);
        case 'appendPlay': return _addAlbum('appendPlayQueue', albumArtist, album, undefined);
        case 'insertAfterCurrent': return _addAlbum('insertAfterCurrentQueue', albumArtist, album, undefined);
        case 'insertPlayAfterCurrent': return _addAlbum('insertPlayAfterCurrentQueue', albumArtist, album, undefined);
        case 'replace': return _addAlbum('replaceQueue', albumArtist, album, undefined);
        case 'replacePlay': return _addAlbum('replacePlayQueue', albumArtist, album, undefined);
    }
}

/**
 * Click song handler
 * @param {string} uri song uri
 * @param {event} event the event
 * @returns {void}
 */
function clickSong(uri, event) {
    switch (settings.webuiSettings.clickSong) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
        case 'view': return songDetails(uri);
        case 'context': return showContextMenu(event);
    }
}

/**
 * Handler for radiobrowser links
 * @param {string} uri stream uri
 * @param {string} uuid radiobrowser station uuid
 * @param {event} event the event
 * @returns {void}
 */
function clickRadiobrowser(uri, uuid, event) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
        case 'view': return showRadiobrowserDetails(uuid);
        case 'context': return showContextMenu(event);
    }
    countClickRadiobrowser(uuid);
}

/**
 * Handler for webradioDB links
 * @param {string} uri stream uri
 * @param {event} event the event
 * @returns {void}
 */
function clickWebradiodb(uri, event) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
        case 'view': return showWebradiodbDetails(uri);
        case 'context': return showContextMenu(event);
    }
}

/**
 * Handler for webradio favorites links
 * @param {string} uri webradio favorite uri, starting with mympd://webradio/
 * @param {event} event the event
 * @returns {void}
 */
function clickRadioFavorites(uri, event) {
    const fullUri = getRadioFavoriteUri(uri);
    switch(settings.webuiSettings.clickRadioFavorites) {
        case 'append': return appendQueue('plist', fullUri);
        case 'appendPlay': return appendPlayQueue('plist', fullUri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', fullUri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', fullUri);
        case 'replace': return replaceQueue('plist', fullUri);
        case 'replacePlay': return replacePlayQueue('plist', fullUri);
        case 'edit': return editRadioFavorite(uri);
        case 'context': return showContextMenu(event);
    }
}

/**
 * Handler for song links in queue
 * @param {string} songid the song id
 * @param {string} uri the song uri
 * @param {event} event the event
 * @returns {void}
 */
function clickQueueSong(songid, uri, event) {
    switch(settings.webuiSettings.clickQueueSong) {
        case 'play':
            if (songid === null) {
                return;
            }
            sendAPI("MYMPD_API_PLAYER_PLAY_SONG", {
                "songId": songid
            }, null, false);
            break;
        case 'view':
            if (uri === null) {
                return;
            }
            return songDetails(uri);
        case 'context':
            return showContextMenu(event);
    }
}

/**
 * Handler for playlist links
 * @param {string} uri playlist uri
 * @param {event} event the event
 * @returns {void}
 */
function clickPlaylist(uri, event) {
    switch(settings.webuiSettings.clickPlaylist) {
        case 'append': return appendQueue('plist', uri);
        case 'appendPlay': return appendPlayQueue('plist', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', uri);
        case 'replace': return replaceQueue('plist', uri);
        case 'replacePlay': return replacePlayQueue('plist', uri);
        case 'view': return playlistDetails(uri);
        case 'context': return showContextMenu(event);
    }
}

/**
 * Handler for click on playlists in filesystem view
 * @param {string} uri playlist uri
 * @param {event} event the event
 * @returns {void}
 */
function clickFilesystemPlaylist(uri, event) {
    switch(settings.webuiSettings.clickFilesystemPlaylist) {
        case 'append': return appendQueue('plist', uri);
        case 'appendPlay': return appendPlayQueue('plist', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', uri);
        case 'replace': return replaceQueue('plist', uri);
        case 'replacePlay': return replacePlayQueue('plist', uri);
        case 'view':
            //remember offset for current browse uri
            browseFilesystemHistory[app.current.search] = {
                "offset": app.current.offset,
                "scrollPos": document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop
            };
            //reset filter and show playlist
            app.current.filter = '-';
            appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, app.current.filter, app.current.sort, 'plist', uri);
            break;
        case 'context': return showContextMenu(event);
    }
}

/**
 * Handler for click on folder in filesystem view
 * @param {string} uri folder uri
 * @returns {void}
 */
function clickFolder(uri) {
    //remember offset for current browse uri
    browseFilesystemHistory[app.current.search] = {
        "offset": app.current.offset,
        "scrollPos": getScrollPosY()
    };
    //reset filter and open folder
    appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, '-', app.current.sort, 'dir', uri);
}

/**
 * Seeks the current song forward by 5s
 * @returns {void}
 */
function seekRelativeForward() {
    seekRelative(5);
}

/**
 * Seeks the current song backward by 5s
 * @returns {void}
 */
function seekRelativeBackward() {
    seekRelative(-5);
}

/**
 * Seeks the current song by offset seconds
 * @param {number} offset relative seek offset
 * @returns {void}
 */
function seekRelative(offset) {
    sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
        "seek": offset,
        "relative": true
    }, null, false);
}

/**
 * Handler for click on play button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickPlay() {
    switch(currentState.state) {
        case 'play':
            if (settings.webuiSettings.uiFooterPlaybackControls === 'stop' ||
                isStreamUri(currentSongObj.uri) === true)
            {
                //always stop streams
                sendAPI("MYMPD_API_PLAYER_STOP", {}, null, false);
            }
            else {
                sendAPI("MYMPD_API_PLAYER_PAUSE", {}, null, false);
            }
            break;
        case 'pause':
            sendAPI("MYMPD_API_PLAYER_RESUME", {}, null, false);
            break;
        default:
            //fallback if playstate is stop or unknown
            sendAPI("MYMPD_API_PLAYER_PLAY", {}, null, false);
    }
}

/**
 * Handler for click on stop button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MYMPD_API_PLAYER_STOP", {}, null, false);
}

/**
 * Handler for click on prev song button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MYMPD_API_PLAYER_PREV", {}, null, false);
}

/**
 * Handler for click on next song button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MYMPD_API_PLAYER_NEXT", {}, null, false);
}

/**
 * Handler for click on single button
 * @param {string} mode single mode: "0" = off, "1" = single, "oneshot" = single one shot
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickSingle(mode) {
    sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
        "single": mode
    }, null, false);
}
