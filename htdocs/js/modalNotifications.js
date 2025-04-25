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
        if (elGetById('modalNotificationsMessagesTab').classList.contains('active')) {
            showMessages();
        }
        else {
            showLogs();
        }
    });
    elGetById('modalNotificationsMessagesTab').addEventListener('show.bs.tab', function() {
        showMessages();
    });
    elGetById('modalNotificationsLogsTab').addEventListener('show.bs.tab', function() {
        showLogs();
    });
    const modalNotificationsSeveritySelectEl = elGetById('modalNotificationsSeveritySelect');
    for (const severity in severities) {
        const opt = elCreateTextTn('option', {'value': severities[severity].severity}, severity);
        if (severities[severity].severity === settings.loglevel) {
            opt.setAttribute('selected', 'selected');
        }
        modalNotificationsSeveritySelectEl.appendChild(opt);
    }
    elGetById('modalNotificationsSeveritySelect').addEventListener('change', function() {
        if (elGetById('modalNotificationsMessagesTab').classList.contains('active')) {
            showMessages();
        }
        else {
            showLogs();
        }
    }, false);
}

/**
 * Lists the messages in the modalNotificationsList element
 * @returns {void}
 */
function showMessages() {
    const overview = elGetById('modalNotificationsMessagesList').querySelector('tbody');
    elClear(overview);
    const loglevel = getSelectValueId('modalNotificationsSeveritySelect');
    for (const message of messages) {
        if (message.severity > loglevel) {
            continue;
        }
        overview.insertBefore(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, fmtTime(message.timestamp)),
                elCreateNodes('td', {}, [
                    createSeverityIconList(message.severity),
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
 * Lists the logs in the modalNotificationsLogs element
 * @returns {void}
 */
function showLogs() {
    const overview = elGetById('modalNotificationsLogsList').querySelector('tbody');
    elClear(overview);
    const loglevel = getSelectValueId('modalNotificationsSeveritySelect');
    for (const log of logs) {
        if (log.severity > loglevel) {
            continue;
        }
        overview.insertBefore(
            elCreateNodes('tr', {}, [
                elCreateText('td', {}, fmtTime(log.timestamp)),
                elCreateText('td', {}, tn(severityNames[log.severity])),
                elCreateText('td', {}, log.message)
            ]),
            overview.firstElementChild);
    }
    if (overview.querySelector('tr') === null) {
        overview.appendChild(emptyMsgEl(3, 'table'));
    }
}

/**
 * Clears the notification or log buffer
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function clearMessages(target) {
    btnWaiting(target, true);
    if (elGetById('modalNotificationsMessagesTab').classList.contains('active')) {
        messages.length = 0;
        showMessages();
    }
    else {
        logs.length = 0;
        showLogs();
    }
    btnWaiting(target, false);
}

/**
 * Refreshes the notification or log overview
 * @param {Node} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function refreshMessages(target) {
    btnWaiting(target, true);
    if (elGetById('modalNotificationsMessagesTab').classList.contains('active')) {
        showMessages();
    }
    else {
        showLogs();
    }
    btnWaiting(target, false);
}
