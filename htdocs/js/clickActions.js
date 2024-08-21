"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module clickActions_js */

/**
 * Handler for quick remove button
 * @param {EventTarget} target event target
 * @returns {void}
 */
function clickQuickRemove(target) {
    let dataNode = target.parentNode.parentNode;
    if (dataNode.classList.contains('row')) {
        dataNode = dataNode.parentNode;
    }
    switch(app.id) {
        case 'QueueCurrent': {
            const songId = getData(dataNode, 'songid');
            removeFromQueueIDs([songId]);
            break;
        }
        case 'BrowsePlaylistList': {
            const plist = getData(dataNode, 'uri');
            showDelPlaylist([plist]);
            break;
        }
        case 'BrowsePlaylistDetail': {
            const pos = getData(dataNode, 'pos');
            const plist = getDataId('BrowsePlaylistDetailList', 'uri');
            removeFromPlaylistPositions(plist, [pos]);
            break;
        }
        case 'QueueJukeboxSong':
        case 'QueueJukeboxAlbum': {
            const pos = getData(dataNode, 'pos');
            delQueueJukeboxEntries([pos]);
            break;
        }
        default:
            logError('Invalid appid' + app.id);
    }
}

/**
 * Handler for quick play button
 * @param {EventTarget} target event target
 * @returns {void}
 */
function clickQuickPlay(target) {
    let dataNode = target.parentNode.parentNode;
    if (dataNode.classList.contains('row')) {
        dataNode = dataNode.parentNode;
    }
    const type = getData(dataNode, 'type');
    const uri = [];
    switch(type) {
        case 'album':
            uri.push(getData(dataNode, 'AlbumId'));
            break;
        case 'disc':
            uri.push(getData(dataNode, 'AlbumId'), getData(dataNode, 'Disc'));
            break;
        default:
            uri.push(getData(dataNode, 'uri'));
    }
    switch (settings.webuiSettings.clickQuickPlay) {
        case 'append': return appendQueue(type, uri);
        case 'appendPlay': return appendPlayQueue(type, uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue(type, uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue(type, uri);
        case 'replace': return replaceQueue(type, uri);
        case 'replacePlay': return replacePlayQueue(type, uri);
        default: logError('Invalid action: ' + settings.webuiSettings.clickQuickPlay);
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
        case 'append': return appendQueue('song', [uri]);
        case 'appendPlay': return appendPlayQueue('song', [uri]);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', [uri]);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', [uri]);
        case 'replace': return replaceQueue('song', [uri]);
        case 'replacePlay': return replacePlayQueue('song', [uri]);
        case 'view': return songDetails(uri);
        case 'context': return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickSong);
    }
}

/**
 * Handler for webradioDB links
 * @param {string} uri stream uri
 * @param {event} event the event
 * @returns {void}
 */
function clickWebradiodb(uri, event) {
    switch (settings.webuiSettings.clickWebradiodb) {
        case 'append': return appendQueue('song', [uri]);
        case 'appendPlay': return appendPlayQueue('song', [uri]);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', [uri]);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', [uri]);
        case 'replace': return replaceQueue('song', [uri]);
        case 'replacePlay': return replacePlayQueue('song', [uri]);
        case 'view': return showWebradiodbDetails(uri);
        case 'context': return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickWebradiodb);
    }
}

/**
 * Handler for webradio favorites links
 * @param {string} uri webradio favorite uri (filename only)
 * @param {event} event the event
 * @returns {void}
 */
