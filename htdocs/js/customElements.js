"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module customElements_js */

/**
 * Creates the pre-generated elements
 * @returns {void}
 */
function createPreGeneratedElements() {
    pEl.selectBtn = elCreateText('button', {"type": "button", "href": "#", "class": ["btn", "mi", "border-0", "color-darkgrey"], "data-title-phrase": "Select"}, 'radio_button_unchecked');
    pEl.selectAllBtn = elCreateText('button', {"type": "button", "href": "#", "class": ["btn", "mi", "border-0"], "data-title-phrase": "Select all"}, 'radio_button_unchecked');
    pEl.actionsBtn = elCreateText('a', {"data-action": "popover", "href": "#", "class": ["mi", "color-darkgrey"], "data-title-phrase": "Actions"}, ligatures['more']);
    pEl.actionsDiscBtn = elCreateText('a', {"data-action": "popover", "data-contextmenu": "disc", "href": "#", "class": ["mi", "color-darkgrey"], "data-title-phrase": "Actions"}, ligatures['more']);
    pEl.removeBtn = elCreateText('a', {"data-action": "quickRemove", "href": "#", "class": ["mi", "color-darkgrey", "me-1"], "data-title-phrase": "Remove"}, 'clear');
    pEl.playBtn = elCreateText('a', {"data-action": "quickPlay", "href": "#", "class": ["mi", "color-darkgrey", "me-1"], "data-title-phrase": "Quick play"}, 'play_arrow');
    pEl.showSongsBtn = elCreateText('a', {"data-action": "showSongsByTag", "class": ["mi", "color-darkgrey", "me-1"], "href": "#", "data-title-phrase": "Show songs"}, 'music_note');
    pEl.showAlbumsBtn = elCreateText('a', {"data-action": "showAlbumsByTag", "class": ["mi", "color-darkgrey", "me-1"], "href": "#", "data-title-phrase": "Show albums"}, 'album');
    pEl.showStickersBtn = elCreateText('a', {"data-action": "showStickersByTag", "class": ["mi", "color-darkgrey", "me-1", "featStickerAdv"], "href": "#", "data-title-phrase": "Sticker"}, 'discount');

    pEl.actionMenu = [
        pEl.actionsBtn,
        pEl.selectBtn
    ];
    pEl.actionMenuPlay = [
        pEl.playBtn,
        pEl.actionsBtn,
        pEl.selectBtn
    ];
    pEl.actionMenuRemove = [
        pEl.removeBtn,
        pEl.actionsBtn,
        pEl.selectBtn
    ];
    pEl.actionMenuPlayRemove = [
        pEl.playBtn,
        pEl.removeBtn,
        pEl.actionsBtn,
        pEl.selectBtn
    ];
    pEl.actionMenuBrowseDatabaseTagSongs = [
        pEl.showSongsBtn,
        pEl.showStickersBtn
    ];
    pEl.actionMenuBrowseDatabaseTagSongAlbums = [
        pEl.showSongsBtn,
        pEl.showAlbumsBtn,
        pEl.showStickersBtn
    ];
    pEl.actionMenuDisc = [
        pEl.actionsDiscBtn
    ];
    pEl.actionMenuDiscPlay = [
        pEl.playBtn,
        pEl.actionsDiscBtn
    ];

    pEl.actionIcons = pEl.actionMenu;
    pEl.actionDiscIcons = pEl.actionMenuDisc;
    pEl.actionQueueIcons = pEl.actionMenu;
    pEl.actionJukeboxIcons = pEl.actionMenu;
    pEl.actionPlaylistDetailIcons = pEl.actionMenu;
    pEl.actionPlaylistIcons = pEl.actionMenu;

    pEl.viewTable = elCreateNode('div', {'class': ['table-responsive', 'scrollContainer']},
        elCreateNodes('table', {'class': ['table', 'table-hover', 'table-sm']}, [
            elCreateNode('thead', {},
                elCreateEmpty('tr', {})
            ),
            elCreateEmpty('tbody', {'class': ['clickable']}),
            elCreateEmpty('tfoot')
        ])
    );
    pEl.viewGrid = elCreateNode('div', {'class': ['container-fluid', 'cardsContainer', 'scrollContainer']},
        elCreateEmpty('div', {'class': ['row', 'mympd-grid']})
    );
    pEl.viewList = elCreateNode('div', {'class': ['scrollContainer', 'listContainer']},
        elCreateEmpty('div', {'class': ['list-group']})
    );
    pEl.resumeBtn = elCreateNodes('div', {'class': ['btn-group', 'dropup']}, [
        elCreateText('button', {'type': 'button', 'data-title-phrase': 'Resume', 'data-bs-toggle': 'dropdown', 'class': ['btn', 'btn-sm', 'btn-secondary', 'dropdown-toggle', 'mi', 'mi-sm']}, 'replay'),
        elCreateNode('div', {'class': ['dropdown-menu', 'px-2']}, 
            elCreateNodes('div', {'class': ['d-grid', 'gap-2']}, [
                elCreateTextTn('button', {'class': ['btn', 'btn-sm', 'btn-secondary'], 'data-action': 'append'}, 'Append to queue'),
                elCreateTextTn('button', {'class': ['btn', 'btn-sm', 'btn-secondary'], 'data-action': 'insert'}, 'Insert at start of queue'),
                elCreateTextTn('button', {'class': ['btn', 'btn-sm', 'btn-secondary'], 'data-action': 'replace'}, 'Replace queue'),
            ])
        )
    ]);
}

