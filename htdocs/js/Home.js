"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module Home_js */

/**
 * Handles home
 * @returns {void}
 */
function handleHome() {
    sendAPI("MYMPD_API_HOME_ICON_LIST", {}, parseHomeIcons, false);
}

/**
 * Initializes the home feature elements
 * @returns {void}
 */
function initHome() {
    //home screen
    document.getElementById('HomeList').addEventListener('click', function(event) {
        if (event.target.classList.contains('card-body')) {
            const href = getData(event.target.parentNode, 'href');
            if (href !== undefined) {
               parseCmd(event, href);
            }
        }
        else if (event.target.classList.contains('card-footer')){
            showContextMenu(event);
        }
    }, false);

    document.getElementById('HomeList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('card-body') ||
            event.target.classList.contains('card-footer'))
        {
            showContextMenu(event);
        }
    }, false);

    document.getElementById('HomeList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('card-body') ||
            event.target.classList.contains('card-footer'))
        {
            showContextMenu(event);
        }
    }, false);

    dragAndDropHome();

    //modals
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
 * Returns the friendly type of the home icon
 * @param {string} cmd the command
 * @param {string} action action of the command
 * @returns {string} friendly type
 */
function getHomeIconType(cmd, action) {
    switch(cmd) {
        case 'appGoto':
        case 'execScriptFromOptions':
        case 'openExternalLink':
        case 'openModal':
            return typeFriendly[cmd];
        default:
            return typeFriendly[action];
    }
}

/**
 * Parses the MYMPD_API_HOME_ICON_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseHomeIcons(obj) {
    const cardContainer = document.getElementById('HomeList');
    unsetUpdateView(cardContainer);
    const cols = cardContainer.querySelectorAll('.col');

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["ms-3", "mb-3", "not-clickable", "alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        return;
    }
    if (cols.length === 0) {
        elClear(cardContainer);
    }
    if (obj.result.returnedEntities === 0) {
        elClear(cardContainer);
        const div = elCreateNodes('div', {"class": ["px-3", "py-1"]}, [
            elCreateTextTn('h3', {}, 'Homescreen'),
            elCreateNodes('p', {}, [
                document.createTextNode(tn('Homescreen welcome')),
                elCreateText('span', {"class": ["mi"]}, 'add_to_home_screen'),
                document.createTextNode(' '),
                elCreateText('span', {"class": ["mi"]}, 'library_add')
            ])
        ]);
        cardContainer.appendChild(div);
        return;
    }
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const homeType = getHomeIconType(obj.result.data[i].cmd, obj.result.data[i].options[0]);
        const actionType = friendlyActions[obj.result.data[i].cmd];

        // second option must be an array
        if (obj.result.data[i].options[1] !== undefined) {
            obj.result.data[i].options[1] = [obj.result.data[i].options[1]];
        }

        const col = elCreateEmpty('div', {"class": ["col", "px-0", "flex-grow-0"]});
        const card = elCreateEmpty('div', {"data-contextmenu": "home", "class": ["card", "home-icons"], "draggable": "true",
            "title": tn(homeType) + ':' + smallSpace + obj.result.data[i].name +
            '\n' + tn(actionType)});
        //decode json options
        for (let j = 0, k = obj.result.data[i].options.length; j < k; j++) {
            if (obj.result.data[i].options[j].indexOf('{"') === 0 ||
                obj.result.data[i].options[j].indexOf('["') === 0)
            {
                obj.result.data[i].options[j] = JSON.parse(obj.result.data[i].options[j]);
            }
        }

        setData(card, 'href', {"cmd": obj.result.data[i].cmd, "options": obj.result.data[i].options});
        setData(card, 'pos', i);
        const cardBody = elCreateText('div', {"class": ["card-body", "mi", "rounded", "clickable"]}, obj.result.data[i].ligature);
        if (obj.result.data[i].image !== '') {
            cardBody.style.backgroundImage = getCssImageUri(obj.result.data[i].image);
        }
        if (obj.result.data[i].bgcolor !== '') {
            cardBody.style.backgroundColor = obj.result.data[i].bgcolor;
        }
        if (obj.result.data[i].color !== '' &&
            obj.result.data[i].color !== undefined)
        {
            cardBody.style.color = obj.result.data[i].color;
        }
        card.appendChild(cardBody);
        card.appendChild(
            elCreateText('div', {"class": ["card-footer", "card-footer-grid", "p-2", "clickable"]}, obj.result.data[i].name)
        );
        col.appendChild(card);
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
    }
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }
    setScrollViewHeight(cardContainer);
}

/**
 * Shows the dragover tip
 * @param {EventTarget} from from element
 * @param {EventTarget} to to element
 * @returns {void}
 */
