"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module api_js */

/**
 * This messages are hidden from notifications.
 */
/** @type {object} */
const ignoreMessages = ['No current song', 'No lyrics found'];

/**
 * Sends a JSON-RPC API request to the selected partition and handles the response.
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {boolean} true on success, else false
 */
 function sendAPI(method, params, callback, onerror) {
    return sendAPIpartition(localSettings.partition, method, params, callback, onerror);
 }

/**
 * Sends a JSON-RPC API request and handles the response.
 * @param {string} partition partition endpoint
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {boolean} true on success, else false
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
            //show and log message
            showNotification(tn(obj.error.message, obj.error.data), '', obj.error.facility, obj.error.severity);
            logSeverity(obj.error.severity, ajaxRequest.responseText);
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
