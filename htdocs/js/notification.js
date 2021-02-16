"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function setStateIcon() {
    if (websocketConnected === false || settings.mpdConnected === false) {
        document.getElementById('logoBg').setAttribute('fill', '#6c757d');
    }
    else {
        document.getElementById('logoBg').setAttribute('fill', settings.highlightColor);
    }
}

function toggleAlert(alertBox, state, msg) {
    const alertBoxEl = document.getElementById(alertBox);
    if (state === false) {
        alertBoxEl.innerHTML = '';
        alertBoxEl.classList.add('hide');
    }
    else {
        alertBoxEl.innerHTML = msg;
        alertBoxEl.classList.remove('hide');
    }
}

//severities: info, warn, error
//facilities: player, queue, general, database, playlist

function showNotification(title, text, facility, severity) {
    setStateIcon();
    logMessage(title, text, facility, severity);
    
    if (settings.notificationWeb === true) {
        let notification = new Notification(title, {icon: 'assets/favicon.ico', body: text});
        setTimeout(notification.close.bind(notification), 3000);
    }
    
    if (severity === 'info') {
        //notifications with severity info can be hidden
        if (settings.notificationPage === false) { return; }
        if (facility === 'player' && settings.advanced.notificationPlayer === false) { return; }
        if (facility === 'queue' && settings.advanced.notificationQueue === false) { return; }
        if (facility === 'general' && settings.advanced.notificationGeneral === false) { return; }
        if (facility === 'database' && settings.advanced.notificationDatabase === false) { return; }
        if (facility === 'playlist' && settings.advanced.notificationPlaylist === false) { return; }
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
            let ab = document.getElementById('alertBox');
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
    if (severity === 'info') { severity = 'Info'; }
    else if (severity === 'warn') { severity = 'Warning'; }
    else if (severity === 'error') { severity = 'Error'; }
    else { 
        logDebug('Unknown severity: ' + severity);
    }
    
    if (facility === 'player') { facility = 'Player'; }
    else if (facility === 'queue') { facility = 'Queue'; }
    else if (facility === 'general') { facility = 'General'; }
    else if (facility === 'database') { facility = 'Database'; }
    else if (facility === 'playlist') { facility = 'Playlist'; }
    else { 
        logDebug('Unknown facility: ' + facility);
    }
    
    let overview = document.getElementById('logOverview');

    let append = true;
    let lastEntry = overview.firstElementChild;
    if (lastEntry) {
        if (getAttDec(lastEntry, 'data-title') === title) {
            append = false;        
        }
    }

    let entry = document.createElement('div');
    entry.classList.add('text-light');
    setAttEnc(entry, 'data-title', title);
    let occurence = 1;
    if (append === false) {
        occurence += parseInt(getAttDec(lastEntry, 'data-occurence'));
    }
    setAttEnc(entry, 'data-occurence', occurence);
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
   
    let overviewEls = overview.getElementsByTagName('div');
    if (overviewEls.length > 10) {
        overviewEls[10].remove();
    }
}

//eslint-disable-next-line no-unused-vars
function clearLogOverview() {
    let overviewEls = document.getElementById('logOverview').getElementsByTagName('div');
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
            let alertBox = document.getElementById('alertBox');
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
    let els = type === 'tag' ? document.getElementsByTagName(tag) : document.getElementsByClassName(tag);
    let elsLen = els.length;
    for (let i = 0; i < elsLen; i++) {
        if (els[i].classList.contains('close')) {
            continue;
        }
        if (state === 'disabled') {
            if (els[i].classList.contains('alwaysEnabled') === false) {
                if (els[i].getAttribute('disabled') === null) {
                    disableEl(els[i]);
                    els[i].classList.add('disabled');
                }
            }
        }
        else {
            if (els[i].classList.contains('disabled')) {
                enableEl(els[i]);
                els[i].classList.remove('disabled');
            }
        }
    }
}

function toggleUI() {
    let state = 'disabled';
    const topAlert = document.getElementById('top-alerts');
    if (websocketConnected === true && settings.mpdConnected === true) {
        topAlert.classList.add('hide');
        state = 'enabled';
    }
    else {
        let topPadding = 0;
        if (window.innerWidth < window.innerHeight) {
            topPadding = document.getElementById('header').offsetHeight;
        }
        topAlert.style.paddingTop = topPadding + 'px';
        topAlert.classList.remove('hide');
    }
    let enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state, 'tag');
        setElsState('input', state, 'tag');
        setElsState('select', state, 'tag');
        setElsState('button', state, 'tag');
        setElsState('clickable', state, 'class');
        uiEnabled = enabled;
    }

    if (settings.mpdConnected === true) {
        toggleAlert('alertMpdState', false, '');
    }
    else {
        toggleAlert('alertMpdState', true, t('MPD disconnected'));
        logMessage(t('MPD disconnected'), '', '', 'danger');
    }

    if (websocketConnected === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else if (appInited === true) {
        toggleAlert('alertMympdState', true, t('Websocket is disconnected'));
        logMessage(t('Websocket is disconnected'), '', '', 'danger');
    }
 
    setStateIcon();
}
