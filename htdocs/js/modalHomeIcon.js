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
    const modalHomeIconCmdInput = elGetById('modalHomeIconCmdInput');
    modalHomeIconCmdInput.addEventListener('change', function() {
        showHomeIconCmdOptions(undefined);
    }, false);

    elGetById('modalHomeIconBgcolorInput').addEventListener('change', function(event) {
        elGetById('modalHomeIconPreview').style.backgroundColor = event.target.value;
    }, false);

    elGetById('modalHomeIconColorInput').addEventListener('change', function(event) {
        elGetById('modalHomeIconPreview').style.color = event.target.value;
    }, false);

    elGetById('modalHomeIconImageInput').addEventListener('change', function(event) {
        const value = getData(event.target, 'value');
        if (value !== '') {
            elGetById('modalHomeIconPreview').style.backgroundImage = getCssImageUri(value);
            elHideId('divHomeIconLigature');
            elClearId('modalHomeIconPreview');
        }
        else {
            elGetById('modalHomeIconPreview').style.backgroundImage = '';
            elShowId('divHomeIconLigature');
            elGetById('modalHomeIconPreview').textContent =
                elGetById('modalHomeIconLigatureInput').value;
        }
    }, false);

    setDataId('modalHomeIconImageInput', 'cb-filter', 'filterImageSelect');
    setDataId('modalHomeIconImageInput', 'cb-filter-options', ['modalHomeIconImageInput']);

    elGetById('modalHomeIconLigatureBtn').parentNode.addEventListener('show.bs.dropdown', function () {
        populateHomeIconLigatures();
        const selLig = elGetById('modalHomeIconLigatureInput').value;
        if (selLig !== '') {
            elGetById('modalHomeIconLigatureSearch').value = selLig;
            if (selLig !== '') {
                elShow(elGetById('modalHomeIconLigatureSearch').nextElementSibling);
            }
            else {
                elHide(elGetById('modalHomeIconLigatureSearch').nextElementSibling);
            }
            filterHomeIconLigatures();
        }
    }, false);

    elGetById('modalHomeIconLigatureList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            selectHomeIconLigature(event.target);
            uiElements.modalHomeIconLigatureDropdown.hide();
        }
    });

    elGetById('modalHomeIconLigatureSearch').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    const modalHomeIconSearchCat = elGetById('modalHomeIconSearchCat');
    modalHomeIconSearchCat.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    modalHomeIconSearchCat.addEventListener('change', function() {
        filterHomeIconLigatures();
    }, false);

    const modalHomeIconLigatureSearch = elGetById('modalHomeIconLigatureSearch');
    modalHomeIconLigatureSearch.addEventListener('keydown', function(event) {
        event.stopPropagation();
        if (event.key === 'Enter') {
            event.preventDefault();
        }
    }, false);

    modalHomeIconLigatureSearch.addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            const sel = document.querySelector('#modalHomeIconLigatureList .active');
            if (sel !== null) {
                selectHomeIconLigature(sel);
                uiElements.modalHomeIconLigatureDropdown.hide();
            }
        }
        else {
            filterHomeIconLigatures();
        }
    }, false);
}

/**
 * Opens the add to homescreen modal, this function is called by the add*ToHome functions above.
 * @param {string} cmd action
 * @param {string} name name for the home icon
 * @param {string} ligature ligature for the home icon
 * @param {string} image picture for the home icon
 * @param {object} options options array
 * @returns {void}
 */
