"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module session_js */

/**
 * Shows the login modal or logs out
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function loginOrLogout() {
    if (session.token === '') {
        enterPin(undefined, undefined, undefined, false);
    }
    else {
        removeSession();
    }
}

/**
 * Removes the enter pin dialog from a modal footer and restores the original footer.
 * @param {HTMLElement} footer pin footer to remove
 * @returns {void}
 */
 function removeEnterPinFooter(footer) {
    const modal = getOpenModal();
    if (modal === null) {
        return;
    }
    if (footer === undefined) {
        footer = modal.querySelector('.enterPinFooter');
    }

    const prevId = footer.getAttribute('data-footer');
    const prevFooter = prevId === null
        ? footer.previousElementSibling
        : modal.querySelector('#' + prevId);

    elShow(prevFooter);
    footer.remove();
}

/**
 * Creates the enter pin footer and sends the original api request after the session is created.
 * @param {NodeList} footers modal footers to hide
 * @param {string} method jsonrpc method of the original api request
 * @param {object} params json object of the original api request
 * @param {Function} callback callback function of the original api request
 * @param {boolean} onerror true = execute callback also on error
 * @returns {void}
 */
function createEnterPinFooter(footers, method, params, callback, onerror) {
    const enterBtnId = method + 'EnterBtn';
    const input = elCreateEmpty('input', {"type": "password", "autocomplete": "off", "class": ["form-control", "border-secondary"]});
    const btn = elCreateTextTn('button', {"class": ["btn", "btn-success"], "id": enterBtnId}, 'Enter');
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
        if (footer.classList.contains('d-none') === false) {
            footer.classList.add('d-none');
            // remember the id of the now hidden footer
            const footerId = footer.getAttribute('id');
            if (footerId !== null) {
                newFooter.setAttribute('data-footer', footerId);
            }
            break;
        }
    }
    footers[0].parentNode.appendChild(newFooter);
    setFocus(input);
    btn.addEventListener('click', function(event) {
        //@ts-ignore
        btnWaiting(event.target, true);
        const alert = newFooter.querySelector('.alert');
        if (alert !== null) {
            alert.remove();
        }
        sendAPI('MYMPD_API_SESSION_LOGIN', {"pin": input.value}, function(obj) {
            btnWaitingId(enterBtnId, false);
            input.value = '';
            if (obj.error) {
                newFooter.appendChild(
                    elCreateTextTn('div', {"class": ["alert", "alert-danger", "p-2", "w-100"]}, obj.error.message, obj.error.data)
                );
            }
            else if (obj.result.session !== '') {
                setSession(obj.result.session);
                removeEnterPinFooter(newFooter);
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
 * @returns {void}
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
        // open modal to enter pin and resend API request
        // replace enter button to inherit the enterPin options
        const enterBtn = elCreateTextTn('button', {"id": "modalEnterPinEnterBtn", "class": ["btn", "btn-success"]}, 'Enter');
        enterBtn.addEventListener('click', function(event) {
            //@ts-ignore
            btnWaiting(event.target, true);
            sendAPI('MYMPD_API_SESSION_LOGIN', {
                "pin": elGetById('modalEnterPinPinInput').value},
                function(obj) {
                    btnWaitingId('modalEnterPinEnterBtn', false);
                    elGetById('modalEnterPinPinInput').value = '';
                    if (obj.error) {
                        const em = elGetById('modalEnterPinMessage');
                        em.textContent = tn(obj.error.message, obj.error.data);
                        elShow(em);
                    }
                    else if (obj.result.session !== '') {
                        setSession(obj.result.session);
                        uiElements.modalEnterPin.hide();
                        if (method !== undefined) {
                            //call original API
                            sendAPI(method, params, callback, onerror);
                        }
                    }
                }, true);
        }, false);
        elGetById('modalEnterPinEnterBtn').replaceWith(enterBtn);
        elHideId('modalEnterPinMessage');
        elGetById('modalEnterPinPinInput').value = '';
        uiElements.modalEnterPin.show();
    }
}

/**
 * Initializes the session object
 * @param {string} token retrieved session token
 * @returns {void}
 */
function setSession(token) {
    session.token = token;
    session.timeout = getTimestamp() + sessionLifetime;
    setSessionState();
    showNotification(tn('Session successfully created'), 'session', 'info');
}

/**
 * Sets the session state.
 * Shows/hides the lock indicator and the login/logout menu entry.
 * @returns {void}
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
 * @returns {void}
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
 * @returns {void}
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
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function removeSession() {
    sendAPI('MYMPD_API_SESSION_LOGOUT', {}, function() {
        session.timeout = 0;
        setSessionState();
    }, false);
}