function showDropoverIcon(from, to) {
    const fromPos = getData(from, 'pos');
    const toPos = getData(to, 'pos');
    if (toPos > fromPos) {
        to.classList.add('dragover-icon-right');
    }
    else {
        to.classList.add('dragover-icon-left');
    }
    to.classList.add('dragover-icon');
}

/**
 * Hides the dragover tip
 * @param {EventTarget} el element
 * @returns {void}
 */
function hideDropoverIcon(el) {
    el.classList.remove('dragover-icon-left', 'dragover-icon-right');
}

/**
 * Drag and drop event handler
 * @returns {void}
 */
function dragAndDropHome() {
    const HomeList = document.getElementById('HomeList');

    HomeList.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('home-icons')) {
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragEl = event.target;
        }
    }, false);

    HomeList.addEventListener('dragenter', function(event) {
        if (dragEl !== undefined &&
            event.target.classList.contains('home-icons'))
        {
            showDropoverIcon(dragEl, event.target);
        }
    }, false);

    HomeList.addEventListener('dragleave', function(event) {
        if (dragEl !== undefined &&
            event.target.classList.contains('home-icons'))
        {
            hideDropoverIcon(event.target);
        }
    }, false);

    HomeList.addEventListener('dragover', function(event) {
        // prevent default to allow drop
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
    }, false);

    HomeList.addEventListener('drop', function(event) {
        event.preventDefault();
        event.stopPropagation();
        
        const target = event.target.classList.contains('card-body')
            ? event.target.parentNode
            : event.target;
        if (target.classList.contains('home-icons')) {
            hideDropoverIcon(target);
            const to = getData(target, 'pos');
            const from = getData(dragEl, 'pos');
            if (isNaN(to) === false &&
                isNaN(from) === false &&
                from !== to)
            {
                sendAPI("MYMPD_API_HOME_ICON_MOVE", {"from": from, "to": to}, null, false);
            }
        }
    }, false);

    HomeList.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
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
 * Executes the home icon action
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function executeHomeIcon(pos) {
    const el = document.getElementById('HomeList').children[pos].firstChild;
    parseCmd(null, getData(el, 'href'));
}

