"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalNotifications_js */

/**
 * Initialization function for the notification modal elements
 * @returns {void}
 */
function initModalNotifications() {
    elGetById('modalNotifications').addEventListener('show.bs.modal', function() {
        showMessages();
    });
}

/**
 * Lists the logbuffer in the modalNotificationsList element
 * @returns {void}
 */
function showMessages() {
    const overview = elGetById('modalNotificationsList');
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
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearMessages(target) {
    btnWaiting(target, true);
    const overview = elGetById('modalNotificationsList');
    elClear(overview);
    overview.appendChild(emptyRow(4));
    messages.length = 0;
    btnWaiting(target, false);
}
