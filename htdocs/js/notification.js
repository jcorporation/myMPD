"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function setStateIcon() {
    if (websocketConnected === false ||
        settings.partition.mpdConnected === false)
    {
        document.getElementById('logoBg').setAttribute('fill', '#6c757d');
    }
    else {
        document.getElementById('logoBg').setAttribute('fill', settings.partition.highlightColor);
    }
}

function toggleAlert(alertBox, state, msg) {
    const alertBoxEl = document.getElementById(alertBox);
    if (state === false) {
        elHide(alertBoxEl);
        elClear(alertBoxEl);
        return;
    }

    alertBoxEl.textContent = msg;
    if (alertBox === 'alertMpdStatusError') {
        const clBtn = elCreateEmpty('button', {"class": ["btn-close", "btn-close-alert"]});
        alertBoxEl.appendChild(clBtn);
        clBtn.addEventListener('click', function() {
            clearMPDerror();
        }, false);
    }
    elShow(alertBoxEl);
}

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

function getSeverityIcon(severity) {
    return elCreateText('span', {"title": tn(severities[severity].text),
        "class": ["mi", severities[severity].class, "me-2"]}, severities[severity].icon);
}

function showNotification(title, text, facility, severity) {
    if (appInited === false) {
        showAppInitAlert(title);
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
                getSeverityIcon(severity),
                elCreateText('strong', {"class": ["me-auto"]}, title),
                elCreateEmpty('button', {"type": "button", "class": ["btn-close"], "data-bs-dismiss": "toast"})
            ])
        );
        if (text !== '') {
            toast.appendChild(elCreateText('div', {"class": ["toast-body"]}, text));
        }
        document.getElementById('alertBox').prepend(toast);
        const toastInit = new BSN.Toast(toast, {delay: 2500});
        toast.addEventListener('hidden.bs.toast', function() {
            this.remove();
        }, false);
        toastInit.show();
    }
}

function logMessage(title, text, facility, severity) {
    let messagesLen = messages.length;
    const lastMessage = messagesLen > 0 ? messages[messagesLen - 1] : null;
    if (lastMessage &&
        lastMessage.title === title)
    {
        lastMessage.occurence++;
        lastMessage.timestamp = getTimestamp();
    }
    else {
        messages.push({
            "title": title,
            "text": text,
            "facility": facility,
            "severity": severity,
            "occurence": 1,
            "timestamp": getTimestamp()
        });
        messagesLen++;
    }
    if (messagesLen > 10) {
        messages.shift();
        messagesLen = 10;
    }
    domCache.notificationCount.textContent = messagesLen;
}

function showMessages() {
    const overview = document.getElementById('logOverview');
    elClear(overview);
    for (const message of messages) {
        const entry = elCreateEmpty('div', {"class": ["row", "align-items-center", "mb-2", "me-0"]});
        entry.appendChild(elCreateNode('div', {"class": ["col", "col-1", "ps-0"]}, getSeverityIcon(message.severity)));
        const col = elCreateEmpty('div', {"class": ["col", "col-11"]});
        col.appendChild(elCreateText('small', {"class": ["me-2"]}, localeDate(message.timestamp) +
            smallSpace + nDash + smallSpace + tn(facilities[message.facility])));
        if (message.occurence > 1) {
            col.appendChild(elCreateText('div', {"class": ["badge", "bg-secondary"]}, message.occurence));
        }
        col.appendChild(elCreateText('p', {"class": ["mb-0"]}, message.title));
        if (message.text !== '') {
            col.appendChild(elCreateText('p', {"class": ["mb-0"]}, message.text));
        }
        entry.appendChild(col);
        overview.insertBefore(entry, overview.firstElementChild);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

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

function toggleUI() {
    let state = 'disabled';
    if (websocketConnected === true &&
        settings.partition.mpdConnected === true)
    {
        state = 'enabled';
    }
    const enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state, 'tag');
        setElsState('input', state, 'tag');
        setElsState('select', state, 'tag');
        setElsState('button', state, 'tag');
        setElsState('textarea', state, 'tag');
        if (enabled === false) {
            setElsState('.clickable', state, 'class');
        }
        else {
            setElsState('.not-clickable', state, 'class');
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

    if (websocketConnected === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else if (appInited === true) {
        toggleAlert('alertMympdState', true, tn('Websocket is disconnected'));
        logMessage(tn('Websocket is disconnected'), '', 'general', 'error');
    }

    toggleTopAlert();
    setStateIcon();
}

function toggleTopAlert() {
    const topAlert = document.getElementById('top-alerts');
    if (uiEnabled === false ||
        (currentState !== undefined && currentState.lastError !== '')
    ) {
        elShow(topAlert);
        const topPadding = window.innerWidth < window.innerHeight ? document.getElementById('header').offsetHeight : 0;
        const mt = topAlert.offsetHeight - topPadding;
        document.getElementsByTagName('main')[0].style.marginTop = mt + 'px';
    }
    else {
        document.getElementsByTagName('main')[0].style.marginTop = 0;
        elHide(topAlert);
    }
}

function showModalAlert(obj) {
    const aModal = getOpenModal();
    const activeAlert = aModal.getElementsByClassName('modalAlert')[0];
    const div = elCreateText('div', {"class": ["alert", "alert-danger", "modalAlert"]}, tn(obj.error.message, obj.error.data));
    if (activeAlert === undefined) {
        aModal.getElementsByClassName('modal-body')[0].appendChild(div);
    }
    else {
        aModal.getElementsByClassName('modal-body')[0].replaceChild(div, activeAlert);
    }
}

function hideModalAlert(el) {
    const activeAlerts = el.getElementsByClassName('modalAlert');
    for (let i = activeAlerts.length - 1; i >= 0; i--) {
        activeAlerts[i].remove();
    }
}
