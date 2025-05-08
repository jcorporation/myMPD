"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module notifications_js */

/**
 * Sets the background color of the myMPD logo according to the websocket state
 * @returns {void}
 */
function setStateIcon() {
    const fgColor = uiEnabled === true
        ? settings.partition.highlightColorContrast
        : '#f8f9fa;';
    const bgColor = uiEnabled === true
        ? settings.partition.highlightColor
        : '#6c757d';

    const logoFgs = document.querySelectorAll('.logoFg');
    for (const logoFg of logoFgs) {
        logoFg.setAttribute('fill', fgColor);
    }
    const logoBgs = document.querySelectorAll('.logoBg');
    for (const logoBg of logoBgs) {
        logoBg.setAttribute('fill', bgColor);
    }
}

/**
 * Toggles and sets/clears the message of the top alertBox
 * @param {string} alertBoxId the id of alertBox to toggle
 * @param {boolean} state false = hides the alertBox, true = shows the alertBox
 * @param {string} msg already translated message
 * @returns {void}
 */
function toggleAlert(alertBoxId, state, msg) {
    //get existing alert
    const topAlert = document.querySelector('#top-alerts');
    let alertBoxEl = topAlert.querySelector('#' + alertBoxId);
    if (state === false) {
        //remove alert
        if (alertBoxEl !== null) {
            alertBoxEl.remove();
        }
    }
    else if (alertBoxEl === null) {
        //create new alert
        alertBoxEl = elCreateNode('div', {"id": alertBoxId, "class": ["alert", "top-alert", "d-flex", "flex-row"]},
            elCreateText('span', {}, msg)
        );
        switch(alertBoxId) {
            case 'alertCrit': {
                alertBoxEl.classList.add('alert-danger', 'top-alert-dismissible');
                const clBtn = elCreateEmpty('button', {"class": ["btn-close"]});
                alertBoxEl.appendChild(clBtn);
                clBtn.addEventListener('click', function(event) {
                    event.preventDefault();
                    toggleAlert('alertCrit', false, '');
                }, false);
                break;
            }
            case 'alertMpdStatusError': {
                alertBoxEl.classList.add('alert-danger', 'top-alert-dismissible');
                const clBtn = elCreateEmpty('button', {"class": ["btn-close"]});
                alertBoxEl.appendChild(clBtn);
                clBtn.addEventListener('click', function(event) {
                    event.preventDefault();
                    clearMPDerror();
                }, false);
                break;
            }
            case 'alertJukeboxStatusError': {
                alertBoxEl.classList.add('alert-danger', 'top-alert-dismissible');
                const clBtn = elCreateEmpty('button', {"class": ["btn-close"]});
                alertBoxEl.appendChild(clBtn);
                clBtn.addEventListener('click', function(event) {
                    event.preventDefault();
                    clearJukeboxError();
                }, false);
                break;
            }
            case 'alertUpdateDBState':
            case 'alertUpdateCacheState': {
                alertBoxEl.classList.add('alert-success');
                break;
            }
            case 'alertMympdState': {
                alertBoxEl.classList.add('alert-danger', 'top-alert-dismissible');
                const clBtn = elCreateText('button', {"class": ["alwaysEnabled", "btn-retry", "mi"], "title": tn('Reconnect'), "data-title-phrase": "Reconnect"}, "refresh");
                alertBoxEl.appendChild(clBtn);
                clBtn.addEventListener('click', function(event) {
                    event.preventDefault();
                    onShow('reconnect');
                }, false);
                break;
            }
            default:
                alertBoxEl.classList.add('alert-danger');
        }
        topAlert.appendChild(alertBoxEl);
    }
    else {
        //replace the message
        alertBoxEl.firstElementChild.textContent = msg;
    }

    //check if we should show the alert container
    if (topAlert.childElementCount > 0) {
        elShow(topAlert);
        domCache.main.style.marginTop = topAlert.offsetHeight + 'px';
    }
    else {
        domCache.main.style.marginTop = '0';
        elHide(topAlert);
    }
}

/**
 * Jsonrpc notification facilities
 * @type {object}
 */
const facilities = {
    "database": "Database",
    "general":  "General",
    "home":     "Home",
    "jukebox":  "Jukebox",
    "lyrics":   "Lyrics",
    "mpd":      "MPD",
    "playlist": "Playlist",
    "player":   "Player",
    "queue":    "Queue",
    "session":  "Session",
    "script":   "Script",
    "sticker":  "Sticker",
    "timer":    "Timer",
    "trigger":  "Trigger"
};

/**
 * Creates a severity icon
 * @param {number} severity Syslog severity number
 * @returns {HTMLElement} Severity icon
 */
function createSeverityIcon(severity) {
    const severityName = severityNames[severity];
    return elCreateText('span', {"data-title-phrase": severityName,
        "class": ["mi", "text-light", "px-3"]}, severities[severityName].icon);
}

/**
 * Creates a severity icon
 * @param {number} severity Syslog severity number
 * @returns {HTMLElement} Severity icon
 */
function createSeverityIconList(severity) {
    const severityName = severityNames[severity];
    return elCreateText('span', {"data-title-phrase": severityName,
        "class": ["mi", severities[severityName].class, "me-2"]}, severities[severityName].icon);
}

/**
 * Shows a toast notification or an appinit alert
 * @param {string} message Message - already translated
 * @param {string} facility Facility
 * @param {string} severityName Syslog severity name
 * @returns {void}
 */