/**
 * Initializes all elements with data-is attribute
 * We do not use the custom element is="" feature - safari does not support it
 * @param {Element} parent root element to find custom elements
 * @returns {void}
 */
function initElements(parent) {
    for (const el of parent.querySelectorAll('[data-is]')) {
        initElement(el, el.getAttribute('data-is'));
    }
}

/**
 * Initializes a custom element
 * @param {Element} el elment
 * @param {string} elType type of the custom element
 * @returns {void}
 */
function initElement(el, elType) {
    switch(elType) {
        case 'mympd-input-clear':
            setInputClear(el);
            break;
        case 'mympd-input-reset':
            setInputReset(el);
            break;
        case 'mympd-input-password':
            setInputPassword(el);
            break;
        case 'mympd-select-search':
            setSelectSearch(el);
            break;
        case 'mympd-select-new':
            setSelectNew(el);
            break;
        default:
            logError('Invalid element type: ' + elType);
    }
    el.removeAttribute('data-is');
}

/**
 * Creates an input element with clear button
 * @param {Element} el element
 * @returns {void}
 */
function setInputClear(el) {
    const button = elCreateText('button', {"data-title-phrase": "Clear", "class": ["mi", "mi-sm", "input-inner-button"]}, 'clear');
    el.button = button;
    el.classList.add('innerButton');
    if (el.classList.contains('alwaysEnabled')) {
        el.button.classList.add('alwaysEnabled');
    }
    if (el.parentNode.classList.contains('col')) {
        el.button.style.right = '1rem';
    }
    el.parentNode.insertBefore(el.button, el.nextElementSibling);
    el.updateBtn = function() {
        if (el.value === '') {
            elHide(el.button);
        }
        else {
            elShow(el.button);
        }
    };
    el.updateBtn();
    
    el.addEventListener('keyup', function() {
        el.updateBtn();
    }, false);

    el.button.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
    }, false);

    el.button.addEventListener('mouseup', function(event) {
        event.target.previousElementSibling.value = '';
        const dataClearEvent = event.target.previousElementSibling.getAttribute('data-clear-event');
        if (dataClearEvent !== null) {
            const clearEvent = new KeyboardEvent(dataClearEvent, {key: 'Enter'});
            event.target.previousElementSibling.dispatchEvent(clearEvent);
        }
    }, false);
}

