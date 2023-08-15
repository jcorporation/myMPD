"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalHomeIcon_js */

/**
 * Initializes the modalHomeIcon
 * @returns {void}
 */
function initModalHomeIcon() {
    const selectHomeIconCmd = document.getElementById('selectHomeIconCmd');
    selectHomeIconCmd.addEventListener('change', function() {
        showHomeIconCmdOptions(undefined);
    }, false);

    document.getElementById('inputHomeIconBgcolor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.backgroundColor = event.target.value;
    }, false);

    document.getElementById('inputHomeIconColor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.color = event.target.value;
    }, false);

    document.getElementById('inputHomeIconImage').addEventListener('change', function(event) {
        const value = getData(event.target, 'value');
        if (value !== '') {
            document.getElementById('homeIconPreview').style.backgroundImage = getCssImageUri(value);
            elHideId('divHomeIconLigature');
            elClearId('homeIconPreview');
        }
        else {
            document.getElementById('homeIconPreview').style.backgroundImage = '';
            elShowId('divHomeIconLigature');
            document.getElementById('homeIconPreview').textContent =
                document.getElementById('inputHomeIconLigature').value;
        }
    }, false);

    setDataId('inputHomeIconImage', 'cb-filter', 'filterImageSelect');
    setDataId('inputHomeIconImage', 'cb-filter-options', ['inputHomeIconImage']);

    document.getElementById('btnHomeIconLigature').parentNode.addEventListener('show.bs.dropdown', function () {
        populateHomeIconLigatures();
        const selLig = document.getElementById('inputHomeIconLigature').value;
        if (selLig !== '') {
            document.getElementById('searchHomeIconLigature').value = selLig;
            if (selLig !== '') {
                elShow(document.getElementById('searchHomeIconLigature').nextElementSibling);
            }
            else {
                elHide(document.getElementById('searchHomeIconLigature').nextElementSibling);
            }
            filterHomeIconLigatures();
        }
    }, false);

    document.getElementById('listHomeIconLigature').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            selectHomeIconLigature(event.target);
            uiElements.dropdownHomeIconLigature.hide();
        }
    });

    document.getElementById('searchHomeIconLigature').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    const searchHomeIconCat = document.getElementById('searchHomeIconCat');
    searchHomeIconCat.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    searchHomeIconCat.addEventListener('change', function() {
        filterHomeIconLigatures();
    }, false);

    const searchHomeIconLigature = document.getElementById('searchHomeIconLigature');
    searchHomeIconLigature.addEventListener('keydown', function(event) {
        event.stopPropagation();
        if (event.key === 'Enter') {
            event.preventDefault();
        }
    }, false);

    searchHomeIconLigature.addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            const sel = document.querySelector('#listHomeIconLigature .active');
            if (sel !== null) {
                selectHomeIconLigature(sel);
                uiElements.dropdownHomeIconLigature.hide();
            }
        }
        else {
            filterHomeIconLigatures();
        }
    }, false);
}

/**
 * Saves the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveHomeIcon() {
    cleanupModalId('modalHomeIcon');
    let formOK = true;
    const nameEl = document.getElementById('inputHomeIconName');
    if (!validateNotBlankEl(nameEl)) {
        formOK = false;
    }
    if (formOK === true) {
        const options = [];
        const optionEls = document.querySelectorAll('#divHomeIconOptions input, #divHomeIconOptions select');
        for (const optionEl of optionEls) {
            switch(optionEl.nodeName) {
                case 'SELECT':
                    options.push(getSelectValue(optionEl));
                    break;
                default:
                    options.push(optionEl.value);
            }
        }
        const image = getData(document.getElementById('inputHomeIconImage'), 'value');
        sendAPI("MYMPD_API_HOME_ICON_SAVE", {
            "replace": strToBool(document.getElementById('inputHomeIconReplace').value),
            "oldpos": Number(document.getElementById('inputHomeIconOldpos').value),
            "name": nameEl.value,
            "ligature": (image === '' ? document.getElementById('inputHomeIconLigature').value : ''),
            "bgcolor": document.getElementById('inputHomeIconBgcolor').value,
            "color": document.getElementById('inputHomeIconColor').value,
            "image": image,
            "cmd": document.getElementById('selectHomeIconCmd').value,
            "options": options
        }, saveHomeIconClose, true);
    }
}

/**
 * Response handler for save home icon
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveHomeIconClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalHomeIcon.hide();
    }
}

/**
 * Populates the ligatures dropdown
 * @returns {void}
 */
function populateHomeIconLigatures() {
    const listHomeIconLigature = document.getElementById('listHomeIconLigature');
    const searchHomeIconCat = document.getElementById('searchHomeIconCat');
    if (searchHomeIconCat.firstChild !== null) {
        return;
    }
    elClear(listHomeIconLigature);
    elClear(searchHomeIconCat);
    searchHomeIconCat.appendChild(
        elCreateTextTn('option', {"value": "all"}, 'icon-all')
    );
    for (const cat in materialIcons) {
        searchHomeIconCat.appendChild(
            elCreateTextTn('option', {"value": cat}, 'icon-' + cat)
        );
        for (const icon of materialIcons[cat]) {
            listHomeIconLigature.appendChild(
                elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm", "mi", "m-1"], "title": icon, "data-cat": cat}, icon)
            );
        }
    }
}

/**
 * Event handler for selecting a ligature
 * @param {EventTarget} el selected element
 * @returns {void}
 */
function selectHomeIconLigature(el) {
    document.getElementById('inputHomeIconLigature').value = el.getAttribute('title');
    document.getElementById('homeIconPreview').textContent = el.getAttribute('title');
    document.getElementById('homeIconPreview').style.backgroundImage = '';
    document.getElementById('inputHomeIconImage').value = tn('Use ligature');
    setData(document.getElementById('inputHomeIconImage'), 'value', '');
}