function clickRadioFavorites(uri, event) {
    switch(settings.webuiSettings.clickRadioFavorites) {
        case 'append': return appendQueue('webradio', [uri]);
        case 'appendPlay': return appendPlayQueue('webradio', [uri]);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('webradio', [uri]);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('webradio', [uri]);
        case 'replace': return replaceQueue('webradio', [uri]);
        case 'replacePlay': return replacePlayQueue('webradio', [uri]);
        case 'edit': return editRadioFavorite(uri);
        case 'context': return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickRadioFavorites);
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
            if (currentState.currentSongId === songid) {
                return clickPlay();
            }
            return sendAPI("MYMPD_API_PLAYER_PLAY_SONG", {
                "songId": songid
            }, null, false);
        case 'view':
            if (uri === null) {
                return;
            }
            return songDetails(uri);
        case 'context':
            return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickQueueSong);
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
        case 'append': return appendQueue('plist', [uri]);
        case 'appendPlay': return appendPlayQueue('plist', [uri]);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', [uri]);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', [uri]);
        case 'replace': return replaceQueue('plist', [uri]);
        case 'replacePlay': return replacePlayQueue('plist', [uri]);
        case 'view': return playlistDetails(uri);
        case 'context': return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickPlaylist);
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
        case 'append': return appendQueue('plist', [uri]);
        case 'appendPlay': return appendPlayQueue('plist', [uri]);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', [uri]);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', [uri]);
        case 'replace': return replaceQueue('plist', [uri]);
        case 'replacePlay': return replacePlayQueue('plist', [uri]);
        case 'view':
            //remember offset for current browse uri
            browseFilesystemHistory[app.current.filter] = {
                "offset": app.current.offset,
                "scrollPos": document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop
            };
            //reset search and show playlist
            app.current.search = '';
            appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, uri, app.current.sort, 'plist', '', 0);
            break;
        case 'context': return showContextMenu(event);
        default: logError('Invalid action: ' + settings.webuiSettings.clickFilesystemPlaylist);
    }
}

/**
 * Handler for click on folder in filesystem view
 * @param {string} uri folder uri
 * @returns {void}
 */
function clickFolder(uri) {
    //remember offset for current browse uri
    browseFilesystemHistory[app.current.filter] = {
        "offset": app.current.offset,
        "scrollPos": getScrollPosY()
    };
    //reset search and open folder
    app.current.search = '';
    appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, uri, app.current.sort, 'dir', '', 0);
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
            if (settings.webuiSettings.footerPlaybackControls === 'stop' ||
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
 * Handler for click on fast rewind button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickFastRewind() {
    clickSeek(-settings.webuiSettings.seekStep, true);
}

/**
 * Handler for click on fast rewind button in playback controls popover
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickFastRewindValue() {
    lastSeekStep = parseToSeconds(elGetById('popoverFooterSeekInput').value);
    clickSeek(-lastSeekStep, true);
}

/**
 * Handler for click on fast rewind button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickFastForward() {
    clickSeek(settings.webuiSettings.seekStep, true);
}

/**
 * Handler for click on fast rewind button in playback controls popover
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickFastForwardValue() {
    lastSeekStep = parseToSeconds(elGetById('popoverFooterSeekInput').value);
    clickSeek(lastSeekStep, true);
}

/**
 * Handler for click on goto position button in playback controls popover
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickGotoPos() {
    const seekToPos = parseToSeconds(elGetById('popoverFooterGotoInput').value);
    clickSeek(seekToPos, false);
}

/**
 * Shows the advanced playback control popover
 * @param {Event} event triggering event
 * @returns {void}
 */
function toggleAdvPlaycontrolsPopover(event) {
    if (event.target.closest('.dropdown-menu') !== null) {
        return;
    }
    event.preventDefault();
    event.stopPropagation();
    if (domCache.footer.getAttribute('aria-describedby') === null) {
        showPopover(domCache.footer, 'footer');
    }
    else {
        hidePopover();
    }
}

/**
 * Seek handler
 * @param {number} value seek by/to value
 * @param {boolean} relative true = number is relative
 * @returns {void}
 */
function clickSeek(value, relative) {
    sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
        "seek": value,
        "relative": relative
    }, null, false);
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

/**
 * Handler for click on consume button
 * @param {string} mode single mode: "0" = off, "1" = consume, "oneshot" = consume one shot
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clickConsume(mode) {
    sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
        "consume": mode
    }, null, false);
}

/**
 * Handler for resume dropdown actions
 * @param {Event} event Click event
 * @returns {void}
 */
function clickResumeSong(event) {
    event.preventDefault();
    if (event.target.nodeName !== 'BUTTON') {
        return;
    }
    const dataNode = event.target.closest('.btn-group');
    const uri = getData(dataNode, 'uri');
    const action = event.target.getAttribute('data-action');
    switch(action) {
        case 'append':
            sendAPI("MYMPD_API_QUEUE_APPEND_URI_RESUME", {
                'uri': uri
            }, null, false);
            break;
        case 'insert':
            sendAPI('MYMPD_API_QUEUE_INSERT_URI_RESUME', {
                'uri': uri,
                'to': 0,
                'whence': 0
            }, null, false);
            break;
        case 'replace':
            sendAPI("MYMPD_API_QUEUE_APPEND_URI_RESUME", {
                'uri': uri
            }, null, false);
            break;
        // No default
    }
}
