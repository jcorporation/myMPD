"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module log_js */

/**
 * Central logging function
 * @param {number} loglevel the numeric loglevel: 0 = error, 1 = warn, 2 = info, 3 = verbose, 4 = debug
 * @param {string} message message to log
 * @returns {void}
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
 * @param {string} severity jsonrpc severity, on off error, warn, info
 * @param {string} message message to log
 * @returns {void}
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
 * @param {string} message message to log
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function logError(message) {
    logLog(0, 'ERROR: ' + message);
}

/**
 * Logs a warn message
 * @param {string} message message to log
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function logWarn(message) {
    logLog(1, 'WARN: ' + message);
}

/**
 * Logs an info message
 * @param {string} message message to log
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function logInfo(message) {
    logLog(2, 'INFO: ' + message);
}

/**
 * Logs a verbose message
 * @param {string} message message to log
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function logVerbose(message) {
    logLog(3, 'VERBOSE: ' + message);
}

/**
 * Logs a debug message
 * @param {string} message message to log
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function logDebug(message) {
    logLog(4, 'DEBUG: ' + message);
}
