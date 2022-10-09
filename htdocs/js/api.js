"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * This messages are hidden from notifications.
 */
/** @type {Object} */
const ignoreMessages = ['No current song', 'No lyrics found'];

/**
 * Removes the enter pin dialog from a modal footer.
 * @param {HTMLElement} footer parent element of the enter pin dialog
 */
function removeEnterPinFooter(footer) {
    if (footer !== undefined) {
        elShow(footer.previousElementSibling);
        footer.remove();
        return;
    }
    const f = document.querySelectorAll('.enterPinFooter');
    for (let i = f.length - 1; i >= 0; i--) {
        const prev = f[i].previousElementSibling;
        if (prev.classList.contains('modal-footer')) {
            elShow(prev);
        }
        f[i].remove();
    }
}

/**
 * Creates the enter pin footer and sends the original api request after the session is created.
 * @param {NodeList} footers modal footers to hide
 * @param {String} method jsonrpc method of the original api request
 * @param {Object} params json object of the original api request
 * @param {Function} callback callback function of the original api request
 * @param {Boolean} onerror true = execute callback also on error
 */
function createEnterPinFooter(footers, method, params, callback, onerror) {
    const input = elCreateEmpty('input', {"type": "password", "autocomplete": "off", "class": ["form-control", "border-secondary"]});
    const btn = elCreateTextTn('button', {"class": ["btn", "btn-success"]}, 'Enter');
    const newFooter = elCreateNode('div', {"class": ["modal-footer", "enterPinFooter"]},
        elCreateNodes('div', {"class": ["row", "w-100"]}, [
            elCreateTextTn('label', {"class": ["col-4", "col-form-label", "ps-0"]}, 'Enter pin'),
            elCreateNode('div', {"class": ["col-8", "pe-0"]},
                elCreateNodes('div', {"class": ["input-group"]}, [
                    input,
                    btn
                ])
            )
        ])
    );
    for (const footer of footers) {
        footer.classList.add('d-none');
    }
    footers[0].parentNode.appendChild(newFooter);
    setFocus(input);
    btn.addEventListener('click', function() {
        sendAPI('MYMPD_API_SESSION_LOGIN', {"pin": input.value}, function(obj) {
            input.value = '';
            const alert = footers[0].querySelector('.alert');
            if (alert !== undefined) {
                alert.remove();
            }
            if (obj.error) {
                newFooter.appendChild(
                    elCreateTextTn('div', {"class": ["alert", "alert-danger", "p-2", "w-100"]}, obj.error.message, obj.error.data)
                );
            }
            else if (obj.result.session !== '') {
                session.token = obj.result.session;
                session.timeout = getTimestamp() + sessionLifetime;
                setSessionState();
                removeEnterPinFooter(newFooter);
                showNotification(tn('Session successfully created'), '', 'session', 'info');
                if (method !== undefined) {
                    //call original API
                    sendAPI(method, params, callback, onerror);
                }
            }
        }, true);
    }, false);
    input.addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            btn.click();
        }
    }, false);
}

/**
 * Shows the enter pin dialog in a new model or if a modal is already opened in the footer of this modal.
 * @param {String} method jsonrpc method of the original api request
 * @param {Object} params json object of the original api request
 * @param {Function} callback callback function of the original api request
 * @param {Boolean} onerror true = execute callback also on error
 */
function enterPin(method, params, callback, onerror) {
    session.timeout = 0;
    setSessionState();
    const modal = getOpenModal();
    if (modal !== null) {
        logDebug('Show pin dialog in modal');
        //a modal is already opened, show enter pin dialog in footer
        const footer = modal.querySelectorAll('.modal-footer');
        createEnterPinFooter(footer, method, params, callback, onerror);
    }
    else {
        logDebug('Open pin modal');
        //open modal to enter pin and resend API request
        const enterBtn = elCreateTextTn('button', {"id": "modalEnterPinEnterBtn", "class": ["btn", "btn-success"]}, 'Enter');
        enterBtn.addEventListener('click', function() {
            sendAPI('MYMPD_API_SESSION_LOGIN', {
                "pin": document.getElementById('inputPinModal').value},
                function(obj) {
                    document.getElementById('inputPinModal').value = '';
                    if (obj.error) {
                        const em = document.getElementById('modalEnterPinMessage');
                        em.textContent = tn(obj.error.message, obj.error.data);
                        elShow(em);
                    }
                    else if (obj.result.session !== '') {
                        session.token = obj.result.session;
                        session.timeout = getTimestamp() + sessionLifetime;
                        setSessionState();
                        uiElements.modalEnterPin.hide();
                        showNotification(tn('Session successfully created'), '', 'session', 'info');
                        if (method !== undefined) {
                            //call original API
                            sendAPI(method, params, callback, onerror);
                        }
                    }
                }, true);
        }, false);
        document.getElementById('modalEnterPinEnterBtn').replaceWith(enterBtn);
        elHideId('modalEnterPinMessage');
        document.getElementById('inputPinModal').value = '';
        uiElements.modalEnterPin.show();
    }
}

