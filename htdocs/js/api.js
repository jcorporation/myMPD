"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const ignoreMessages = ['No current song', 'No lyrics found'];

function sendAPI(method, params, callback, onerror) {
    const request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": params};
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', subdir + '/api/', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            if (ajaxRequest.responseText !== '') {
                let obj;
                try {
                    obj = JSON.parse(ajaxRequest.responseText);
                }
                catch(error) {
                    showNotification(t('Can not parse response to json object'), '', 'general', 'error');
                    logError('Can not parse response to json object:' + ajaxRequest.responseText);
                }
                if (obj.error) {
                    showNotification(t(obj.error.message, obj.error.data), '', obj.error.facility, obj.error.severity);
                    logError(JSON.stringify(obj.error));
                }
                else if (obj.result && obj.result.message && obj.result.message !== 'ok') {
                    logDebug('Got API response: ' + JSON.stringify(obj.result));
                    if (ignoreMessages.includes(obj.result.message) === false) {
                        showNotification(t(obj.result.message, obj.result.data), '', obj.result.facility, obj.result.severity);
                    }
                }
                else if (obj.result && obj.result.message && obj.result.message === 'ok') {
                    logDebug('Got API response: ' + JSON.stringify(obj.result));
                }
                else if (obj.result && obj.result.method) {
                    logDebug('Got API response of type: ' + obj.result.method);
                }
                else {
                    logError('Got invalid API response: ' + ajaxRequest.responseText);
                    if (onerror !== true) {
                        return;
                    }
                }
                if (callback !== undefined && typeof(callback) === 'function') {
                    if (obj.result !== undefined || onerror === true) {
                        logDebug('Calling ' + callback.name);
                        callback(obj);
                    }
                    else {
                        logDebug('Undefined resultset, skip calling ' + callback.name);
                    }
                }
            }
            else {
                logError('Empty response for request: ' + JSON.stringify(request));
                if (onerror === true) {
                    if (callback !== undefined && typeof(callback) === 'function') {
                        logDebug('Got empty API response calling ' + callback.name);
                        callback({"error": {"message": "Empty response"}});
                    }
                }
            }
        }
    };
    ajaxRequest.send(JSON.stringify(request));
    logDebug('Send API request: ' + method);
}

function webSocketConnect() {
    if (socket !== null && socket.readyState === WebSocket.OPEN) {
        logInfo('Socket already connected');
        websocketConnected = true;
        return;
    }
    else if (socket !== null && socket.readyState === WebSocket.CONNECTING) {
        logInfo('Socket connection in progress');
        websocketConnected = false;
        return;
    }

    websocketConnected = false;  
    const wsUrl = (window.location.protocol === 'https:' ? 'wss://' : 'ws://') +
        window.location.hostname + 
        (window.location.port !== '' ? ':' + window.location.port : '') + subdir + '/ws/';
    socket = new WebSocket(wsUrl);
    logInfo('Connecting to ' + wsUrl);

    try {
        socket.onopen = function() {
            logInfo('Websocket is connected');
            websocketConnected = true;
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
        };

        socket.onmessage = function(msg) {
            let obj;
            try {
                obj = JSON.parse(msg.data);
                logDebug('Websocket notification: ' + JSON.stringify(obj));
            }
            catch(error) {
                logError('Invalid JSON data received: ' + msg.data);
                return;
            }
            
            switch (obj.method) {
                case 'welcome':
                    websocketConnected = true;
                    showNotification(t('Connected to myMPD'), wsUrl, 'general', 'info');
                    //appRoute();
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState, true);
                    break;
                case 'update_state':
                    obj.result = obj.params;
                    parseState(obj);
                    break;
                case 'mpd_disconnected':
                    if (progressTimer) {
                        clearTimeout(progressTimer);
                    }
                    getSettings(true);
                    break;
                case 'mpd_connected':
                    //MPD connection established get state and settings
                    showNotification(t('Connected to MPD'), '', 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState);
                    getSettings(true);
                    break;
                case 'update_queue':
                    if (app.current.app === 'Queue') {
                        getQueue();
                    }
                    obj.result = obj.params;
                    parseUpdateQueue(obj);
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI('MYMPD_API_PLAYER_OUTPUT_LIST', {}, parseOutputs);
                    break;
                case 'update_started':
                    updateDBstarted(false);
                    break;
                case 'update_database':
                    //fall through
                case 'update_finished':
                    updateDBfinished(obj.method);
                    break;
                case 'update_volume':
                    obj.result = obj.params;
                    parseVolume(obj);
                    break;
                case 'update_stored_playlist':
                    if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
                        sendAPI('MYMPD_API_PLAYLIST_LIST', {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search}, parsePlaylistsList);
                    }
                    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
                        sendAPI('MYMPD_API_PLAYLIST_CONTENT_LIST', {"offset": app.current.offset, "limit": app.current.limit, "searchstr": app.current.search, "plist": app.current.filter, "cols": settings.colsBrowsePlaylistsDetail}, parsePlaylistsDetail);
                    }
                    break;
                case 'update_lastplayed':
                    if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
                        sendAPI('MYMPD_API_QUEUE_LAST_PLAYED', {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueLastPlayed}, parseLastPlayed);
                    }
                    break;
                case 'update_jukebox':
                    if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
                        sendAPI('MYMPD_API_JUKEBOX_LIST', {"offset": app.current.offset, "limit": app.current.limit, "cols": settings.colsQueueJukebox}, parseJukeboxList);
                    }
                    break;
                case 'notify':
                    if (document.getElementById('alertMpdState').classList.contains('hide')) {
                        showNotification(t(obj.params.message, obj.params.data), '', obj.params.facility, obj.params.severity);
                    }
                    break;
                default:
                    break;
            }
        };

        socket.onclose = function(event) {
            logError('Websocket is disconnected');
            websocketConnected = false;
            if (appInited === true) {
                toggleUI();
                if (progressTimer) {
                    clearTimeout(progressTimer);
                }
            }
            else {
                showAppInitAlert(t('Websocket connection failed'));
                logError('Websocket connection failed: ' + event.code);
            }
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
            websocketTimer = setTimeout(function() {
                logInfo('Reconnecting websocket');
                toggleAlert('alertMympdState', true, t('Websocket connection failed, trying to reconnect') + '&nbsp;&nbsp;<div class="spinner-border spinner-border-sm"></div>');
                webSocketConnect();
            }, 3000);
            socket = null;
        };
        
        socket.onerror = function() {
            logError('Websocket error occured');
            if (socket !== null) {
                socket.close();
            }
        };
    }
    catch(error) {
        logError(error);
    }
}

function webSocketClose() {
    if (websocketTimer !== null) {
        clearTimeout(websocketTimer);
        websocketTimer = null;
    }
    if (socket !== null) {
        // disable onclose handler first
        socket.onclose = function () {}; 
        socket.close();
        socket = null;
    }
    websocketConnected = false;
}