function showNotification(message, facility, severityName) {
    if (appInited === false) {
        showAppInitAlert(message);
        return;
    }
    const severity = severities[severityName].severity;
    logNotification(message, facility, severity);
    if (severity === 7) {
        // Debug notifications are only logged
        return;
    }
    if (severity > 4) {
        // Notifications with severity info and notice can be hidden
        if (settings.webuiSettings.notifyPage === false &&
            settings.webuiSettings.notifyWeb === false)
        {
            return;
        }
        // Disabled notification for facility in advanced setting
        let show = settings.webuiSettings['notification' + facilities[facility]];
        if (show === null ) {
            logDebug('Unknown facility: ' + facility);
            // Fallback to general
            show = settings.webuiSettings['notificationGeneral'];
        }
        if (show === false) {
            return;
        }
    }

    if (facility === 'jukebox' &&
        severity < 5)
    {
        toggleAlert('alertJukeboxStatusError', true, message);
        return;
    }

    if (settings.webuiSettings.notifyWeb === true) {
        const notification = new Notification(message, {icon: 'assets/favicon.ico'});
        setTimeout(notification.close.bind(notification), 3000);
    }
    if (settings.webuiSettings.notifyPage === true) {
        const toast = elCreateNodes('div', {"class": ["toast", "mt-2"]}, [
            elCreateNodes('div', {"class": ["toast-header", "p-0", severities[severityName].bgclass, "rounded"]}, [
                createSeverityIcon(severity),
                elCreateText('span', {"class": ["p-2", "ps-3", "bg-dark", "w-100", "rounded-end"]}, message)
            ])
        ]);
        elGetById('alertBox').prepend(toast);
        const toastInit = new BSN.Toast(toast, {delay: severities[severityName].delay});
        toast.addEventListener('hidden.bs.toast', function() {
            this.remove();
        }, false);
        toastInit.show();
    }
    if (severity < 3) {
        // Display critical notifications also on the top of the page
        toggleAlert('alertCrit', true, message);
    }
}

/**
 * Appends a message to the notification buffer
 * @param {string} message Message to log - already translated
 * @param {string} facility Jsonrpc facility
 * @param {number} severity Syslog severity number
 * @returns {void}
 */
function logNotification(message, facility, severity) {
    const messagesLen = messages.length;
    const lastMessage = messagesLen > 0
        ? messages[messagesLen - 1]
        : null;
    if (lastMessage !== null &&
        lastMessage.message === message)
    {
        lastMessage.occurrence++;
        lastMessage.timestamp = getTimestamp();
    }
    else {
        messages.push({
            "message": message,
            "facility": facility,
            "severity": severity,
            "occurrence": 1,
            "timestamp": getTimestamp()
        });
        if (messagesLen > messagesMax) {
            messages.shift();
        }
    }
    //update notification messages overview if shown
    if (elGetById('modalNotifications').classList.contains('show')) {
        showMessages();
    }
}

/**
 * Checks for web notification support
 * @returns {boolean} true if web notifications are supported, else false
 */
function notificationsSupported() {
    return "Notification" in window;
}

/**
 * Toggles the ui state
 * @returns {void}
 */
function toggleUI() {
    /** @type {string} */
    const state = getWebsocketState() &&
        settings.partition.mpdConnected &&
        myMPDready
            ? 'enabled'
            : 'disabled';
    /** @type {boolean} */
    const enabled = state === 'disabled'
        ? false
        : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        domCache.body.setAttribute('data-uiState', state);
        //remember current state
        uiEnabled = enabled;
    }

    if (myMPDready === false) {
        toggleAlert('alertMympdNotReady', true, tn('myMPD not yet ready'));
    }
    else {
        toggleAlert('alertMympdNotReady', false, '');
    }

    if (settings.partition.mpdConnected === true) {
        toggleAlert('alertMpdState', false, '');
    }
    else {
        toggleAlert('alertMpdState', true, tn('MPD disconnected'));
        logNotification(tn('MPD disconnected'), 'mpd', 3);
    }

    if (getWebsocketState() === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else if (appInited === true) {
        toggleAlert('alertMympdState', true, tn('Disconnected from myMPD'));
        logNotification(tn('Websocket is disconnected'), 'general', 3);
    }

    setStateIcon();
}

/**
 * Shows an alert in a modal
 * @param {object} obj jsonrpc error response
 * @returns {void}
 */
function showModalAlert(obj) {
    const aModal = getOpenModal();
    const activeAlert = aModal.querySelector('.modalAlert');
    const div = elCreateTextTn('div', {"class": ["alert", "alert-danger", "modalAlert"]}, obj.error.message, obj.error.data);
    if (activeAlert === null) {
        aModal.querySelector('.modal-body').appendChild(div);
    }
    else {
        aModal.querySelector('.modal-body').replaceChild(div, activeAlert);
    }
}

/**
 * Shows an info in a modal
 * @param {string} message message to display
 * @returns {void}
 */
function showModalInfo(message) {
    const aModal = getOpenModal();
    const activeAlert = aModal.querySelector('.modalAlert');
    const div = elCreateTextTn('div', {"class": ["alert", "alert-success", "modalAlert"]}, message);
    if (activeAlert === null) {
        aModal.querySelector('.modal-body').appendChild(div);
    }
    else {
        aModal.querySelector('.modal-body').replaceChild(div, activeAlert);
    }
}

/**
 * Removes all alerts in a modal
 * @param {HTMLElement | Element} el the modal element
 * @returns {void}
 */
function hideModalAlert(el) {
    const activeAlerts = el.querySelectorAll('.modalAlert');
    for (let i = 0, j = activeAlerts.length; i < j; i++) {
        activeAlerts[i].remove();
    }
}

/**
 * Hides a dismissible alert
 * @param {EventTarget} target close button of the alert
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function hideAlert(target) {
    elHide(target.parentNode);
}
