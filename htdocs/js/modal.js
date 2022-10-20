"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modal_js */

/**
 * Opens a modal
 * @param {string} modal name of the modal
 */
//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    uiElements[modal].show();
}

/**
 * Gets the currently opened modal
 * @returns {Element} the opened modal or null if no modal is opened
 */
 function getOpenModal() {
    const modals = document.querySelectorAll('.modal');
    for (const modal of modals) {
        if (modal.classList.contains('show')) {
            return modal;
        }
    }
    return null;
}

/**
 * Removes all invalid indicators and warning messages from a modal with the given id.
 * @param {string} id id of the modal
 */
 function cleanupModalId(id) {
    cleanupModal(document.getElementById(id));
}

/**
 * Removes all invalid indicators and warning messages from a modal pointed by el.
 * @param {Element} el the modal element
 */
function cleanupModal(el) {
    //remove validation warnings
    removeIsInvalid(el);
    //remove enter pin footer
    const enterPinFooter = el.querySelector('.enterPinFooter');
    if (enterPinFooter !== null) {
        removeEnterPinFooter(enterPinFooter);
    }
    //remove error messages
    hideModalAlert(el);
    //remove spinners
    const spinners = el.querySelectorAll('.spinner-border');
    for (let i = spinners.length - 1; i >= 0; i--) {
        spinners[i].remove();
    }
}

/**
 * Shows a confirmation modal
 * @param {string} text text to show (already translated)
 * @param {string} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 */
 function showConfirm(text, btnText, callback) {
    document.getElementById('modalConfirmText').textContent = text;
    const yesBtn = elCreateText('button', {"id": "modalConfirmYesBtn", "class": ["btn", "btn-danger"]}, btnText);
    yesBtn.addEventListener('click', function() {
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            callback();
        }
        uiElements.modalConfirm.hide();
    }, false);
    document.getElementById('modalConfirmYesBtn').replaceWith(yesBtn);
    uiElements.modalConfirm.show();
}

/**
 * Shows an inline confirmation (for open modals)
 * @param {Element | ChildNode} el parent element to add the confirmation dialog
 * @param {string} text text to show (already translated)
 * @param {string} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 */
function showConfirmInline(el, text, btnText, callback) {
    const confirm = elCreateNode('div', {"class": ["alert", "alert-danger", "mt-2"]},
        elCreateText('p', {}, text)
    );

    const cancelBtn = elCreateTextTn('button', {"class": ["btn", "btn-secondary"]}, 'Cancel');
    cancelBtn.addEventListener('click', function(event) {
        event.stopPropagation();
        this.parentNode.remove();
    }, false);
    confirm.appendChild(cancelBtn);

    const yesBtn = elCreateText('button', {"class": ["btn", "btn-danger", "float-end"]}, btnText);
    yesBtn.addEventListener('click', function(event) {
        event.stopPropagation();
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            callback();
        }
        this.parentNode.remove();
    }, false);
    confirm.appendChild(yesBtn);
    el.appendChild(confirm);
}