/**
 * Adds a link to a view to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addViewToHome() {
    _addHomeIcon('appGoto', '', 'preview', '', [app.current.card, app.current.tab, app.current.view,
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search]);
}

/**
 * Adds a modal shortcut to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addOpenModalToHome() {
    _addHomeIcon('openModal', '', 'web_asset', '', []);
}

/**
 * Adds a external link to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addExternalLinkToHome() {
    _addHomeIcon('openExternalLink', '', 'link', '', []);
}

/**
 * Adds a script to the homescreen
 * @param {string} name name for the home icon
 * @param {object} script script object
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addScriptToHome(name, script) {
    const options = [script.script, script.arguments.join(',')];
    _addHomeIcon('execScriptFromOptions', name, 'code', '', options);
}

/**
 * Adds a playlist to the homescreen
 * @param {string} uri uri of the playlist
 * @param {string} type one of plist, smartpls
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addPlistToHome(uri, type, name) {
    _addHomeIcon('replaceQueue', name, 'list', '', [type, uri]);
}

/**
 * Adds a webradio favorite to the homescreen
 * @param {string} uri uri of the webradio favorite
 * @param {string} type must be webradio
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addRadioFavoriteToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

/**
 * Adds a webradioDB entry to the homescreen
 * @param {string} uri uri of the stream
 * @param {string} type must be stream
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addWebRadiodbToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

/**
 * Adds a directory to the homescreen
 * @param {string} uri directory uri
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addDirToHome(uri, name) {
    if(uri === undefined) {
        uri = app.current.search;
        name = basename(app.current.search, false);
    }
    _addHomeIcon('replaceQueue', name, 'folder_open', '', ['dir', uri]);
}

/**
 * Adds a song or stream to the homescreen
 * @param {string} uri song or stream uri
 * @param {string} type one of song, stream
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSongToHome(uri, type, name) {
    const ligature = type === 'song' ? 'music_note' : 'stream';
    _addHomeIcon('replaceQueue', name, ligature, '', [type, uri]);
}

/**
 * Adds the current search to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSearchToHome() {
    _addHomeIcon('replaceQueue', tn('Current search'), 'saved_search', '', ['search', app.current.search]);
}

/**
 * Adds an album to the homescreen
 * @param {string} albumId the albumid
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAlbumToHome(albumId, name, image) {
    if (image === '') {
        _addHomeIcon('replaceQueue', name, 'album', '', ['album', albumId]);
    }
    else {
        _addHomeIcon('replaceQueue', name, '', image, ['album', albumId]);
    }
}

/**
 * Adds a new stream to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addStreamToHome() {
    const mode = getRadioBoxValueId('addToPlaylistPos');
    const uri = document.getElementById('streamUrl').value;
    let action;
    switch(mode) {
        case 'append': action = 'appendQueue'; break;
        case 'appendPlay': action = 'appendPlayQueue'; break;
        case 'insertAfterCurrent': action = 'insertAfterCurrentQueue'; break;
        case 'insertPlayAfterCurrent': action = 'insertPlayAfterCurrentQueue'; break;
        case 'replace': action = 'replaceQueue'; break;
        case 'replacePlay': action = 'replacePlayQueue'; break;
    }
    _addHomeIcon(action, '', 'stream', '', ['stream', uri]);
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
    document.getElementById('modalEditHomeIconTitle').textContent = tn('Add to homescreen');
    document.getElementById('inputHomeIconReplace').value = 'false';
    document.getElementById('inputHomeIconOldpos').value = '0';
    document.getElementById('inputHomeIconName').value = name;
    document.getElementById('inputHomeIconBgcolor').value = '#28a745';
    document.getElementById('inputHomeIconColor').value = '#ffffff';

    populateHomeIconCmdSelect(cmd, options[0]);
    document.getElementById('selectHomeIconCmd').value = cmd;
    elClearId('divHomeIconOptions');
    showHomeIconCmdOptions(options);
    getHomeIconPictureList();
    const homeIconPreviewEl = document.getElementById('homeIconPreview');
    const homeIconImageInput = document.getElementById('inputHomeIconImage');
    if (image !== '') {
        homeIconImageInput.value = image;
        setData(homeIconImageInput, 'value', image);
        document.getElementById('inputHomeIconLigature').value = '';
        elClear(homeIconPreviewEl);
        homeIconPreviewEl.style.backgroundImage = getCssImageUri(image);
        elHideId('divHomeIconLigature');
    }
    else {
        //use ligature
        homeIconImageInput.value = tn('Use ligature');
        setData(homeIconImageInput, 'value', '');
        document.getElementById('inputHomeIconLigature').value = ligature;
        homeIconPreviewEl.textContent = ligature;
        homeIconPreviewEl.style.backgroundImage = '';
        elShowId('divHomeIconLigature');
    }

    homeIconPreviewEl.style.backgroundColor = '#28a745';
    homeIconPreviewEl.style.color = '#ffffff';
    uiElements.modalEditHomeIcon.show();
}

/**
 * Duplicates a home icon
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function duplicateHomeIcon(pos) {
    _editHomeIcon(pos, false, "Duplicate home icon");
}

/**
 * Opens the edit home icon dialog
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function editHomeIcon(pos) {
    _editHomeIcon(pos, true, "Edit home icon");
}

/**
 * The real edit home icon function
 * @param {number} pos home icon position
 * @param {boolean} replace true = replace existing home icon, false = duplicate home icon
 * @param {string} title title for the modal
 * @returns {void}
 */