/**
 * Creates an input element with reset to default button
 * @param {Element} el element
 * @returns {void}
 */
function setInputReset(el) {
    const button = elCreateText('button', {"data-title-phrase": "Reset to default", "class": ["mi", "mi-sm", "input-inner-button"]}, 'settings_backup_restore');
    el.button = button;
    el.classList.add('innerButton');
    if (el.classList.contains('alwaysEnabled')) {
        el.button.classList.add('alwaysEnabled');
    }
    if (el.nextElementSibling) {
        el.parentNode.insertBefore(el.button, el.nextElementSibling);
    }
    else {
        el.parentNode.appendChild(el.button);
    }
    el.button.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const input = event.target.previousElementSibling;
        input.value = getData(input, 'default') !== undefined ? getData(input, 'default') :
            (input.getAttribute('placeholder') !== null ? input.getAttribute('placeholder') : '');
    }, false);
}

/**
 * Creates an password input element with show button
 * @param {Element} el element
 * @returns {void}
 */
function setInputPassword(el) {
    const button = elCreateText('button', {"data-title-phrase": "Show or hide", "class": ["mi", "mi-sm", "input-inner-button"]}, 'visibility');
    el.button = button;
    el.classList.add('innerButton');
    if (el.classList.contains('alwaysEnabled')) {
        el.button.classList.add('alwaysEnabled');
    }
    if (el.parentNode.classList.contains('col-sm-8')) {
        el.button.style.right = '1rem';
    }
    if (el.nextElementSibling) {
        el.parentNode.insertBefore(el.button, el.nextElementSibling);
    }
    else {
        el.parentNode.appendChild(el.button);
    }
    el.button.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const input = event.target.previousElementSibling;
        if (input.type === 'password') {
            input.type = 'text';
            event.target.textContent = 'visibility_off';
        }
        else {
            input.type = 'password';
            event.target.textContent = 'visibility';
        }
    }, false);
}

/**
 * Creates a searchable combined select + input element from an input element
 * @param {Element} el element
 * @returns {void}
 */
function setSelectSearch(el) {
    const filterInput = elCreateEmpty('input', {"class": ["form-control", "form-control-sm", "mb-1"], "data-placeholder-phrase": "Filter", "placeholder": tn('Filter')});
    const filterResult = elCreateEmpty('ul', {"class": ["list-group", "list-group-scroll"]});
    const dropdown = elCreateNodes('div', {"class": ["dropdown-menu", "p-2", "w-100"]}, [
        filterInput,
        filterResult
    ]);
    el.parentNode.insertBefore(dropdown, el.nextElementSibling);

    const button = elCreateEmpty('button', {"class": ["input-inner-button", "select-inner-button"], "data-bs-toggle": "dropdown"});
    if (el.parentNode.classList.contains('col-sm-8')) {
        button.style.right = '1rem';
    }
    button.style.cursor = 'default';
    el.parentNode.insertBefore(button, el.nextElementSibling);
    el.dropdownButton = button;
    el.filterInput = filterInput;
    el.filterResult = filterResult;
    el.classList.add('innerButton');
    if (el.classList.contains('alwaysEnabled')) {
        el.button.classList.add('alwaysEnabled');
    }
    setData(el, 'value', el.value);
    el.setValue = function(value, valueTn) {
        el.value = valueTn;
        setData(el, 'value', value);
    };
    el.addEventListener('keyup', function(event) {
        setData(el, 'value', event.target.value);
    }, false);
    el.filterResult.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        el.value = event.target.textContent;
        setData(el, 'value', getData(event.target, 'value'));
        BSN.Dropdown.getInstance(el.nextElementSibling).hide();
        const changeEvent = new Event('change');
        el.dispatchEvent(changeEvent);
    }, false);
    el.filterInput.addEventListener('keyup', function(event) {
        const cb = getData(el, 'cb-filter');
        const cbOptions = getData(el, 'cb-filter-options');
        // @ts-ignore
        window[cb](... cbOptions, event.target.value);
    }, false);
    el.filterInput.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    el.addFilterResult = function(text, value) {
        const item = elCreateTextTn('li', {"class": ["list-group-item", "list-group-item-action", "clickable"]}, text);
        setData(item, 'value', value);
        el.filterResult.appendChild(item);
    };
    el.addFilterResultPlain = function(value) {
        const item = elCreateText('li', {"class": ["list-group-item", "list-group-item-action", "clickable"]}, value);
        setData(item, 'value', value);
        el.filterResult.appendChild(item);
    };
    new BSN.Dropdown(el.dropdownButton);
    if (el.getAttribute('readonly') === 'readonly') {
        el.addEventListener('click', function(event) {
            BSN.Dropdown.getInstance(event.target.nextElementSibling).toggle();
        }, false);
    }
    if (userAgentData.isMobile === true) {
        //scrolling optimization for mobile browsers
        el.parentNode.addEventListener('shown.bs.dropdown', function() {
            domCache.body.style.overflow = 'hidden';
        }, false);
        el.parentNode.addEventListener('hidden.bs.dropdown', function() {
            domCache.body.style.overflow = 'initial';
        }, false);
    }
    el.dropdownButton.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
    }, false);
}

