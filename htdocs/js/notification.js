"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function setStateIcon() {
    if (websocketConnected === false || settings.mpdConnected === false) {
//        domCache.mainMenu.classList.add('text-light');
//        domCache.mainMenu.classList.remove('connected');
        document.getElementById('logoBg').setAttribute('fill', '#6c757d');
    }
    else {
//        domCache.mainMenu.classList.add('connected');
//        domCache.mainMenu.classList.remove('text-light');
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

function showNotification(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (settings.notificationWeb === true) {
        let notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
        setTimeout(notification.close.bind(notification), 3000);
    } 
    if (settings.notificationPage === true || notificationType === 'danger' || notificationType === 'warning') {
        let alertBox;
        if (alertTimeout) {
            clearTimeout(alertTimeout);
        }
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
        else if (notificationType === 'warning' ) {
            toast += '<span class="material-icons text-warning mr-2">warning</span>';
        }
        else {
            toast += '<span class="material-icons text-danger mr-2">error</span>';
        }
        toast += '<strong class="mr-auto">' + e(notificationTitle) + '</strong>' +
            '<button type="button" class="ml-2 mb-1 close">&times;</button></div>';
        if (notificationHtml !== '' || notificationText !== '') {
            toast += '<div class="toast-body">' + (notificationHtml === '' ? e(notificationText) : notificationHtml) + '</div>';
        }
        toast += '</div>';
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
    setStateIcon();
    logMessage(notificationTitle, notificationText, notificationHtml, notificationType);
}

function logMessage(notificationTitle, notificationText, notificationHtml, notificationType) {
    if (notificationType === 'success') { notificationType = 'Info'; }
    else if (notificationType === 'warning') { notificationType = 'Warning'; }
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
    const topAlert = document.getElementById('top-alerts');
    if (websocketConnected === true && settings.mpdConnected === true) {
        topAlert.classList.add('hide');
        state = 'enabled';
    }
    else {
        let topPadding = 0;
        if (window.innerWidth < window.innerHeight) {
            topPadding = domCache.header.offsetHeight;
        }
        topAlert.style.paddingTop = topPadding + 'px';
        topAlert.classList.remove('hide');
    }
    let enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
        logDebug('Setting ui state to ' + state);
        setElsState('a', state, 'tag');
        setElsState('input', state, 'tag');
        setElsState('select', sate, 'tag');
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
