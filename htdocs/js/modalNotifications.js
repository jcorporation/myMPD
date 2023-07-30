"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalNotifications_js */

/**
 * Initializes the notification html elements
 * @returns {void}
 */
function initModalNotifications() {
    document.getElementById('modalNotifications').addEventListener('show.bs.modal', function() {
        showMessages();
    });
}

/**
 * Lists the logbuffer in the logOverview element
 * @returns {void}
 */
function showMessages() {
    const overview = document.getElementById('logOverview');
    elClear(overview);
    for (const message of messages) {
        overview.insertBefore(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, fmtTime(message.timestamp)),
                elCreateNodes('td', {}, [
                    createSeverityIcon(message.severity),
                    document.createTextNode(tn(facilities[message.facility]))
                ]),
                elCreateText('td', {}, message.occurrence),
                elCreateNodes('td', {}, [
                    elCreateText('p', {"class": ["mb-0"]}, message.message)
                ])
            ]),
        overview.firstElementChild);
    }
    if (overview.querySelector('tr') === null) {
        overview.appendChild(emptyRow(4));
    }
}

/**
 * Clears the logbuffer
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearMessages() {
    const overview = document.getElementById('logOverview');
    elClear(overview);
    overview.appendChild(emptyRow(4));
    messages.length = 0;
}
