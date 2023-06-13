"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module websocket_js */

/**
 * Checks if the websocket is connected
 * @returns {boolean} true if websocket is connected, else false
 */
function getWebsocketState() {
    return socket !== null && socket.readyState === WebSocket.OPEN;
}

/**
 * Connects to the websocket, registers the event handlers and enables the keepalive timer
 * @returns {void}
 */
function webSocketConnect() {
    if (getWebsocketState() === true)
    {
        logDebug('Socket already connected');
        return;
    }
    else if (socket !== null &&
        socket.readyState === WebSocket.CONNECTING)
    {
        logDebug('Socket connection in progress');
        return;
    }

    const wsUrl = (window.location.protocol === 'https:' ? 'wss://' : 'ws://') +
        window.location.hostname +
        (window.location.port !== '' ? ':' + window.location.port : '') +
        subdir + '/ws/' + localSettings.partition;
    socket = new WebSocket(wsUrl);
    logDebug('Connecting to ' + wsUrl);

    try {
        socket.onopen = function() {
            logDebug('Websocket is connected');
            socket.send('id:' + jsonrpcClientId);
            if (websocketKeepAliveTimer === null) {
                websocketKeepAliveTimer = setInterval(websocketKeepAlive, 25000);
            }
        };

        socket.onmessage = function(msg) {
            if (msg.data === 'pong') {
                //websocket keepalive
                logDebug('Got websocket pong');
                return;
            }
            if (msg.data === 'ok') {
                logDebug('Jsonrpc id registered successfuly');
                return;
            }
            if (msg.data.length > 100000) {
                logError("Websocket message is too large, discarding");
                return;
            }
            let obj;
            try {
                obj = JSON.parse(msg.data);
                logDebug('Websocket notification: ' + JSON.stringify(obj));
            }
            catch(error) {
                logError('Invalid websocket notification received: ' + msg.data);
                return;
            }

            // async response
            if (obj.result ||
                obj.error)
            {
                const callback = obj.result
                    ? getResponseCallback(obj.result.method)
                    : getResponseCallback(obj.error.method);
                checkAPIresponse(obj, callback, true);
                return;
            }

            // notification
            switch (obj.method) {
                case 'welcome':
                    showNotification(tn('Connected to myMPD') + ': ' +
                        tn('Partition') + ' ' + localSettings.partition, 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState, true);
                    if (session.token !== '') {
                        validateSession();
                    }
                    toggleAlert('alertMympdState', false, '');
                    break;
                case 'update_queue':
                case 'update_state':
                    //rename param to result
                    obj.result = obj.params;
                    delete obj.params;
                    if (app.id === 'QueueCurrent' &&
                        obj.method === 'update_queue')
                    {
                        getQueue(document.getElementById('searchQueueStr').value);
                    }
                    parseState(obj);
                    break;
                case 'mpd_disconnected':
                    if (progressTimer) {
                        clearTimeout(progressTimer);
                    }
                    settings.partition.mpdConnected = false;
                    toggleUI();
                    break;
                case 'mpd_connected':
                    //MPD connection established get state and settings
                    showNotification(tn('Connected to MPD'), 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState, false);
                    getSettings();
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI('MYMPD_API_PLAYER_OUTPUT_LIST', {}, parseOutputs, false);
                    break;
                case 'update_started':
                    showNotification(tn('Database update started'), 'database', 'info');
                    toggleAlert('alertUpdateDBState', true, tn('Updating MPD database'));
                    break;
                case 'update_database':
                case 'update_finished':
                    updateDBfinished(obj.method);
                    toggleAlert('alertUpdateDBState', false, '');
                    break;
                case 'update_volume':
                    //rename param to result
                    obj.result = obj.params;
                    delete obj.params;
                    parseVolume(obj);
                    break;
                case 'update_stored_playlist':
                    if (app.id === 'BrowsePlaylistList') {
                        sendAPI('MYMPD_API_PLAYLIST_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "searchstr": app.current.search,
                            "type": 0
                        }, parsePlaylistList, false);
                    }
                    else if (app.id === 'BrowsePlaylistDetail') {
                        sendAPI('MYMPD_API_PLAYLIST_CONTENT_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "searchstr": app.current.search,
                            "plist": app.current.filter,
                            "cols": settings.colsBrowsePlaylistDetailFetch
                        }, parsePlaylistsDetail, false);
                    }
                    break;
                case 'update_last_played':
                    if (app.id === 'QueueLastPlayed') {
                        sendAPI('MYMPD_API_LAST_PLAYED_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "cols": settings.colsQueueLastPlayedFetch,
                            "searchstr": app.current.search
                        }, parseLastPlayed, false);
                    }
                    break;
                case 'update_home':
                    if (app.id === 'Home') {
                        sendAPI("MYMPD_API_HOME_ICON_LIST", {}, parseHomeIcons, false);
                    }
                    break;
                case 'update_jukebox':
                    if (app.id === 'QueueJukebox') {
                        sendAPI('MYMPD_API_JUKEBOX_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "cols": settings.colsQueueJukeboxFetch,
                            "searchstr": app.current.search
                        }, parseJukeboxList, false);
                    }
                    break;
                case 'update_cache_started':
                    showNotification(tn('Cache update started'), 'database', 'info');
                    toggleAlert('alertUpdateCacheState', true, tn('Updating caches'));
                    break;
                case 'update_cache_finished':
                    if (app.id === 'BrowseDatabaseAlbumList') {
                        sendAPI("MYMPD_API_DATABASE_ALBUM_LIST", {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "expression": app.current.search,
                            "sort": app.current.sort.tag,
                            "sortdesc": app.current.sort.desc,
                            "cols": settings.colsBrowseDatabaseAlbumListFetch
                        }, parseDatabaseAlbumList, true);
                    }
                    toggleAlert('alertUpdateCacheState', false, '');
                    break;
                case 'notify':
                    showNotification(tn(obj.params.message, obj.params.data), obj.params.facility, obj.params.severity);
                    break;
                default:
                    logDebug('Unknown websocket notification: ' + obj.method);
                    break;
            }
        };

        socket.onclose = function(event) {
            logError('Websocket connection closed: ' + event.code);
            if (appInited === true) {
                toggleUI();
                if (progressTimer) {
                    clearTimeout(progressTimer);
                }
            }
            else {
                showAppInitAlert(tn('Websocket connection closed'));
            }
            socket = null;
        };

        socket.onerror = function() {
            logError('Websocket error occurred');
            if (socket !== null) {
                socket.close();
            }
        };
    }
    catch(error) {
        logError(error);
    }
}

/**
 * Closes the websocket and terminates the keepalive timer
 * @returns {void}
 */
function webSocketClose() {
    if (websocketKeepAliveTimer !== null) {
        clearInterval(websocketKeepAliveTimer);
        websocketKeepAliveTimer = null;
    }
    if (socket !== null) {
        //disable onclose handler first
        socket.onclose = function () {};
        socket.close();
        socket = null;
    }
}

/**
 * Sends a ping keepalive message to the websocket endpoint
 * or reconnects the socket.
 * @returns {void}
 */
function websocketKeepAlive() {
    if (getWebsocketState() === true) {
        socket.send('ping');
    }
    else {
        logDebug('Reconnecting websocket');
        toggleAlert('alertMympdState', true, tn('Websocket connection failed, trying to reconnect'));
        webSocketConnect();
    }
}
