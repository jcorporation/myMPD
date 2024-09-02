"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalCustomSticker_js */

/**
 * Initialization functions for the sticker modal elements
 * @returns {void}
 */
function initModalSticker() {
    elGetById('modalStickerList').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        const target = event.target.closest('TR');
        if (event.target.nodeName === 'A') {
            const action = getData(event.target, 'action');
            const name = getData(target, 'name');
            switch(action) {
                case 'delete':
                    deleteSticker(event.target, name);
                    break;
                default:
                    logError('Invalid action: ' + action);
            }
            return;
        }
        if (checkTargetClick(target) === true) {
            showEditSticker(getData(target, 'name'), getData(target, 'value'));
        }
    }, false);

    setDataId('modalStickerNameInput', 'cb-filter', 'filterStickerSelect');
    setDataId('modalStickerNameInput', 'cb-filter-options', ['modalStickerNameInput']);
}

/**
 * Gets sticker names and populates a select
 * @param {string} elId select element id
 * @param {string} searchstr search string
 * @param {string} selectedSticker current selected playlist
 * @returns {void}
 */
function filterStickerSelect(elId, searchstr, selectedSticker) {
    const type = getDataId('modalSticker', 'type');
    sendAPI("MYMPD_API_STICKER_NAMES", {
        "type": type,
        "searchstr": searchstr
    }, function(obj) {
        populateStickerSelect(obj, elId, selectedSticker);
    }, false);
}

/**
 * Populates the custom input element mympd-select-search
 * @param {object} obj jsonrpc response
 * @param {string} stickerSelectId select element id
 * @param {string} selectedSticker current selected sticker
 * @returns {void}
 */
function populateStickerSelect(obj, stickerSelectId, selectedSticker) {
    if (selectedSticker === undefined) {
        selectedSticker = '';
    }
    const selectEl = elGetById(stickerSelectId);
    //set input element values
    selectEl.value = selectedSticker;
    setData(selectEl, 'value', selectedSticker);
    elClear(selectEl.filterResult);

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        selectEl.addFilterResultPlain(obj.result.data[i]);
        if (obj.result.data[i] === selectedSticker) {
            selectEl.filterResult.lastChild.classList.add('active');
        }
    }
}

/**
 * Saves the custom sticker
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSticker(target) {
    cleanupModalId('modalSticker');
    btnWaiting(target, true);
    sendAPI("MYMPD_API_STICKER_SET", {
        "uri": getDataId('modalSticker', 'uri'),
        "type": getDataId('modalSticker', 'type'),
        "name": elGetById('modalStickerNameInput').value,
        "value": elGetById('modalStickerValueInput').value
    }, saveStickerCheckError, true);
}

/**
 * Handler for the MYMPD_API_SCRIPT_SAVE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveStickerCheckError(obj) {
    if (modalApply(obj) === true) {
        showListSticker();
    }
}

/**
 * Opens the sticker modal and shows the list tab
 * @param {string} uri Sticker uri
 * @param {string} type Sticker type
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showStickerModal(uri, type) {
    setDataId('modalSticker', 'uri', uri),
    setDataId('modalSticker', 'type', type);
    uiElements.modalSticker.show();
    showListSticker();
}

/**
 * Shows the edit sticker tab
 * @param {string} name Sticker name
 * @param {string} value Sticker value
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showEditSticker(name, value) {
    cleanupModalId('modalSticker');
    elGetById('modalStickerListTab').classList.remove('active');
    elGetById('modalStickerEditTab').classList.add('active');
    elHideId('modalStickerListFooter');
    elShowId('modalStickerEditFooter');

    elGetById('modalStickerNameInput').value = '';
    elGetById('modalStickerNameInput').filterInput.value = '';
    filterStickerSelect('modalStickerNameInput', '', name);
    elGetById('modalStickerValueInput').value = value;
    if (name === '') {
        setFocusId('modalStickerNameInput');
        elEnableId('modalStickerNameInput');
    }
    else {
        elDisableId('modalStickerNameInput');
        setFocusId('modalStickerValueInput');
    }
}

/**
 * Shows the list scripts tab
 * @returns {void}
 */