function _editHomeIcon(pos, replace, title) {
    document.getElementById('modalEditHomeIconTitle').textContent = tn(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        document.getElementById('inputHomeIconReplace').value = replace;
        document.getElementById('inputHomeIconOldpos').value = pos;
        document.getElementById('inputHomeIconName').value = obj.result.data.name;
        document.getElementById('inputHomeIconLigature').value = obj.result.data.ligature;
        document.getElementById('inputHomeIconBgcolor').value = obj.result.data.bgcolor;
        document.getElementById('inputHomeIconColor').value = obj.result.data.color;

        populateHomeIconCmdSelect(obj.result.data.cmd, obj.result.data.options[0]);
        document.getElementById('selectHomeIconCmd').value = obj.result.data.cmd;
        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList();
        document.getElementById('inputHomeIconImage').value = obj.result.data.image === '' ? tn('Use ligature') : obj.result.data.image;
        setData(document.getElementById('inputHomeIconImage'),'value', obj.result.data.image);

        document.getElementById('homeIconPreview').textContent = obj.result.data.ligature;
        document.getElementById('homeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        document.getElementById('homeIconPreview').style.color = obj.result.data.color;

        if (obj.result.data.image === '') {
            elShowId('divHomeIconLigature');
            document.getElementById('homeIconPreview').style.backgroundImage = '';
        }
        else {
            elHideId('divHomeIconLigature');
            document.getElementById('homeIconPreview').style.backgroundImage = getCssImageUri(obj.result.data.image);
        }
        //reset ligature selection
        document.getElementById('searchHomeIconLigature').value = '';
        document.getElementById('searchHomeIconCat').value = 'all';
        filterHomeIconLigatures();
        //show modal
        cleanupModalId('modalEditHomeIcon');
        uiElements.modalEditHomeIcon.show();
    }, false);
}

/**
 * Saves the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveHomeIcon() {
    cleanupModalId('modalEditHomeIcon');
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
        uiElements.modalEditHomeIcon.hide();
    }
}

/**
 * Deletes a home icon
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deleteHomeIcon(pos) {
    sendAPI("MYMPD_API_HOME_ICON_RM", {"pos": pos}, null, false);
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
    }
    return elCreateEmpty('input', {"class": ["form-control", "border-secondary"], "name": name, "value": value});
}

/**
 * Populates the picture list in the home icon edit modal
 * @returns {void}
 */
function getHomeIconPictureList() {
    const selectHomeIconImage = document.getElementById('inputHomeIconImage');
    getImageList(selectHomeIconImage, [{"value": "", "text": tn('Use ligature')}], 'thumbs');
}

/**
 * Opens the link in a new window
 * @param {string} link uri to open
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function openExternalLink(link) {
    window.open(link);
}

/**
 * Goto handler for home icons
 * @param {string} type one of dir, search, album, plist, smartpls
 * @param {string} uri type = search: search expression,
 *                     type = album: album id,
 *                     else uri of directory or playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function homeIconGoto(type, uri) {
    switch(type) {
        case 'dir':
            gotoFilesystem(uri[0], type);
            break;
        case 'search':
            appGoto('Search', undefined, undefined, 0, undefined, 'any', {'tag': 'Title', 'desc': false}, '-', uri[0]);
            break;
        case 'album':
            //uri = AlbumId
            gotoAlbum(uri[0]);
            break;
        case 'plist':
        case 'smartpls':
            playlistDetails(uri[0]);
            break;
    }
}
