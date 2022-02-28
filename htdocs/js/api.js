"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const ignoreMessages = ['No current song', 'No lyrics found'];

function removeEnterPinFooter(footer) {
    if (footer !== undefined) {
        elShow(footer.previousElementSibling);
        footer.remove();
        return;
    }
    const f = document.getElementsByClassName('enterPinFooter');
    for (let i = f.length - 1; i >= 0; i--) {
        const prev = f[i].previousElementSibling;
        if (prev.classList.contains('modal-footer')) {
            elShow(prev);
        }
        f[i].remove();
    }
}

function createEnterPinFooter(footers, method, params, callback, onerror) {
    const input = elCreateEmpty('input', {"type": "password", "autocomplete": "off", "class": ["form-control", "border-secondary"]});
    const btn = elCreateText('button', {"class": ["btn", "btn-success"]}, tn('Enter'));
    const newFooter = elCreateNode('div', {"class": ["modal-footer", "enterPinFooter"]},
        elCreateNodes('div', {"class": ["row", "w-100"]}, [
            elCreateText('label', {"class": ["col-4", "col-form-label", "ps-0"]}, tn('Enter pin')),
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
    input.focus();
    btn.addEventListener('click', function() {
        sendAPI('MYMPD_API_SESSION_LOGIN', {"pin": input.value}, function(obj) {
            input.value = '';
            const alert = footers[0].getElementsByClassName('alert')[0];
            if (alert !== undefined) {
                alert.remove();
            }
            if (obj.error) {
                newFooter.appendChild(
                    elCreateText('div', {"class": ["alert", "alert-danger", "p-2", "w-100"]}, tn(obj.error.message))
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

function enterPin(method, params, callback, onerror) {
    session.timeout = 0;
    setSessionState();
    const modal = getOpenModal();
    if (modal !== null) {
        logDebug('Show pin dialog in modal');
        //a modal is already opened, show enter pin dialog in footer
        const footer = modal.getElementsByClassName('modal-footer');
        createEnterPinFooter(footer, method, params, callback, onerror);
    }
    else {
        logDebug('Open pin modal');
        //open modal to enter pin and resend API request
        const enterBtn = elCreateText('button', {"id": "modalEnterPinEnterBtn", "class": ["btn", "btn-success"]}, tn('Enter'));
        enterBtn.addEventListener('click', function() {
            sendAPI('MYMPD_API_SESSION_LOGIN', {
                "pin": document.getElementById('inputPinModal').value},
            function(obj) {
                document.getElementById('inputPinModal').value = '';
                if (obj.error) {
                    const em = document.getElementById('modalEnterPinMessage');
                    em.textContent = tn(obj.error.message);
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

function resetSessionTimer() {
    if (sessionTimer !== null) {
        clearTimeout(sessionTimer);
        sessionTimer = null;
    }
    sessionTimer = setTimeout(function() {
        validateSession();
    }, sessionRenewInterval);
}

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

//eslint-disable-next-line no-unused-vars
function removeSession() {
    sendAPI('MYMPD_API_SESSION_LOGOUT', {}, function() {
        session.timeout = 0;
        setSessionState();
    }, false);
}

function sendAPI(method, params, callback, onerror) {
    if (APImethods[method] === undefined) {
        logError('Method "' + method + '" is not defined');
    }
    if (settings.pin === true &&
        session.token === '' &&
        session.timeout < getTimestamp() && APImethods[method].protected === true)
    {
        logDebug('Request must be authorized but we have no session');
        enterPin(method, params, callback, onerror);
        return false;
    }
    const request = {"jsonrpc": "2.0", "id": 0, "method": method, "params": params};
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', subdir + '/api/', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    if (session.token !== '') {
        ajaxRequest.setRequestHeader('Authorization', 'Bearer ' + session.token);
    }
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState !== 4) {
            return;
        }
        if (ajaxRequest.status === 401 &&
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
            showNotification(tn('Can not parse response to json object'), '', 'general', 'error');
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
            //rest is invalid
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

function webSocketConnect() {
    if (socket !== null &&
        socket.readyState === WebSocket.OPEN)
    {
        logDebug('Socket already connected');
        websocketConnected = true;
        return;
    }
    else if (socket !== null &&
        socket.readyState === WebSocket.CONNECTING)
    {
        logDebug('Socket connection in progress');
        websocketConnected = false;
        return;
    }

    websocketConnected = false;
    const wsUrl = (window.location.protocol === 'https:' ? 'wss://' : 'ws://') +
        window.location.hostname +
        (window.location.port !== '' ? ':' + window.location.port : '') +
        subdir + '/ws/';
    socket = new WebSocket(wsUrl);
    logDebug('Connecting to ' + wsUrl);

    try {
        socket.onopen = function() {
            logDebug('Websocket is connected');
            websocketConnected = true;
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
                    websocketConnected = true;
                    showNotification(tn('Connected to myMPD'), wsUrl, 'general', 'info');
                    //appRoute();
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
                        getQueue();
                    }
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
                    showNotification(tn('Connected to MPD'), '', 'general', 'info');
                    sendAPI('MYMPD_API_PLAYER_STATE', {}, parseState);
                    getSettings(true);
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI('MYMPD_API_PLAYER_OUTPUT_LIST', {
                        "partition":""
                    }, parseOutputs);
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
                        }, parsePlaylistsList);
                    }
                    else if (app.id === 'BrowsePlaylistsDetail') {
                        sendAPI('MYMPD_API_PLAYLIST_CONTENT_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "searchstr": app.current.search,
                            "plist": app.current.filter,
                            "cols": settings.colsBrowsePlaylistsDetailFetch
                        }, parsePlaylistsDetail);
                    }
                    break;
                case 'update_lastplayed':
                    if (app.id === 'QueueLastPlayed') {
                        sendAPI('MYMPD_API_QUEUE_LAST_PLAYED', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "cols": settings.colsQueueLastPlayedFetch,
                            "searchstr": app.current.search
                        }, parseLastPlayed);
                    }
                    break;
                case 'update_jukebox':
                    if (app.id === 'QueueJukebox') {
                        sendAPI('MYMPD_API_JUKEBOX_LIST', {
                            "offset": app.current.offset,
                            "limit": app.current.limit,
                            "cols": settings.colsQueueJukeboxFetch,
                            "searchstr": app.current.search
                        }, parseJukeboxList);
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
            websocketConnected = false;
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
    websocketConnected = false;
}

function websocketKeepAlive() {
    if (socket !== null &&
        socket.readyState === WebSocket.OPEN)
    {
        socket.send('ping');
    }
}