/**
 * Sets the session state.
 * Shows/hides the lock indicator and the login/logout menu entry.
 */
function setSessionState() {
    if (session.timeout < getTimestamp()) {
        logDebug('Session expired: ' + session.timeout);
        session.timeout = 0;
        session.token = '';
    }
    if (settings.pin === true) {
        if (session.token === '') {
            domCache.body.classList.add('locked');
            elShowId('mmLogin');
            elHideId('mmLogout');
        }
        else {
            domCache.body.classList.remove('locked');
            elShowId('mmLogout');
            elHideId('mmLogin');
            resetSessionTimer();
        }
    }
    else {
        domCache.body.classList.remove('locked');
        elHideId('mmLogin');
        elHideId('mmLogout');
    }
}

/**
 * Resets the session timer.
 */
function resetSessionTimer() {
    if (sessionTimer !== null) {
        clearTimeout(sessionTimer);
        sessionTimer = null;
    }
    sessionTimer = setTimeout(function() {
        validateSession();
    }, sessionRenewInterval);
}

/**
 * Validates a session by calling the MYMPD_API_SESSION_VALIDATE endpoint
 * and calls setSessionState to update the DOM.
 */
function validateSession() {
    sendAPI('MYMPD_API_SESSION_VALIDATE', {}, function(obj) {
        if (obj.result !== undefined &&
            obj.result.message === 'ok')
        {
            session.timeout = getTimestamp() + sessionLifetime;
        }
        else {
            session.timeout = 0;
        }
        setSessionState();
    }, true);
}

/**
 * Removes a session by calling the MYMPD_API_SESSION_LOGOUT endpoint
 * and calls setSessionState to update the DOM.
 */
//eslint-disable-next-line no-unused-vars
function removeSession() {
    sendAPI('MYMPD_API_SESSION_LOGOUT', {}, function() {
        session.timeout = 0;
        setSessionState();
    }, false);
}

/**
 * Sends a JSON-RPC API request to the selected partition and handles the response.
 * @param {String} method jsonrpc api method
 * @param {Object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {Boolean} onerror true = execute callback also on error
 * @returns {Boolean} true on success, else false
 */
 function sendAPI(method, params, callback, onerror) {
    return sendAPIpartition(localSettings.partition, method, params, callback, onerror);
 }

/**
 * Sends a JSON-RPC API request and handles the response.
 * @param {String} partition partition endpoint
 * @param {String} method jsonrpc api method
 * @param {Object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {Boolean} onerror true = execute callback also on error
 * @returns {Boolean} true on success, else false
 */
function sendAPIpartition(partition, method, params, callback, onerror) {
    if (APImethods[method] === undefined) {
        logError('Method "' + method + '" is not defined');
    }
    if (settings.pin === true &&
        session.token === '' &&
        session.timeout < getTimestamp() &&
        APImethods[method].protected === true)
    {
        logDebug('Request must be authorized but we have no session');
        enterPin(method, params, callback, onerror);
        return false;
    }
    //we do not use the jsonrpc id field because we get the response directly.
    const request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": params};
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', subdir + '/api/' + partition, true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    if (session.token !== '') {
        ajaxRequest.setRequestHeader('X-myMPD-Session', session.token);
    }
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState !== 4) {
            return;
        }
        if (ajaxRequest.status === 403 &&
            method !== 'MYMPD_API_SESSION_VALIDATE')
        {
            logDebug('Authorization required for ' + method);
            enterPin(method, params, callback, onerror);
            return;
        }
        if (ajaxRequest.status !== 200 ||
            ajaxRequest.responseText === '' ||
            ajaxRequest.responseText.length > 1000000)
        {
            logError('Invalid response for request: ' + JSON.stringify(request));
            logError('Response code: ' + ajaxRequest.status);
            logError('Response length: ' + ajaxRequest.responseText.length);
            if (onerror === true) {
                if (callback !== undefined && typeof(callback) === 'function') {
                    logDebug('Got empty API response calling ' + callback.name);
                    callback({"error": {"message": "Invalid response"}});
                }
            }
            return;
        }

        if (settings.pin === true &&
            session.token !== '' &&
            APImethods[method].protected === true)
        {
            //session was extended through request
            session.timeout = getTimestamp() + sessionLifetime;
            resetSessionTimer();
        }
        let obj;
        try {
            obj = JSON.parse(ajaxRequest.responseText);
        }
        catch(error) {
            showNotification(tn('Can not parse response from %{uri} to json object', {"uri": subdir + '/api/' + partition}), '', 'general', 'error');
            logError('Can not parse response to json object:' + ajaxRequest.responseText);
        }
        if (obj.error &&
            typeof obj.error.message === 'string')
        {
            //show error message
            showNotification(tn(obj.error.message, obj.error.data), '', obj.error.facility, obj.error.severity);
            logError(ajaxRequest.responseText);
        }
        else if (obj.result &&
                 obj.result.message === 'ok')
        {
            //show no message
            logDebug('Got API response: ' + ajaxRequest.responseText);
        }
        else if (obj.result &&
                 typeof obj.result.message === 'string')
        {
            //show message
            logDebug('Got API response: ' + ajaxRequest.responseText);
            if (ignoreMessages.includes(obj.result.message) === false) {
                showNotification(tn(obj.result.message, obj.result.data), '', obj.result.facility, obj.result.severity);
            }
        }
        else if (obj.result &&
                 typeof obj.result.method === 'string')
        {
            //result is used in callback
            logDebug('Got API response of type: ' + obj.result.method);
        }
        else {
            //remaining results are invalid
            logError('Got invalid API response: ' + ajaxRequest.responseText);
            if (onerror !== true) {
                return;
            }
        }
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            if (obj.result !== undefined ||
                onerror === true)
            {
                logDebug('Calling ' + callback.name);
                callback(obj);
            }
            else {
                logDebug('Undefined resultset, skip calling ' + callback.name);
            }
        }
    };
    ajaxRequest.send(JSON.stringify(request));
    logDebug('Send API request: ' + method);
    return true;
}

