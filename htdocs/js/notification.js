"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function setStateIcon() {
    if (websocketConnected === false || settings.mpdConnected === false) {
        document.getElementById('logoBg').setAttribute('fill', '#6c757d');
    }
    else {
        document.getElementById('logoBg').setAttribute('fill', settings.webuiSettings.uiHighlightColor);
    }
}

function toggleAlert(alertBox, state, msg) {
    const alertBoxEl = document.getElementById(alertBox);
    if (state === false) {
        elHide(alertBoxEl);
        elClear(alertBoxEl);
    }
    else {
        elClear(alertBoxEl);
        addIconLine(alertBoxEl, 'error', msg);
        if (alertBox === 'alertMpdStatusError') {
            const clBtn = elCreate('button', {"class": ["close"]}, '×');
            alertBoxEl.appendChild(clBtn);
            clBtn.addEventListener('click', function() {
                clearMPDerror();
            }, false);
        }
        elShow(alertBoxEl);
    }
}

const severities = {
    "info": "Info",
    "warn": "Warning",
    "error": "Error"
};

const facilities = { 
    "player": "Player",
    "queue": "Queue",
    "general": "General",
    "database": "Database",
    "playlist": "Playlist",
    "mpd": "MPD",
    "lyrics": "Lyrics",
    "jukebox": "Jukebox",
    "trigger": "Trigger",
    "script": "Script",
    "sticker": "Sticker",
    "home": "Home",
    "timer": "Timer"
};

function showNotification(title, text, facility, severity) {
    setStateIcon();
    logMessage(title, text, facility, severity);
    
    if (settings.notificationWeb === true) {
        const notification = new Notification(title, {icon: 'assets/favicon.ico', body: text});
        setTimeout(notification.close.bind(notification), 3000);
    }
    
    if (severity === 'info') {
        //notifications with severity info can be hidden
        if (settings.notificationPage === false) { 
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
        
    if (alertTimeout) {
        clearTimeout(alertTimeout);
    }
    let alertBox = document.getElementById('alertBox');
    if (alertBox === null) {
        alertBox = document.createElement('div');
        alertBox.setAttribute('id', 'alertBox');
        alertBox.classList.add('toast');
    }
        
    let toast = '<div class="toast-header">';
    if (severity === 'info' ) {
        toast += '<span class="mi text-success mr-2">info</span>';
    }
    else if (severity === 'warn' ) {
        toast += '<span class="mi text-warning mr-2">warning</span>';
    }
    else {
        toast += '<span class="mi text-danger mr-2">error</span>';
    }
    toast += '<strong class="mr-auto">' + e(title) + '</strong>' +
             '<button type="button" class="ml-2 mb-1 close">&times;</button></div>' +
             (text === '' ? '' : '<div class="toast-body">' + e(text) + '</div>') +
             '</div>';
    alertBox.innerHTML = toast;
        
    if (!document.getElementById('alertBox')) {
        document.getElementsByTagName('main')[0].append(alertBox);
        requestAnimationFrame(function() {
            const ab = document.getElementById('alertBox');
            if (ab) {
                ab.classList.add('alertBoxActive');
            }
        });
    }
    alertBox.getElementsByTagName('button')[0].addEventListener('click', function() {
        hideNotification();
    }, false);

    alertTimeout = setTimeout(function() {
        hideNotification();
    }, 3000);
}

function logMessage(title, text, facility, severity) {
    if (severities[severity] !== undefined) {
        severity = severities[severity];
    }
    else { 
        logDebug('Unknown severity: ' + severity);
    }
    
    if (facilities[facility] !== undefined) {
        facility = facilities[facility];
    }
    else { 
        logDebug('Unknown facility: ' + facility);
    }
    
    const overview = document.getElementById('logOverview');

    let append = true;
    const lastEntry = overview.firstElementChild;
    if (lastEntry && getCustomDomProperty(lastEntry, 'data-title') === title) {
        append = false;        
    }

    const entry = document.createElement('div');
    entry.classList.add('text-light');
    setCustomDomProperty(entry, 'data-title', title);
    let occurence = 1;
    if (append === false) {
        occurence += Number(getCustomDomProperty(lastEntry, 'data-occurence'));
    }
    setCustomDomProperty(entry, 'data-occurence', occurence);
    entry.innerHTML = '<small>' + localeDate() + '&nbsp;&ndash;&nbsp;' + t(facility) +
        ':&nbsp;' + t(severity) +
        (occurence > 1 ? '&nbsp;(' + occurence + ')' : '') + '</small>' +
        '<p>' + e(title) + (text === '' ? '' : '<br/>' + e(text)) + '</p>';

    if (append === true) {
        overview.insertBefore(entry, overview.firstElementChild);
    }
    else {
        overview.replaceChild(entry, lastEntry);
    }
   
    const overviewEls = overview.getElementsByTagName('div');
    if (overviewEls.length > 10) {
        overviewEls[10].remove();
    }
}

//eslint-disable-next-line no-unused-vars
function clearLogOverview() {
    const overviewEls = document.getElementById('logOverview').getElementsByTagName('div');
    for (let i = overviewEls.length - 1; i >= 0; i--) {
        overviewEls[i].remove();
    }
    setStateIcon();
}

function hideNotification() {
    if (alertTimeout) {
        clearTimeout(alertTimeout);
    }

    if (document.getElementById('alertBox')) {
        document.getElementById('alertBox').classList.remove('alertBoxActive');
        setTimeout(function() {
            const alertBox = document.getElementById('alertBox');
            if (alertBox) {
                alertBox.remove();
            }
        }, 750);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function setElsState(tag, state, type) {
    const els = type === 'tag' ? document.getElementsByTagName(tag) : document.getElementsByClassName(tag);
    for (const el of els) {
        if (el.classList.contains('close')) {
            continue;
        }
        if (state === 'disabled') {
            if (el.classList.contains('alwaysEnabled') === false && el.getAttribute('disabled') === null) {
                elDisable(el);
                el.classList.add('disabled');
            }
        }
        else if (el.classList.contains('disabled')) {
            elEnable(el);
            el.classList.remove('disabled');
        }
    }
}

function toggleUI() {
    let state = 'disabled';
    if (websocketConnected === true && settings.mpdConnected === true) {
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
        setElsState('clickable', state, 'class');
        uiEnabled = enabled;
    }

    if (settings.mpdConnected === true) {
        toggleAlert('alertMpdState', false, '');
    }
    else {
        toggleAlert('alertMpdState', true, tn('MPD disconnected'));
        logMessage(t('MPD disconnected'), '', 'mpd', 'error');
    }

    if (websocketConnected === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else if (appInited === true) {
        toggleAlert('alertMympdState', true, tn('Websocket is disconnected'));
        logMessage(t('Websocket is disconnected'), '', 'general', 'error');
    }

    toggleTopAlert();
    setStateIcon();
}

function toggleTopAlert() {
    const topAlert = document.getElementById('top-alerts');
    if (uiEnabled === false || (lastState !== undefined && lastState.lastError !== '')) {
        let topPadding = 0;
        if (window.innerWidth < window.innerHeight) {
            topPadding = document.getElementById('header').offsetHeight;
        }
        topAlert.style.paddingTop = topPadding + 'px';
        topAlert.classList.remove('hide');
        const mt = topAlert.offsetHeight - parseInt(topAlert.style.paddingTop);
        document.getElementsByTagName('main')[0].style.marginTop = mt + 'px';
    }
    else {
        document.getElementsByTagName('main')[0].style.marginTop = 0;
        topAlert.classList.add('hide');
    }
}
