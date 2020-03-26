"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function setStateIcon(state) {
    let stateIcon = document.getElementById('navState').children[0];
    let websocketStateIcon = document.getElementById('websocketState').children[0];
    let mpdStateIcon = document.getElementById('mpdState').children[0];
    let websocketStateText = document.getElementById('websocketState').getElementsByTagName('small')[0];
    let mpdStateText = document.getElementById('mpdState').getElementsByTagName('small')[0];
    
    if (websocketConnected === false) {
        stateIcon.innerText = 'cloud_off';
    }
    else if (settings.mpdConnected === false) {
        stateIcon.innerText = 'cloud_off';
    }
    else {
        if (state === 'newMessage') {
            stateIcon.innerText = 'chat';
        }
        else if (state === 'noMessage') {
            stateIcon.innerText = 'chat_bubble_outline';
        }
    }
    
    if (websocketConnected === false) {
        websocketStateIcon.innerText = 'cloud_off';
        websocketStateIcon.classList.remove('text-success');
        websocketStateText.innerText = t('Websocket disconnected');
    }
    else { 
        websocketStateIcon.innerText = 'cloud_done';
        websocketStateIcon.classList.add('text-success');
        websocketStateText.innerText = t('Websocket connected');
    }

    if (websocketConnected === false) { 
        mpdStateIcon.innerText = 'cloud_off';
        mpdStateIcon.classList.remove('text-success');
        mpdStateText.innerText = t('MPD disconnected');
    }
    else {
        mpdStateIcon.innerText = 'cloud_done';
        mpdStateIcon.classList.add('text-success');
        mpdStateText.innerText = t('MPD connected');
    }
}

function toggleAlert(alertBox, state, msg) {
    let mpdState = document.getElementById(alertBox);
    if (state === false) {
        mpdState.innerHTML = '';
        mpdState.classList.add('hide');
    }
    else {
        mpdState.innerHTML = msg;
        mpdState.classList.remove('hide');
    }
}

function showNotification(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (settings.notificationWeb === true) {
        let notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
        setTimeout(notification.close.bind(notification), 3000);
    } 
    if (settings.notificationPage === true || notificationType === 'danger') {
        let alertBox;
        if (!document.getElementById('alertBox')) {
            alertBox = document.createElement('div');
            alertBox.setAttribute('id', 'alertBox');
            alertBox.classList.add('toast');
        }
        else {
            alertBox = document.getElementById('alertBox');
        }
        let toast = '<div class="toast-header">';
        if (notificationType === 'success' ) {
            toast += '<span class="material-icons text-success mr-2">info</span>';
        }
        else {
            toast += '<span class="material-icons text-danger mr-2">warning</span>';
        }
        toast += '<strong class="mr-auto">' + e(notificationTitle) + '</strong>' +
            '<button type="button" class="ml-2 mb-1 close">&times;</button></div>';
        if (notificationHtml !== '' && notificationText !== '') {
            toast += '<div class="toast-body">' + (notificationHtml === '' ? e(notificationText) : notificationHtml) + '</div>';
        }
        toast += '</div>';
        alertBox.innerHTML = toast;
        
        if (!document.getElementById('alertBox')) {
            document.getElementsByTagName('main')[0].append(alertBox);
            requestAnimationFrame(function() {
                document.getElementById('alertBox').classList.add('alertBoxActive');
            });
        }
        alertBox.getElementsByTagName('button')[0].addEventListener('click', function() {
            hideNotification();
        }, false);

        if (alertTimeout) {
            clearTimeout(alertTimeout);
        }
        alertTimeout = setTimeout(function() {
            hideNotification();
        }, 3000);
    }
    setStateIcon('newMessage');
    logMessage(notificationTitle, notificationText, notificationHtml, notificationType);
}

function logMessage(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (notificationType === 'success') { notificationType = 'Info'; }
    else if (notificationType === 'danger') { notificationType = 'Error'; }
    
    let overview = document.getElementById('logOverview');

    let append = true;
    let lastEntry = overview.firstElementChild;
    if (lastEntry) {
        if (lastEntry.getAttribute('data-title') === notificationTitle) {
            append = false;        
        }
    }

    let entry = document.createElement('div');
    entry.classList.add('text-light');
    entry.setAttribute('data-title', notificationTitle);
    let occurence = 1;
    if (append === false) {
        occurence += parseInt(lastEntry.getAttribute('data-occurence'));
    }
    entry.setAttribute('data-occurence', occurence);
    entry.innerHTML = '<small>' + localeDate() + '&nbsp;&ndash;&nbsp;' + t(notificationType) +
        (occurence > 1 ? '&nbsp;(' + occurence + ')' : '') + '</small>' +
        '<p>' + e(notificationTitle) +
        (notificationHtml === '' && notificationText === '' ? '' :
        '<br/>' + (notificationHtml === '' ? e(notificationText) : notificationHtml)) +
        '</p>';

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

    document.getElementById('navState').children[0].classList.add('text-success');
    setTimeout(function() {
        document.getElementById('navState').children[0].classList.remove('text-success');
    }, 250);
}

//eslint-disable-next-line no-unused-vars
function clearLogOverview() {
    let overviewEls = document.getElementById('logOverview').getElementsByTagName('div');
    for (let i = overviewEls.length - 1; i >= 0; i--) {
        overviewEls[i].remove();
    }
    setStateIcon('noMessage');
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

function setElsState(tag, state) {
    let els = document.getElementsByTagName(tag);
    let elsLen = els.length;
    for (let i = 0; i < elsLen; i++) {
        if (state === 'disabled') {
            if (els[i].classList.contains('alwaysEnabled') === false) {
                if (els[i].getAttribute('disabled') === null) {
                    els[i].setAttribute('disabled', 'disabled');
                    els[i].classList.add('disabled');
                }
            }
        }
        else {
            if (els[i].classList.contains('disabled')) {
                els[i].removeAttribute('disabled');
                els[i].classList.remove('disabled');
            }
        }
    }
}

function toggleUI() {
    let state = 'disabled';
    if (websocketConnected === true && settings.mpdConnected === true) {
        state = 'enabled';
    }
    let enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state);
        setElsState('input', state);
        setElsState('button', state);
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
    else {
        toggleAlert('alertMympdState', true, t('Websocket is disconnected'));
        logMessage(t('Websocket is disconnected'), '', '', 'danger');
    }
    setStateIcon();
}
