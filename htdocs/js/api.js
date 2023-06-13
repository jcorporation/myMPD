"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module api_js */

/**
 * This messages are hidden from notifications.
 */
/** @type {object} */
const ignoreMessages = [
    'ok',
    'No current song',
    'No lyrics found'
];

/**
 * Sends a JSON-RPC API request to the selected partition and handles the response.
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {void}
 */
function sendAPI(method, params, callback, onerror) {
    sendAPIpartition(localSettings.partition, method, params, callback, onerror);
}

/**
 * Sends a JSON-RPC API request and handles the response.
 * @param {string} partition partition endpoint
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {Promise<void>}
 */
async function sendAPIpartition(partition, method, params, callback, onerror) {
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
        return;
    }

    logDebug('Send API request: ' + method);
    const uri = subdir + '/api/' + partition;
    const headers = {'Content-Type': 'application/json'};
    if (session.token !== '') {
        headers['X-myMPD-Session'] = session.token;
    }
    let response = null;
    try {
        response = await fetch(uri, {
            method: 'POST',
            mode: 'same-origin',
            credentials: 'same-origin',
            cache: 'no-store',
            redirect: 'follow',
            headers: headers,
            body: JSON.stringify(
                {"jsonrpc": "2.0", "id": generateJsonrpcId(), "method": method, "params": params}
            )
        });
    }
    catch(error) {
        showNotification(tn('API error') + '\n' + tn('Error accessing %{uri}', {"uri": uri}), 'general', 'error');
        logError('Error posting to ' + uri);
        logError(error);
        if (onerror === true) {
            callback(null);
        }
        return;
    }

    if (response.redirected === true) {
        window.location.reload();
        logError('Request was redirect, reloading application');
        return;
    }
    if (response.status === 403 &&
        method !== 'MYMPD_API_SESSION_VALIDATE')
    {
        //myMPD session authentication
        logDebug('Authorization required for ' + method);
        enterPin(method, params, callback, onerror);
        return;
    }
    if (response.ok === false) {
        showNotification(tn('API error') + '\n' +
            tn('Error accessing %{uri}', {"uri": uri}) + '\n' +
            tn('Response code: %{code}', {"code": response.status + ' - ' + response.statusText}),
            'general', 'error');
        logError('Error posting to ' + uri + ', code ' + response.status + ' - ' + response.statusText);
        if (onerror === true) {
            callback(null);
        }
        return;
    }
    
    //successful http response - extend session
    if (settings.pin === true &&
        session.token !== '' &&
        APImethods[method].protected === true)
    {
        //session was extended through request
        session.timeout = getTimestamp() + sessionLifetime;
        resetSessionTimer();
    }

    //parse response
    try {
        const obj = await response.json();
        checkAPIresponse(obj, callback, onerror);
    }
    catch(error) {
        showNotification(tn('API error') + '\n' + tn('Can not parse response from %{uri}', {"uri": uri}), 'general', 'error');
        logError('Can not parse response from ' + uri);
        logError(error);
        if (onerror === true) {
            callback(null);
        }
        return;
    }
}

/**
 * Validates the JSON-RPC API response and calls the callback function
 * @param {object} obj parsed json rpc response object
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {void}
 */
function checkAPIresponse(obj, callback, onerror) {
    logDebug('Got API response: ' + JSON.stringify(obj));

    if (obj.error &&
        typeof obj.error.message === 'string')
    {
        //show and log message
        showNotification(tn(obj.error.message, obj.error.data), obj.error.facility, obj.error.severity);
        logSeverity(obj.error.severity, JSON.stringify(obj));
    }
    else if (obj.result &&
             typeof obj.result.message === 'string')
    {
        //show message
        if (ignoreMessages.includes(obj.result.message) === false) {
            showNotification(tn(obj.result.message, obj.result.data), obj.result.facility, obj.result.severity);
        }
    }
    else if (obj.result &&
             typeof obj.result.method === 'string')
    {
        //result is used in callback
    }
    else {
        //remaining results are invalid
        logError('Got invalid API response: ' + JSON.stringify(obj));
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
            logDebug('Result is undefined, skip calling ' + callback.name);
        }
    }
}

/**
 * Gets the callback for an jsonrpc method.
 * Used for async jsonrpc responses.
 * @param {string} method jsonrpc method
 * @returns {Function} the function that can parse the response, or null
 */
function getResponseCallback(method) {
    switch(method) {
        default:
            return null;
    }
}

/**
 * Generates a uniq jsonrpcid, keeping the clientId the same.
 * Wraps around the requestId.
 * @returns {number} the jsonrpcid
 */
function generateJsonrpcId() {
    jsonrpcRequestId = jsonrpcRequestId === 999
        ? 0
        : ++jsonrpcRequestId;
    return jsonrpcClientId * 1000 + jsonrpcRequestId;
}