function _addHomeIcon(cmd, name, ligature, image, options) {
    const modal = elGetById('modalHomeIcon');
    elGetById('modalHomeIconTitle').textContent = tn('Add to homescreen');
    setData(modal, 'replace', false);
    setData(modal, 'oldpos', 0);
    elGetById('modalHomeIconNameInput').value = name;
    elGetById('modalHomeIconBgcolorInput').value = defaults.PARTITION_HIGHLIGHT_COLOR;
    elGetById('modalHomeIconColorInput').value = defaults.PARTITION_HIGHLIGHT_COLOR_CONTRAST;

    populateHomeIconCmdSelect(cmd, options[0]);
    elGetById('modalHomeIconCmdInput').value = cmd;
    elClearId('modalHomeIconCmdOptions');
    showHomeIconCmdOptions(options);
    getHomeIconPictureList();
    const modalHomeIconPreviewEl = elGetById('modalHomeIconPreview');
    const homeIconImageInput = elGetById('modalHomeIconImageInput');
    if (image !== '') {
        homeIconImageInput.value = image;
        setData(homeIconImageInput, 'value', image);
        elGetById('modalHomeIconLigatureInput').value = '';
        elClear(modalHomeIconPreviewEl);
        modalHomeIconPreviewEl.style.backgroundImage = getCssImageUri(image);
        elHideId('divHomeIconLigature');
    }
    else {
        //use ligature
        homeIconImageInput.value = tn('Use ligature');
        setData(homeIconImageInput, 'value', '');
        elGetById('modalHomeIconLigatureInput').value = ligature;
        modalHomeIconPreviewEl.textContent = ligature;
        modalHomeIconPreviewEl.style.backgroundImage = '';
        elShowId('divHomeIconLigature');
    }

    modalHomeIconPreviewEl.style.backgroundColor = defaults.PARTITION_HIGHLIGHT_COLOR;
    modalHomeIconPreviewEl.style.color = defaults.PARTITION_HIGHLIGHT_COLOR_CONTRAST;
    uiElements.modalHomeIcon.show();
}

/**
 * The real edit home icon function
 * @param {number} pos home icon position
 * @param {boolean} replace true = replace existing home icon, false = duplicate home icon
 * @param {string} title title for the modal
 * @returns {void}
 */
function _editHomeIcon(pos, replace, title) {
    elGetById('modalHomeIconTitle').textContent = tn(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        const modal = elGetById('modalHomeIcon');
        setData(modal, 'replace', replace);
        setData(modal, 'oldpos', pos);
        elGetById('modalHomeIconNameInput').value = obj.result.data.name;
        elGetById('modalHomeIconLigatureInput').value = obj.result.data.ligature;
        elGetById('modalHomeIconBgcolorInput').value = obj.result.data.bgcolor;
        elGetById('modalHomeIconColorInput').value = obj.result.data.color;

        populateHomeIconCmdSelect(obj.result.data.cmd, obj.result.data.options[0]);
        elGetById('modalHomeIconCmdInput').value = obj.result.data.cmd;
        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList();
        elGetById('modalHomeIconImageInput').value = obj.result.data.image === ''
            ? tn('Use ligature')
            : obj.result.data.image;
        setData(elGetById('modalHomeIconImageInput'),'value', obj.result.data.image);

        elGetById('modalHomeIconPreview').textContent = obj.result.data.ligature;
        elGetById('modalHomeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        elGetById('modalHomeIconPreview').style.color = obj.result.data.color;

        if (obj.result.data.image === '') {
            elShowId('divHomeIconLigature');
            elGetById('modalHomeIconPreview').style.backgroundImage = '';
        }
        else {
            elHideId('divHomeIconLigature');
            elGetById('modalHomeIconPreview').style.backgroundImage = getCssImageUri(obj.result.data.image);
        }
        //reset ligature selection
        elGetById('modalHomeIconLigatureSearch').value = '';
        elGetById('modalHomeIconSearchCat').value = 'all';
        filterHomeIconLigatures();
        //show modal
        cleanupModalId('modalHomeIcon');
        uiElements.modalHomeIcon.show();
    }, false);
}

/**
 * Saves the home icon
 * @param {Element} target triggering element
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveHomeIcon(target) {
    cleanupModalId('modalHomeIcon');
    btnWaiting(target, true);
    const options = [];
    const optionEls = document.querySelectorAll('#modalHomeIconCmdOptions input, #modalHomeIconCmdOptions select');
    for (const optionEl of optionEls) {
        switch(optionEl.nodeName) {
            case 'SELECT':
                options.push(getSelectValue(optionEl));
                break;
            default:
                options.push(optionEl.value);
        }
    }
    const image = getDataId('modalHomeIconImageInput', 'value');
    const modal = elGetById('modalHomeIcon');
    sendAPI("MYMPD_API_HOME_ICON_SAVE", {
        "replace": getData(modal, 'replace'),
        "oldpos": getData(modal, 'oldpos'),
        "name": elGetById('modalHomeIconNameInput').value,
        "ligature": (image === '' ? elGetById('modalHomeIconLigatureInput').value : ''),
        "bgcolor": elGetById('modalHomeIconBgcolorInput').value,
        "color": elGetById('modalHomeIconColorInput').value,
        "image": image,
        "cmd": elGetById('modalHomeIconCmdInput').value,
        "options": options
    }, modalClose, true);
}

/**
 * Populates the ligatures dropdown
 * @returns {void}
 */
