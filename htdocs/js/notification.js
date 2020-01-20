"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
    if (settings.notificationWeb == true) {
        let notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
        setTimeout(function(notification) {
            notification.close();
        }, 3000, notification);
    } 
    if (settings.notificationPage === true) {
        let alertBox;
        if (!document.getElementById('alertBox')) {
            alertBox = document.createElement('div');
            alertBox.setAttribute('id', 'alertBox');
            alertBox.addEventListener('click', function() {
                hideNotification();
            }, false);
        }
        else {
            alertBox = document.getElementById('alertBox');
        }
        alertBox.classList.remove('alert-success', 'alert-danger');
        alertBox.classList.add('alert','alert-' + notificationType);
        alertBox.innerHTML = '<div><strong>' + e(notificationTitle) + '</strong><br/>' + (notificationHtml == '' ? e(notificationText) : notificationHtml) + '</div>';
        document.getElementsByTagName('main')[0].append(alertBox);
        document.getElementById('alertBox').classList.add('alertBoxActive');
        if (alertTimeout) {
            clearTimeout(alertTimeout);
        }
        alertTimeout = setTimeout(function() {
            hideNotification();
        }, 3000);
    }
}

function hideNotification() {
    if (document.getElementById('alertBox')) {
        document.getElementById('alertBox').classList.remove('alertBoxActive');
        setTimeout(function() {
            let alertBox = document.getElementById('alertBox');
            if (alertBox)
                alertBox.remove();
        }, 600);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function setElsState(tag, state) {
    let els = document.getElementsByTagName(tag);
    let elsLen = els.length;
    for (let i = 0; i< elsLen; i++) {
        if (state == 'disabled') {
            if (!els[i].classList.contains('alwaysEnabled')) {
                if (els[i].getAttribute('disabled')) {
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
    if (websocketConnected == true && settings.mpdConnected == true) {
        state = 'enabled';
    }
    let enabled = state === 'disabled' ? false : true;
    if (enabled !== uiEnabled) {
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
    }
    if (websocketConnected === true) {
        toggleAlert('alertMympdState', false, '');
    }
    else {
        toggleAlert('alertMympdState', true, t('Websocket connection failed'));
    }
}
