"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalNotifications_js */

/**
 * Initialization function for the notification modal elements
 * @returns {void}
 */
function initModalNotifications() {
    elGetById('modalNotifications').addEventListener('show.bs.modal', function() {
        elShowId('modalNotificationsList');
        elHideId('modalNotificationsLogs');
        elGetById('modalNotificationsLogsBtn').classList.remove('active');
        showMessages();
    });
}

/**
 * Lists the messages in the modalNotificationsList element
 * @returns {void}
 */
function showMessages() {
    const overview = elGetById('modalNotificationsList').querySelector('tbody');
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
        overview.appendChild(emptyMsgEl(4, 'table'));
    }
}

/**
 * Clears the notification buffer
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearMessages(target) {
    btnWaiting(target, true);
    messages.length = 0;
    logs.length = 0;
    showMessages();
    showLogs();
    btnWaiting(target, false);
}

/**
 * Shows or hides the logbuffer
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleLogs() {
    const modalNotificationsListEl = elGetById('modalNotificationsList');
    const modalNotificationsLogsEl = elGetById('modalNotificationsLogs');
    if (modalNotificationsListEl.classList.contains('d-none')) {
        elShow(modalNotificationsListEl);
        elHide(modalNotificationsLogsEl);
        elGetById('modalNotificationsLogsBtn').classList.remove('active');
        showMessages();
    }
    else {
        elHide(modalNotificationsListEl);
        elShow(modalNotificationsLogsEl);
        elGetById('modalNotificationsLogsBtn').classList.add('active');
        showLogs();
    }
}

/**
 * Lists the logs in the modalNotificationsLogs element
 * @returns {void}
 */
function showLogs() {
    const overview = elGetById('modalNotificationsLogs').querySelector('tbody');
    elClear(overview);
    for (const log of logs) {
        overview.insertBefore(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, fmtTime(log.timestamp)),
                elCreateText('td', {}, log.prefix),
                elCreateText('td', {}, log.message)
            ]),
            overview.firstElementChild);
    }
    if (overview.querySelector('tr') === null) {
        overview.appendChild(emptyMsgEl(4, 'table'));
    }
}