function showListSticker() {
    cleanupModalId('modalSticker');
    elGetById('modalStickerListTab').classList.add('active');
    elGetById('modalStickerEditTab').classList.remove('active');
    elShowId('modalStickerListFooter');
    elHideId('modalStickerEditFooter');
    getStickerList();
}

/**
 * Deletes a script after confirmation
 * @param {EventTarget} el triggering element
 * @param {string} name Sticker name
 * @returns {void}
 */
function deleteSticker(el, name) {
    cleanupModalId('modalSticker');
    showConfirmInline(el.parentNode.previousSibling, tn('Do you really want to delete the sticker?'), tn('Yes, delete it'), function() {
        sendAPI("MYMPD_API_STICKER_DELETE", {
            "uri": getDataId('modalSticker', 'uri'),
            "type": getDataId('modalSticker', 'type'),
            "name": name
        }, deleteStickerCheckError, true);
    });
}

/**
 * Handler for the MYMPD_API_STICKER_DELETE jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function deleteStickerCheckError(obj) {
    if (modalListApply(obj) === true) {
        getStickerList();
    }
}

/**
 * Gets the list of stickers
 * @returns {void}
 */
function getStickerList() {
    sendAPI("MYMPD_API_STICKER_LIST", {
        "uri": getDataId('modalSticker', 'uri'),
        "type": getDataId('modalSticker', 'type')
    }, parseStickerList, true);
}

/**
 * Parses the MYMPD_API_STICKER_LIST jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseStickerList(obj) {
    const table = elGetById('modalStickerList');
    table.classList.add('stickerMympdHide');
    elGetById('modalStickerToggleInternal').classList.remove('active');
    const tbodySticker = table.querySelector('tbody');
    elClear(tbodySticker);
    if (checkResult(obj, table, 'table') === false) {
        return;
    }

    for (const key of stickerListSongs) {
        if (obj.result[key] !== undefined) {
            const tr = printStickerRow(key, obj.result[key]);
            tr.classList.add('stickerMympd');
            tbodySticker.appendChild(tr);
        }
    }
    let i = 0;
    for (const key in obj.result.sticker) {
        tbodySticker.appendChild(
            printStickerRow(key, obj.result.sticker[key])
        );
        i++;
    }

    if (i === 0) {
        tbodySticker.appendChild(
            elCreateNode('tr', {"class": ["not-clickable"]},
                elCreateNode('td', {"colspan": '2'},
                    elCreateTextTn('div', {"class": ["alert", "alert-secondary"]}, 'No user defined stickers.')
                )
            )
        );
    }
}

/**
 * Prints a sticker table row
 * @param {string} name Sticker name
 * @param {string} value Sticker value
 * @returns {HTMLElement} Table row
 */
function printStickerRow(name, value) {
    const tr = elCreateNodes('tr', {"title": tn('Edit')}, [
        elCreateText('td', {}, name),
        elCreateText('td', {}, value),
        elCreateNodes('td', {"data-col": "Action"}, [
            elCreateText('a', {"href": "#", "data-title-phrase": "Delete", "data-action": "delete", "class": ["me-2", "mi", "color-darkgrey"]}, 'delete'),
        ])
    ]);
    setData(tr, 'name', name);
    setData(tr, 'value', value);
    return tr;
}

/**
 * Shows/hides the interal stickers
 * @param {EventTarget} target Toggle button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showInternalStickers(target) {
    if (target.classList.contains('active')) {
        target.classList.remove('active');
        elGetById('modalStickerList').classList.add('stickerMympdHide');
    }
    else {
        target.classList.add('active');
        elGetById('modalStickerList').classList.remove('stickerMympdHide');
    }
}