function populateHomeIconLigatures() {
    const modalHomeIconLigatureList = elGetById('modalHomeIconLigatureList');
    const modalHomeIconSearchCat = elGetById('modalHomeIconSearchCat');
    if (modalHomeIconSearchCat.firstChild !== null) {
        return;
    }
    elClear(modalHomeIconLigatureList);
    elClear(modalHomeIconSearchCat);
    modalHomeIconSearchCat.appendChild(
        elCreateTextTn('option', {"value": "all"}, 'icon-all')
    );
    for (const cat in materialIcons) {
        modalHomeIconSearchCat.appendChild(
            elCreateTextTn('option', {"value": cat}, 'icon-' + cat)
        );
        for (const icon of materialIcons[cat]) {
            modalHomeIconLigatureList.appendChild(
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
    elGetById('modalHomeIconLigatureInput').value = el.getAttribute('title');
    elGetById('modalHomeIconPreview').textContent = el.getAttribute('title');
    elGetById('modalHomeIconPreview').style.backgroundImage = '';
    elGetById('modalHomeIconImageInput').value = tn('Use ligature');
    setData(elGetById('modalHomeIconImageInput'), 'value', '');
}

/**
 * Event handler for ligature search
 * @returns {void}
 */
function filterHomeIconLigatures() {
    const str = elGetById('modalHomeIconLigatureSearch').value.toLowerCase();
    const cat = getSelectValueId('modalHomeIconSearchCat');
    const els = document.querySelectorAll('#modalHomeIconLigatureList button');
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
    const modalHomeIconCmdInput = elGetById('modalHomeIconCmdInput');
    elClear(modalHomeIconCmdInput);
    switch(cmd) {
        case 'appGoto': {
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "appGoto"}, 'Goto view')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": ["App", "Tab", "View", "Offset", "Limit", "Filter", "Sort", "Tag", "Search"]});
            break;
        }
        case 'openExternalLink': {
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "openExternalLink"}, 'Open external link')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": ["Uri"]});
            break;
        }
        case 'openModal': {
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "openModal"}, 'Open modal')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": ["Modal"]});
            break;
        }
        case 'execScriptFromOptions': {
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "execScriptFromOptions"}, 'Execute script')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options":["Script", "Arguments"]});
            break;
        }
        default: {
            const options = ["Type"];
            if (type === 'search') {
                options.push('Expression', 'Sort', 'Sortdesc');
            }
            else if (type === 'album') {
                options.push('AlbumId');
            }
            else {
                options.push('Uri');
            }
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "replaceQueue"}, 'Replace queue')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "replacePlayQueue"}, 'Replace queue and play')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
            if (features.featWhence === true) {
                modalHomeIconCmdInput.appendChild(
                    elCreateTextTn('option', {"value": "insertAfterCurrentQueue"}, 'Insert after current playing song')
                );
                setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
            }
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "appendQueue"}, 'Append to queue')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
            modalHomeIconCmdInput.appendChild(
                elCreateTextTn('option', {"value": "appendPlayQueue"}, 'Append to queue and play')
            );
            setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
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
                modalHomeIconCmdInput.appendChild(
                    elCreateTextTn('option', {"value": "homeIconGoto"}, title)
                );
                setData(modalHomeIconCmdInput.lastChild, 'options', {"options": options});
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
    const optionEls = document.querySelectorAll('#modalHomeIconCmdOptions input');
    for (const optionEl of optionEls) {
        oldOptions.push(optionEl.value);
    }
    const modalHomeIconCmdOptions = elGetById('modalHomeIconCmdOptions');
    elClear(modalHomeIconCmdOptions);
    const options = getSelectedOptionDataId('modalHomeIconCmdInput', 'options');
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
            modalHomeIconCmdOptions.appendChild(row);
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
    const selectHomeIconImage = elGetById('modalHomeIconImageInput');
    getImageList(selectHomeIconImage, [{"value": "", "text": tn('Use ligature')}], 'thumbs');
}
