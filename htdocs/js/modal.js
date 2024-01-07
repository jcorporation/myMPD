"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modal_js */

/**
 * Opens a modal
 * @param {string} modal name of the modal
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function openModal(modal) {
    switch(modal) {
        case 'modalScripts':
            showListScriptModal();
            break;
        default:
            uiElements[modal].show();
    }
}

/**
 * Gets the currently opened modal
 * @returns {Element} the opened modal or null if no modal is opened
 */
 function getOpenModal() {
    return document.querySelector('.modal.show');
}

/**
 * Sets the focus to the first input or select element
 * @param {EventTarget | Element} container the parent of the elements
 * @returns {void}
 */
function focusFirstInput(container) {
    const inputs = container.querySelectorAll('.modal-body input, .modal-body select, .modal-body textarea');
    for (const input of inputs) {
        if (input.offsetHeight === 0) {
            // element is not shown
            continue;
        }
        if (input.getAttribute('readonly') !== null &&
            input.getAttribute('data-is') !== 'mympd-select-search')
        {
            // readonly element
            continue;
        }
        setFocus(input);
        break;
    }
}

/**
 * Populates the entities element
 * @param {string} id element id
 * @param {Array} entities array with entities to add
 * @returns {void}
 */
function populateEntities(id, entities) {
    elGetById(id).value = arrayToLines(entities);
}

/**
 * Handles the apply jsonrpc response for a modal
 * Shows the possible error and leaves the modal open
 * @param {object} obj jsonrpc response
 * @returns {boolean} true on close, else false
 */
function modalApply(obj) {
    return _modalClose(obj, false);
}

/**
 * Handles the save/apply jsonrpc response for a modal
 * Shows the possible error or closes the modal
 * @param {object} obj jsonrpc response
 * @returns {boolean} true on close, else false
 */
function modalClose(obj) {
    return _modalClose(obj, true);
}

/**
 * Handles the save/apply jsonrpc response for a modal
 * @param {object} obj jsonrpc response
 * @param {boolean} close close the modal if there is no error?
 * @returns {boolean} true on close, else false
 */
function _modalClose(obj, close) {
    const modal = getOpenModal();
    const modalId = modal.getAttribute('id');
    const spinnerEl = modal.querySelector('.spinner-border');
    if (spinnerEl) {
        btnWaiting(spinnerEl.parentNode, false);
    }
    if (obj.error) {
        if (highlightInvalidInput(modalId, obj) === false) {
            showModalAlert(obj);
        }
        return false;
    }
    // no error
    if (close === true) {
        uiElements[modalId].hide();
    }
    return true;
}

/**
 * Removes all invalid indicators and warning messages from a modal with the given id.
 * @param {string} id id of the modal
 * @returns {void}
 */
 function cleanupModalId(id) {
    cleanupModal(elGetById(id));
}

/**
 * Removes all invalid indicators and warning messages from a modal pointed by el.
 * @param {Element} el the modal element
 * @returns {void}
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
        const btn = spinners[i].parentNode;
        if (btn) {
            btn.removeAttribute('disabled');
        }
        spinners[i].remove();
    }
}

/**
 * Shows a confirmation modal
 * @param {string} text text to show (already translated)
 * @param {string} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 * @returns {void}
 */
 function showConfirm(text, btnText, callback) {
    elGetById('modalConfirmText').textContent = text;
    const yesBtn = elCreateText('button', {"id": "modalConfirmYesBtn", "class": ["btn", "btn-danger"]}, btnText);
    yesBtn.addEventListener('click', function() {
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            callback();
        }
        uiElements.modalConfirm.hide();
    }, false);
    elGetById('modalConfirmYesBtn').replaceWith(yesBtn);
    uiElements.modalConfirm.show();
}

/**
 * Shows an inline confirmation (for open modals)
 * @param {Element | ChildNode} el parent element to add the confirmation dialog
 * @param {string} text text to show (already translated)
 * @param {string} btnText text for the yes button (already translated)
 * @param {Function} callback callback function on confirmation
 * @returns {void}
 */
function showConfirmInline(el, text, btnText, callback) {
    const confirm = elCreateNode('div', {"class": ["alert", "alert-danger", "mt-2", "not-clickable"]},
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
