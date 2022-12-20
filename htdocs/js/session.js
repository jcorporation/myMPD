"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module session_js */

/**
 * Initialization function for the session elements
 */
function initSession() {
    document.getElementById('modalEnterPin').addEventListener('shown.bs.modal', function() {
        setFocusId('inputPinModal');
    }, false);

    document.getElementById('inputPinModal').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            document.getElementById('modalEnterPinEnterBtn').click();
        }
    }, false);
}

/**
 * Removes the enter pin dialog from a modal footer.
 * @param {HTMLElement} footer parent element of the enter pin dialog
 */
 function removeEnterPinFooter(footer) {
    if (footer !== undefined) {
        elShow(footer.previousElementSibling);
        footer.remove();
        return;
    }
    const f = document.querySelectorAll('.enterPinFooter');
    for (let i = f.length - 1; i >= 0; i--) {
        const prev = f[i].previousElementSibling;
        if (prev.classList.contains('modal-footer')) {
            elShow(prev);
        }
        f[i].remove();
    }
}

/**
 * Creates the enter pin footer and sends the original api request after the session is created.
 * @param {NodeList} footers modal footers to hide
 * @param {string} method jsonrpc method of the original api request
 * @param {object} params json object of the original api request
 * @param {Function} callback callback function of the original api request
 * @param {boolean} onerror true = execute callback also on error
 */
function createEnterPinFooter(footers, method, params, callback, onerror) {
    const input = elCreateEmpty('input', {"type": "password", "autocomplete": "off", "class": ["form-control", "border-secondary"]});
    const btn = elCreateTextTn('button', {"class": ["btn", "btn-success"]}, 'Enter');
    const newFooter = elCreateNode('div', {"class": ["modal-footer", "enterPinFooter"]},
        elCreateNodes('div', {"class": ["row", "w-100"]}, [
            elCreateTextTn('label', {"class": ["col-4", "col-form-label", "ps-0"]}, 'Enter pin'),
            elCreateNode('div', {"class": ["col-8", "pe-0"]},
                elCreateNodes('div', {"class": ["input-group"]}, [
                    input,
                    btn
                ])
            )
        ])
    );
    for (const footer of footers) {
        footer.classList.add('d-none');
    }
    footers[0].parentNode.appendChild(newFooter);
    setFocus(input);
    btn.addEventListener('click', function() {
        sendAPI('MYMPD_API_SESSION_LOGIN', {"pin": input.value}, function(obj) {
            input.value = '';
            const alert = footers[0].querySelector('.alert');
            if (alert !== null) {
                alert.remove();
            }
            if (obj.error) {
                newFooter.appendChild(
                    elCreateTextTn('div', {"class": ["alert", "alert-danger", "p-2", "w-100"]}, obj.error.message, obj.error.data)
                );
            }
            else if (obj.result.session !== '') {
                session.token = obj.result.session;
                session.timeout = getTimestamp() + sessionLifetime;
                setSessionState();
                removeEnterPinFooter(newFooter);
                showNotification(tn('Session successfully created'), '', 'session', 'info');
                if (method !== undefined) {
                    //call original API
                    sendAPI(method, params, callback, onerror);
                }
            }
        }, true);
    }, false);
    input.addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            btn.click();
        }
    }, false);
}

/**
 * Shows the enter pin dialog in a new model or if a modal is already opened in the footer of this modal.
 * @param {string} method jsonrpc method of the original api request
 * @param {object} params json object of the original api request
 * @param {Function} callback callback function of the original api request
 * @param {boolean} onerror true = execute callback also on error
 */
function enterPin(method, params, callback, onerror) {
    session.timeout = 0;
    setSessionState();
    const modal = getOpenModal();
    if (modal !== null) {
        logDebug('Show pin dialog in modal');
        //a modal is already opened, show enter pin dialog in footer
        const footer = modal.querySelectorAll('.modal-footer');
        createEnterPinFooter(footer, method, params, callback, onerror);
    }
    else {
        logDebug('Open pin modal');
        //open modal to enter pin and resend API request
        const enterBtn = elCreateTextTn('button', {"id": "modalEnterPinEnterBtn", "class": ["btn", "btn-success"]}, 'Enter');
        enterBtn.addEventListener('click', function() {
            sendAPI('MYMPD_API_SESSION_LOGIN', {
                "pin": document.getElementById('inputPinModal').value},
                function(obj) {
                    document.getElementById('inputPinModal').value = '';
                    if (obj.error) {
                        const em = document.getElementById('modalEnterPinMessage');
                        em.textContent = tn(obj.error.message, obj.error.data);
                        elShow(em);
                    }
                    else if (obj.result.session !== '') {
                        session.token = obj.result.session;
                        session.timeout = getTimestamp() + sessionLifetime;
                        setSessionState();
                        uiElements.modalEnterPin.hide();
                        showNotification(tn('Session successfully created'), '', 'session', 'info');
                        if (method !== undefined) {
                            //call original API
                            sendAPI(method, params, callback, onerror);
                        }
                    }
                }, true);
        }, false);
        document.getElementById('modalEnterPinEnterBtn').replaceWith(enterBtn);
        elHideId('modalEnterPinMessage');
        document.getElementById('inputPinModal').value = '';
        uiElements.modalEnterPin.show();
    }
}

/**
 * Sets the session state.
 * Shows/hides the lock indicator and the login/logout menu entry.
 */
function setSessionState() {
    if (session.timeout < getTimestamp()) {
        logDebug('Session expired: ' + session.timeout);
        session.timeout = 0;
        session.token = '';
    }
    if (settings.pin === true) {
        if (session.token === '') {
            domCache.body.classList.add('locked');
            elShowId('mmLogin');
            elHideId('mmLogout');
        }
        else {
            domCache.body.classList.remove('locked');
            elShowId('mmLogout');
            elHideId('mmLogin');
            resetSessionTimer();
        }
        elShowId('mmLoginLogoutDivider');
    }
    else {
        domCache.body.classList.remove('locked');
        elHideId('mmLogin');
        elHideId('mmLogout');
        elHideId('mmLoginLogoutDivider');
    }
}

/**
 * Resets the session timer.
 */
function resetSessionTimer() {
    if (sessionTimer !== null) {
        clearTimeout(sessionTimer);
        sessionTimer = null;
    }
    sessionTimer = setTimeout(function() {
        validateSession();
    }, sessionRenewInterval);
}

/**
 * Validates a session by calling the MYMPD_API_SESSION_VALIDATE endpoint
 * and calls setSessionState to update the DOM.
 */
function validateSession() {
    sendAPI('MYMPD_API_SESSION_VALIDATE', {}, function(obj) {
        if (obj.result !== undefined &&
            obj.result.message === 'ok')
        {
            session.timeout = getTimestamp() + sessionLifetime;
        }
        else {
            session.timeout = 0;
        }
        setSessionState();
    }, true);
}

/**
 * Removes a session by calling the MYMPD_API_SESSION_LOGOUT endpoint
 * and calls setSessionState to update the DOM.
 */
//eslint-disable-next-line no-unused-vars
function removeSession() {
    sendAPI('MYMPD_API_SESSION_LOGOUT', {}, function() {
        session.timeout = 0;
        setSessionState();
    }, false);
}