/**
 * Checks if the websocket is connected
 * @returns {Boolean} true if websocket is connected, else false
 */
function getWebsocketState() {
    return socket !== null && socket.readyState === WebSocket.OPEN;
}

/**
 * Connects to the websocket and registers the event handlers.
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
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
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

            switch (obj.method) {
                case 'welcome':
                    showNotification(tn('Connected to myMPD'),
                        tn('Partition') + ': ' + localSettings.partition, 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState, true);
                    if (session.token !== '') {
                        validateSession();
                    }
                    break;
                case 'update_queue':
                case 'update_state':
                    //rename param to result
                    obj.result = obj.params;
                    delete obj.params;
                    if (app.id === 'QueueCurrent' &&
                        obj.method === 'update_queue')
                    {
                        getQueue(document.getElementById('searchQueueStr').getAttribute('value'));
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
                    showNotification(tn('Connected to MPD'), '', 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState, false);
                    getSettings(true);
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI('MYMPD_API_PLAYER_OUTPUT_LIST', {}, parseOutputs, false);
                    break;
                case 'update_started':
                    updateDBstarted(false, true);
                    break;
                case 'update_database':
                case 'update_finished':
                    updateDBfinished(obj.method);
                    break;
                case 'update_volume':
                    //rename param to result
                    obj.result = obj.params;
                    delete obj.params;
                    parseVolume(obj);
                    break;
                case 'update_stored_playlist':
                    if (app.id === 'BrowsePlaylistsList') {
                        sendAPI('MYMPD_API_PLAYLIST_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "searchstr": app.current.search,
                            "type": 0
                        }, parsePlaylistsList, false);
                    }
                    else if (app.id === 'BrowsePlaylistsDetail') {
                        sendAPI('MYMPD_API_PLAYLIST_CONTENT_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "searchstr": app.current.search,
                            "plist": app.current.filter,
                            "cols": settings.colsBrowsePlaylistsDetailFetch
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
                case 'update_album_cache':
                    if (app.id === 'BrowseDatabaseList' &&
                        app.current.tag === 'Album')
                    {
                        sendAPI("MYMPD_API_DATABASE_ALBUMS_GET", {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "expression": app.current.search,
                            "sort": app.current.sort.tag,
                            "sortdesc": app.current.sort.desc
                        }, parseDatabase, true);
                    }
                    break;
                case 'notify':
                    showNotification(tn(obj.params.message, obj.params.data), '', obj.params.facility, obj.params.severity);
                    break;
                default:
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
            if (websocketTimer !== null) {
                clearTimeout(websocketTimer);
                websocketTimer = null;
            }
            if (websocketKeepAliveTimer !== null) {
                clearInterval(websocketKeepAliveTimer);
                websocketKeepAliveTimer = null;
            }
            websocketTimer = setTimeout(function() {
                logDebug('Reconnecting websocket');
                toggleAlert('alertMympdState', true, tn('Websocket connection failed, trying to reconnect'));
                webSocketConnect();
            }, 3000);
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
 * Closes the websocket and terminates the keepalive and reconnect timer
 */
function webSocketClose() {
    if (websocketTimer !== null) {
        clearTimeout(websocketTimer);
        websocketTimer = null;
    }
    if (websocketKeepAliveTimer) {
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
 * Sends a ping keepalive message to the websocket endpoint.
 */
function websocketKeepAlive() {
    if (getWebsocketState() === true) {
        socket.send('ping');
    }
}