/**
 * Creates a combined select + input element from an input element
 * @param {Element} el element
 * @returns {void}
 */
 function setSelectNew(el) {
    const filterResult = elCreateEmpty('ul', {"class": ["list-group", "list-group-scroll", "border"]});
    const dropdown = elCreateNodes('div', {"class": ["dropdown-menu", "p-2", "w-100"]}, [
        filterResult
    ]);
    el.parentNode.insertBefore(dropdown, el.nextElementSibling);

    const button = elCreateEmpty('button', {"class": ["input-inner-button", "select-inner-button"], "data-bs-toggle": "dropdown"});
    if (el.parentNode.classList.contains('col-sm-8')) {
        button.style.right = '1rem';
    }
    button.style.cursor = 'default';
    el.parentNode.insertBefore(button, el.nextElementSibling);
    el.dropdownButton = button;
    el.filterResult = filterResult;
    el.classList.add('innerButton');
    if (el.classList.contains('alwaysEnabled')) {
        el.button.classList.add('alwaysEnabled');
    }
    setData(el, 'value', el.value);
    el.setValue = function(value, valueTn) {
        el.value = valueTn;
        setData(el, 'value', value);
    };
    el.addEventListener('keyup', function(event) {
        setData(el, 'value', event.target.value);
    }, false);
    el.filterResult.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        el.value = event.target.textContent;
        setData(el, 'value', getData(event.target, 'value'));
        BSN.Dropdown.getInstance(el.nextElementSibling).hide();
        const changeEvent = new Event('change');
        el.dispatchEvent(changeEvent);
    }, false);
    el.addFilterResult = function(text, value) {
        const item = elCreateTextTn('li', {"class": ["list-group-item", "list-group-item-action", "clickable"]}, text);
        setData(item, 'value', value);
        el.filterResult.appendChild(item);
    };
    el.addFilterResultPlain = function(value) {
        const item = elCreateText('li', {"class": ["list-group-item", "list-group-item-action", "clickable"]}, value);
        setData(item, 'value', value);
        el.filterResult.appendChild(item);
    };
    new BSN.Dropdown(el.dropdownButton);
    if (userAgentData.isMobile === true) {
        //scrolling optimization for mobile browsers
        el.parentNode.addEventListener('shown.bs.dropdown', function() {
            domCache.body.style.overflow = 'hidden';
        }, false);
        el.parentNode.addEventListener('hidden.bs.dropdown', function() {
            domCache.body.style.overflow = 'initial';
        }, false);
    }
    el.dropdownButton.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
    }, false);
}
