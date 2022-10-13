"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Central logging function
 * @param {Number} loglevel 
 * @param {String} message 
 */
function logLog(loglevel, message) {
    if (settings.loglevel >= loglevel) {
        switch(loglevel) {
            case 0:  console.error(message); break;
            case 1:  console.warn(message); break;
            case 4:  console.debug(message); break;
            default: console.log(message);
        }
    }
}

/**
 * Logs a message by jsonrpc severity
 * @param {String} severity
 * @param {String} message
 */
function logSeverity(severity, message) {
    switch (severity) {
        case 'error': logError(message); break;
        case 'warn':  logWarn(message); break;
        default:      logInfo(message);
    }
}

/**
 * Logs an error message
 * @param {String} message 
 */
//eslint-disable-next-line no-unused-vars
function logError(message) {
    logLog(0, 'ERROR: ' + message);
}

/**
 * Logs a warn message
 * @param {String} message 
 */
//eslint-disable-next-line no-unused-vars
function logWarn(message) {
    logLog(1, 'WARN: ' + message);
}

/**
 * Logs an info message
 * @param {String} message 
 */
//eslint-disable-next-line no-unused-vars
function logInfo(message) {
    logLog(2, 'INFO: ' + message);
}

/**
 * Logs a verbose message
 * @param {String} message 
 */
//eslint-disable-next-line no-unused-vars
function logVerbose(message) {
    logLog(3, 'VERBOSE: ' + message);
}

/**
 * Logs a debug message
 * @param {String} message 
 */
//eslint-disable-next-line no-unused-vars
function logDebug(message) {
    logLog(4, 'DEBUG: ' + message);
}
