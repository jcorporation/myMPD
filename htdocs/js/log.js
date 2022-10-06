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
        if (loglevel === 0) {
            console.error(message);
        }
        else if (loglevel === 1) {
            console.warn(message);
        }
        else if (loglevel === 4) {
            console.debug(message);
        }
        else {
            console.log(message);
        }
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