/**
 * Event handler for ligature search
 * @returns {void}
 */
function filterHomeIconLigatures() {
    const str = document.getElementById('searchHomeIconLigature').value.toLowerCase();
    const cat = getSelectValueId('searchHomeIconCat');
    const els = document.querySelectorAll('#listHomeIconLigature button');
    for (let i = 0, j = els.length; i < j; i++) {
        if ((str === '' || els[i].getAttribute('title').indexOf(str) > -1) &&
            (cat === 'all' || els[i].getAttribute('data-cat') === cat))
        {
            elShow(els[i]);
            if (els[i].getAttribute('title') === str) {
                els[i].classList.add('active');
            }
            else {
                els[i].classList.remove('active');
            }
        }
        else {
            elHide(els[i]);
            els[i].classList.remove('active' );
        }
    }
}

/**
 * Populates the cmd select box in the add to homescreen dialog
 * @param {string} cmd command
 * @param {string} type one of album, song, dir, search, plist, smartpls
 * @returns {void}
 */
function populateHomeIconCmdSelect(cmd, type) {
    const selectHomeIconCmd = document.getElementById('selectHomeIconCmd');
    elClear(selectHomeIconCmd);
    switch(cmd) {
        case 'appGoto': {
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "appGoto"}, 'Goto view')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["App", "Tab", "View", "Offset", "Limit", "Filter", "Sort", "Tag", "Search"]});
            break;
        }
        case 'openExternalLink': {
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "openExternalLink"}, 'Open external link')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Uri"]});
            break;
        }
        case 'openModal': {
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "openModal"}, 'Open modal')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Modal"]});
            break;
        }
        case 'execScriptFromOptions': {
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "execScriptFromOptions"}, 'Execute script')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options":["Script", "Arguments"]});
            break;
        }
        default: {
            const paramName = type === 'search'
                ? 'Expression'
                : type === 'album'
                    ? 'AlbumId'
                    : 'Uri';
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "replaceQueue"}, 'Replace queue')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "replacePlayQueue"}, 'Replace queue and play')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            if (features.featWhence === true) {
                selectHomeIconCmd.appendChild(
                    elCreateTextTn('option', {"value": "insertAfterCurrentQueue"}, 'Insert after current playing song')
                );
                setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            }
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "appendQueue"}, 'Append to queue')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            selectHomeIconCmd.appendChild(
                elCreateTextTn('option', {"value": "appendPlayQueue"}, 'Append to queue and play')
            );
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            if (type === 'dir' ||
                type === 'search' ||
                type === 'plist' ||
                type === 'smartpls' ||
                type === 'album')
            {
                const title = type === 'dir'
                    ? 'Open directory'
                    : type === 'search'
                        ? 'Show search'
                        : type === 'album'
                            ? 'Album details'
                            : 'View playlist';
                selectHomeIconCmd.appendChild(
                    elCreateTextTn('option', {"value": "homeIconGoto"}, title)
                );
                setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
            }
        }
    }
}

/**
 * Changes the options in the home icon edit modal for the selected cmd.
 * @param {Array} values values to set for the options
 * @returns {void}
 */
function showHomeIconCmdOptions(values) {
    const oldOptions = [];
    const optionEls = document.querySelectorAll('#divHomeIconOptions input');
    for (const optionEl of optionEls) {
        oldOptions.push(optionEl.value);
    }
    const divHomeIconOptions = document.getElementById('divHomeIconOptions');
    elClear(divHomeIconOptions);
    const options = getSelectedOptionDataId('selectHomeIconCmd', 'options');
    if (options !== undefined) {
        for (let i = 0, j = options.options.length; i < j; i++) {
            let value = values !== undefined
                ? values[i] !== undefined
                    ? values[i]
                    : ''
                : '';
            if (value === '' &&
                oldOptions[i] !== undefined) {
                value = oldOptions[i];
            }
            if (typeof value === 'object') {
                value = JSON.stringify(value);
            }
            const row = elCreateNodes('div', {"class": ["mb-3", "row"]}, [
                elCreateTextTn('label', {"class": ["col-sm-4"]}, options.options[i]),
                elCreateNode('div', {"class": ["col-sm-8"]}, 
                    createHomeIconCmdOptionEl(options.options[i], value)
                )
            ]);
            divHomeIconOptions.appendChild(row);
        }
    }
}

/**
 * Creates the form element to select the option value for home icon cmd
 * @param {string} name name of the element
 * @param {string} value value of the element
 * @returns {HTMLElement} the created form element
 */
function createHomeIconCmdOptionEl(name, value) {
    switch(name) {
        case 'Modal': {
            const sel = elCreateEmpty('select', {"class": ["form-select", "border-secondary"], "name": name});
            for (const v of ["modalConnection", "modalSettings", "modalMaintenance", "modalScripts",
                             "modalTimer", "modalTrigger", "modalMounts", "modalAbout"])
            {
                sel.appendChild(
                    elCreateTextTn('option', {"value": v}, v)
                );
            }
            sel.value = value;
            return sel;
        }
        default:
            return elCreateEmpty('input', {"class": ["form-control", "border-secondary"], "name": name, "value": value});
    }
}

/**
 * Populates the picture list in the home icon edit modal
 * @returns {void}
 */
function getHomeIconPictureList() {
    const selectHomeIconImage = document.getElementById('inputHomeIconImage');
    getImageList(selectHomeIconImage, [{"value": "", "text": tn('Use ligature')}], 'thumbs');
}
