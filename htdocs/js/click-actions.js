"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//functions to execute default actions
function clickQuickRemove(target) {
    switch(app.id) {
        case 'QueueCurrent': {
            const songId = getData(target.parentNode.parentNode, 'songid');
            removeFromQueue('single', songId);
            break;
        }
        case 'BrowsePlaylistsDetail': {
            const pos = getData(target.parentNode.parentNode, 'songpos');
            const plist = getDataId('BrowsePlaylistsDetailList', 'uri');
            removeFromPlaylist('single', plist, pos);
            break;
        }
    }
}

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
 * @param {String} uri song uri
 */
function clickSong(uri) {
    switch (settings.webuiSettings.clickSong) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
        case 'view': return songDetails(uri);
    }
}

function clickRadiobrowser(uri, uuid) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
    }
    countClickRadiobrowser(uuid);
}

function clickWebradiodb(uri) {
    switch (settings.webuiSettings.clickRadiobrowser) {
        case 'append': return appendQueue('song', uri);
        case 'appendPlay': return appendPlayQueue('song', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('song', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('song', uri);
        case 'replace': return replaceQueue('song', uri);
        case 'replacePlay': return replacePlayQueue('song', uri);
    }
}

function clickRadioFavorites(uri) {
    const fullUri = getRadioFavoriteUri(uri);
    switch(settings.webuiSettings.clickRadioFavorites) {
        case 'append': return appendQueue('plist', fullUri);
        case 'appendPlay': return appendPlayQueue('plist', fullUri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', fullUri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', fullUri);
        case 'replace': return replaceQueue('plist', fullUri);
        case 'replacePlay': return replacePlayQueue('plist', fullUri);
        case 'edit': return editRadioFavorite(uri);
    }
}

function clickQueueSong(songid, uri) {
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
    }
}

function clickPlaylist(uri) {
    switch(settings.webuiSettings.clickPlaylist) {
        case 'append': return appendQueue('plist', uri);
        case 'appendPlay': return appendPlayQueue('plist', uri);
        case 'insertAfterCurrent': return insertAfterCurrentQueue('plist', uri);
        case 'insertPlayAfterCurrent': return insertPlayAfterCurrentQueue('plist', uri);
        case 'replace': return replaceQueue('plist', uri);
        case 'replacePlay': return replacePlayQueue('plist', uri);
        case 'view': return playlistDetails(uri);
    }
}

function clickFilesystemPlaylist(uri) {
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
    }
}

function clickFolder(uri) {
    //remember offset for current browse uri
    browseFilesystemHistory[app.current.search] = {
        "offset": app.current.offset,
        "scrollPos": getScrollPosY()
    };
    //reset filter and open folder
    app.current.filter = '-';
    appGoto('Browse', 'Filesystem', undefined, 0, app.current.limit, app.current.filter, app.current.sort, 'dir', uri);
}

function seekRelativeForward() {
    seekRelative(5);
}

function seekRelativeBackward() {
    seekRelative(-5);
}

function seekRelative(offset) {
    sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
        "seek": offset,
        "relative": true
    }, null, false);
}

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

//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MYMPD_API_PLAYER_STOP", {}, null, false);
}

//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MYMPD_API_PLAYER_PREV", {}, null, false);
}

//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MYMPD_API_PLAYER_NEXT", {}, null, false);
}

//eslint-disable-next-line no-unused-vars
function clickSingle(mode) {
    //mode: 0 = off, 1 = single, 2 = single one shot
    sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
        "single": mode
    }, null, false);
}
