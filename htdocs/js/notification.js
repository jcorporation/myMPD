"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module notifications_js */

/**
 * Initializes the notification html elements
 * @returns {void}
 */
function initNotifications() {
    document.getElementById('modalNotifications').addEventListener('show.bs.modal', function() {
        showMessages();
    });
}

/**
 * Sets the background color of the myMPD logo according to the websocket state
 * @returns {void}
 */
function setStateIcon() {
    const logoBgs = document.querySelectorAll('.logoBg');
    const logoFgs = document.querySelectorAll('.logoFg');
    if (getWebsocketState() === false ||
        settings.partition.mpdConnected === false)
    {
        for (const logoBg of logoBgs) {
            logoBg.setAttribute('fill', '#6c757d');
        }
        for (const logoFg of logoFgs) {
            logoFg.setAttribute('fill', '#f8f9fa;');
        }
    }
    else {
        for (const logoBg of logoBgs) {
            logoBg.setAttribute('fill', settings.partition.highlightColor);
        }
        for (const logoFg of logoFgs) {
            logoFg.setAttribute('fill', settings.partition.highlightColorContrast);
        }
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
            case 'alertMpdStatusError': {
                alertBoxEl.classList.add('alert-danger', 'top-alert-dismissible');
                const clBtn = elCreateEmpty('button', {"class": ["btn-close"]});
                alertBoxEl.appendChild(clBtn);
                clBtn.addEventListener('click', function() {
                    clearMPDerror();
                }, false);
                break;
            }
            case 'alertUpdateDBState':
            case 'alertUpdateCacheState': {
                alertBoxEl.classList.add('alert-success');
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
 * Notification severities
 * @type {object}
 */
const severities = {
    "info": {
        "text": "Info",
        "icon": "info",
        "class": "text-success"
    },
    "warn": {
        "text": "Warning",
        "icon": "warning",
        "class": "text-warning"
    },
    "error": {
        "text": "Error",
        "icon": "error",
        "class": "text-danger"
    }
};

/**
 * Notification facilities
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
 * @param {string} severity severity
 * @returns {HTMLElement} severity icon
 */
function createSeverityIcon(severity) {
    return elCreateText('span', {"data-title-phrase": severities[severity].text,
        "class": ["mi", severities[severity].class, "me-2"]}, severities[severity].icon);
}

/**
 * Shows a toast notification or an appinit alert
 * @param {string} title title of the notification 
 * @param {string} text notification text
 * @param {string} facility facility
 * @param {string} severity one off info, warn, error
 * @returns {void}
 */
function showNotification(title, text, facility, severity) {
    if (appInited === false) {
        showAppInitAlert(
            title + (text === '' ? '' : ': ' + text)
        );
        return;
    }
    setStateIcon();
    logMessage(title, text, facility, severity);
    if (severity === 'info') {
        //notifications with severity info can be hidden
        if (settings.webuiSettings.notifyPage === false &&
            settings.webuiSettings.notifyWeb === false)
        {
            return;
        }
        //disabled notification for facility in advanced setting
        let show = settings.webuiSettings['notification' + facilities[facility]];
        if (show === null ) {
            logDebug('Unknown facility: ' + facility);
            //fallback to general
            show = settings.webuiSettings['notificationGeneral'];
        }
        if (show === false) {
            return;
        }
    }

    if (settings.webuiSettings.notifyWeb === true) {
        const notification = new Notification(title, {icon: 'assets/favicon.ico', body: text});
        setTimeout(notification.close.bind(notification), 3000);
    }
    if (settings.webuiSettings.notifyPage === true) {
        const toast = elCreateNode('div', {"class": ["toast"]},
            elCreateNodes('div', {"class": ["toast-header"]}, [
                createSeverityIcon(severity),
                elCreateText('strong', {"class": ["me-auto"]}, title),
                elCreateEmpty('button', {"type": "button", "class": ["btn-close"], "data-bs-dismiss": "toast"})
            ])
        );
        if (text !== '') {
            toast.appendChild(
                elCreateText('div', {"class": ["toast-body"]}, text)
            );
        }
        document.getElementById('alertBox').prepend(toast);
        const toastInit = new BSN.Toast(toast, {delay: 2500});
        toast.addEventListener('hidden.bs.toast', function() {
            this.remove();
        }, false);
        toastInit.show();
    }
}

/**
 * Appends a log message to the log buffer
 * @param {string} title title
 * @param {string} text message
 * @param {string} facility facility
 * @param {string} severity one off info, warn, error
 * @returns {void}
 */
function logMessage(title, text, facility, severity) {
    let messagesLen = messages.length;
    const lastMessage = messagesLen > 0 ? messages[messagesLen - 1] : null;
    if (lastMessage !== null &&
        lastMessage.title === title)
    {
        lastMessage.occurrence++;
        lastMessage.timestamp = getTimestamp();
    }
    else {
        messages.push({
            "title": title,
            "text": text,
            "facility": facility,
            "severity": severity,
            "occurrence": 1,
            "timestamp": getTimestamp()
        });
        if (messagesLen >= messagesMax) {
            messages.shift();
        }
        else {
            messagesLen++;
        }
    }
    //update log overview if shown
    if (document.getElementById('modalNotifications').classList.contains('show')) {
        showMessages();
    }
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
                    elCreateText('p', {"class": ["mb-0"]}, message.title),
                    elCreateText('p', {"class": ["mb-0"]}, message.text)
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

/**
 * Checks for web notification support
 * @returns {boolean} true if web notifications are supported, else false
 */
function notificationsSupported() {
    return "Notification" in window;
}

/**
 * Toggles the disabled state of elements
 * @param {string} selector query selector
 * @param {string} state disabled or enabled
 * @returns {void}
 */
function setElsState(selector, state) {
    const els = document.querySelectorAll(selector);
    for (const el of els) {
        if (el.classList.contains('close')) {
            continue;
        }
        if (state === 'disabled') {
            if (el.classList.contains('alwaysEnabled') === false &&
                el.getAttribute('disabled') !== 'disabled')
            {
                //disable only elements that are not already disabled
                elDisable(el);
                el.classList.add('disabled');
            }
        }
        else if (el.classList.contains('disabled')) {
            //enable only elements that are disabled through this function
            elEnable(el);
            el.classList.remove('disabled');
        }
    }
}

/**
 * Toggles the ui state
 * @returns {void}
 */
function toggleUI() {
    let state = 'disabled';
    if (getWebsocketState() === true &&
        settings.partition.mpdConnected === true)
    {
        state = 'enabled';
    }
    const enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state);
        setElsState('input', state);
        setElsState('select', state);
        setElsState('button', state);
        setElsState('textarea', state);
        if (enabled === false) {
            setElsState('.clickable', state);
        }
        else {
            setElsState('.not-clickable', state);
        }
        uiEnabled = enabled;
    }

    if (settings.partition.mpdConnected === true) {
        toggleAlert('alertMpdState', false, '');
    }
    else {
        toggleAlert('alertMpdState', true, tn('MPD disconnected'));
        logMessage(tn('MPD disconnected'), '', 'mpd', 'error');
    }

    if (getWebsocketState() === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else if (appInited === true) {
        toggleAlert('alertMympdState', true, tn('Websocket is disconnected'));
        logMessage(tn('Websocket is disconnected'), '', 'general', 'error');
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
