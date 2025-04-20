"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module log_js */

/**
 * Central logging function
 * It logs to the browser console and to the log buffer
 * @param {number} loglevel the numeric syslog loglevel
 * @param {string} message message to log
 * @returns {void}
 */
function logLog(loglevel, message) {
    if (settings.loglevel >= loglevel) {
        switch(loglevel) {
            case 0:
            case 1:
            case 2:
            case 3:
                console.error(logLevelNames[loglevel] + ': ' + message);
                break;
            case 4:
                console.warn(logLevelNames[loglevel] + ': ' + message);
                break;
            case 5:
            case 6:
                console.log(logLevelNames[loglevel] + ': ' + message);
                break;
            case 7:
                console.debug(logLevelNames[loglevel] + ': ' + message);
                break;
            // no default
        }
        logs.push({
            "timestamp": getTimestamp(),
            "loglevel": loglevel,
            "message": message
        });
        if (logs.length > logsMax) {
            logs.shift();
        }
    }
}

/**
 * Logs a message by severity name
 * @param {string} severity jsonrpc severity, one off: emerg, alert, crit, error, warn, notice, info, debug
 * @param {string} message message to log
 * @returns {void}
 */
function logSeverity(severity, message) {
    switch (severity) {
        case 'emerg':  logEmerg(message); break;
        case 'alert':  logAlert(message); break;
        case 'crit':   logCrit(message); break;
        case 'error':  logError(message); break;
        case 'warn':   logWarn(message); break;
        case 'notice': logNotice(message); break;
        case 'info':   logInfo(message); break;
        case 'debug':  logDebug(message); break;
        // no default
    }
}

/**
 * Logs an emergency message
 * @param {string} message message to log
 * @returns {void}
 */
function logEmerg(message) {
    logLog(0, message);
}

/**
 * Logs an alert message
 * @param {string} message message to log
 * @returns {void}
 */
function logAlert(message) {
    logLog(1, message);
}

/**
 * Logs a critical message
 * @param {string} message message to log
 * @returns {void}
 */
function logCrit(message) {
    logLog(2, message);
}

/**
 * Logs an error message
 * @param {string} message message to log
 * @returns {void}
 */
function logError(message) {
    logLog(3, message);
}

/**
 * Logs a warn message
 * @param {string} message message to log
 * @returns {void}
 */
function logWarn(message) {
    logLog(4, message);
}

/**
 * Logs a notice message
 * @param {string} message message to log
 * @returns {void}
 */
function logNotice(message) {
    logLog(5, message);
}

/**
 * Logs an info message
 * @param {string} message message to log
 * @returns {void}
 */
function logInfo(message) {
    logLog(6, message);
}

/**
 * Logs a debug message
 * @param {string} message message to log
 * @returns {void}
 */
function logDebug(message) {
    logLog(7, message);
}
