"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function logError(line) {
    logLog(0, 'ERROR: ' + line);
}

function logWarn(line) {
    logLog(1, 'WARN: ' + line);
}

function logInfo(line) {
    logLog(2, 'INFO: ' + line);
}

function logVerbose(line) {
    logLog(3, 'VERBOSE: ' + line);
}

function logDebug(line) {
    logLog(4, 'DEBUG: ' + line);
}

function logLog(loglevel, line) {
    if (settings.loglevel >= loglevel) {
        if (loglevel === 0) {
            console.error(line);
        }
        else if (loglevel === 1) {
            console.warn(line);
        }
        else if (loglevel === 4) {
            console.debug(line);
        }
        else {
            console.log(line);
        }
    }
}
