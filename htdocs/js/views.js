"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module fields_js */

/**
 * Insert the view container
 * @param {string} viewName table name
 * @returns {void}
 */
function setView(viewName) {
    const mode = settings['view' + viewName].mode;
    const curContainer = elGetById(viewName + 'Container');
    const curMode = getData(curContainer, 'viewMode');
    if (curMode === mode) {
        return;
    }
    const newContainer = mode === 'table'
        ? pEl.viewTable.cloneNode(true)
        : pEl.viewGrid.cloneNode(true);
    if (curContainer.parentNode.classList.contains('scrollContainer')) {
        // do not insert a scrolling container in an already scrolling parent
        newContainer.classList.remove('scrollContainer', 'table-responsive');
    }
    newContainer.setAttribute('id', viewName + 'Container');
    newContainer.firstElementChild.setAttribute('id', viewName + 'List');
    curContainer.replaceWith(newContainer);
    setData(newContainer, 'viewMode', mode);
    newContainer.firstElementChild.addEventListener('click', function(event) {
        viewClickHandler(event);
    }, false);
    newContainer.firstElementChild.addEventListener('contextmenu', function(event) {
        viewRightClickHandler(event);
    }, false);
    newContainer.firstElementChild.addEventListener('long-press', function(event) {
        viewRightClickHandler(event);
    }, false);
    if (mode === 'table') {
        //init drag and drop
        switch(viewName) {
            case 'QueueCurrentList':
            case 'BrowsePlaylistDetailList':
                dragAndDropTable(viewName);
                break;
            // No default
        }
    }
}

/**
 * Central click handler for views
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function viewClickHandler(event) {
    if (event.target.classList.contains('row') ||
        event.target.nodeName === 'CAPTION' ||
        event.target.nodeName === 'TH')
    {
        return;
    }
    //select mode
    if (selectEntry(event) === true) {
        return;
    }
    let target = null;
    if (settings['view' + app.id].mode === 'table') {
        // Links
        if (event.target.nodeName === 'A') {
            if (event.target.parentNode.getAttribute('data-col') === 'Action') {
                handleViewActionClick(event);
            }
            else {
                // allow default link action
            }
            return;
        }
        //table body
        target = event.target.closest('TR');
        if (target === null) {
            return;
        }
        if (target.parentNode.nodeName !== 'TBODY' ||
            checkTargetClick(target) === false)
        {
            return;
        }
    }
    else {
        if (event.target.nodeName === 'A') {
            if (event.target.getAttribute('href') !== '#') {
                // allow default link action
                return;
            }
            handleViewActionClick(event);
            return;
        }
        // set target to card
        target = event.target.closest('.card');
    }
    event.preventDefault();
    event.stopPropagation();
    switch(app.id) {
        case 'BrowseDatabaseTagList':
            viewBrowseDatabaseTagListListClickHandler(event, target);
            break;
        case 'BrowseDatabaseAlbumList':
            viewBrowseDatabaseAlbumListListClickHandler(event, target);
            break;
        case 'BrowseDatabaseAlbumDetail':
            viewBrowseDatabaseAlbumDetailListClickHandler(event, target);
            break;
        case 'BrowseFilesystem':
            viewBrowseFilesystemListClickHandler(event, target);
            break;
        case 'BrowsePlaylistList':
            viewPlaylistListListClickHandler(event, target);
            break;
        case 'BrowsePlaylistDetail':
            viewPlaylistDetailListClickHandler(event, target);
            break;
        case 'BrowseRadioFavorites':
            viewBrowseRadioFavoritesListClickHandler(event, target);
            break;
        case 'BrowseRadioRadiobrowser':
            viewBrowseRadioRadiobrowserListClickHandler(event, target);
            break;
        case 'BrowseRadioWebradiodb':
            viewBrowseRadioWebradiodbListClickHandler(event, target);
            break;
        case 'Home':
            viewHomeClickHandler(event, target);
            break;
        case 'QueueCurrent':
            viewQueueCurrentListClickHandler(event, target);
            break;
        case 'QueueJukeboxAlbum':
        case 'QueueJukeboxSong':
            viewQueueJukeboxListClickHandler(event, target);
            break;
        case 'QueueLastPlayed':
            viewQueueLastPlayedListClickHandler(event, target);
            break;
        case 'Search':
            viewSearchListClickHandler(event, target);
            break;
        // No default
    }
}

/**
 * Central right click handler for views
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function viewRightClickHandler(event) {
    if (settings['view' + app.id].mode === 'table') {
        if (event.target.parentNode.classList.contains('not-clickable') ||
            event.target.parentNode.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showContextMenu(event);
    }
    else {
        if (event.target.classList.contains('card-title') ||
            event.target.classList.contains('card-body') ||
            event.target.parentNode.classList.contains('card-body') ||
            event.target.classList.contains('card-footer'))
        {
            showContextMenu(event);
        }
    }
}

/**
 * Handles the click on the actions column
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function handleViewActionClick(event) {
    event.preventDefault();
    const action = event.target.getAttribute('data-action');
    switch(action) {
        case 'popover':
            showContextMenu(event);
            break;
        case 'quickPlay':
            clickQuickPlay(event.target);
            break;
        case 'quickRemove':
            clickQuickRemove(event.target);
            break;
        case 'showSongsByTag': {
            elGetById('SearchSearchStr').value = '';
            const tag = getData(event.target.parentNode.parentNode, 'tag');
            const value = getData(event.target.parentNode.parentNode, 'name');
            gotoSearch(tag, value);
            break;
        }
        case 'showAlbumsByTag':
            elGetById('BrowseDatabaseTagSearchStr').value = '';
            // clear album search input
            elGetById('BrowseDatabaseAlbumListSearchStr').value = '';
            gotoBrowse(event);
            break;
        default:
            logError('Invalid action: ' + action);
    }
}

/**
 * Return an array of pre-generated action links
 * @param {*} userData custom user data
 * @returns {Array} array of dom nodes
 */
function getActionLinks(userData) {
    switch(app.id) {
        case 'BrowsePlaylistDetail':
            return pEl.actionPlaylistDetailIcons;
        case 'QueueCurrent':
            return pEl.actionQueueIcons;
        case 'QueueJukeboxSong':
        case 'QueueJukeboxAlbum':
            return pEl.actionJukeboxIcons;
        case 'BrowseDatabaseTagList':
            if (settings.tagListAlbum.includes(userData)) {
                return pEl.actionMenuBrowseDatabaseTagSongAlbums;
            }
            return pEl.actionMenuBrowseDatabaseTagSongs;
        default:
            return pEl.actionIcons;
    }
}

/**
 * Appends action links as child nodes by app.id
 * @param {Element} container container to append the action links as child nodes
 * @param {*} userData custom user data
 * @returns {void}
 */
function addActionLinks(container, userData) {
    const links = getActionLinks(userData);
    for (const link of links) {
        container.appendChild(link.cloneNode(true));
    }
}

/**
 * Saves the fields for views
 * @param {string} viewName table name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveView(viewName) {
    const modeEl = elGetById('viewSettingsMode');
    const mode = modeEl === null
        ? settings["view" + viewName].mode
        : getBtnGroupValueId('viewSettingsMode');
    const params = {
        "view": "view" + viewName,
        "mode": mode,
        "fields": []
    };
    const fieldsForm = elGetById(viewName + 'FieldsSelect');
    const fields = fieldsForm.querySelector('ul').querySelectorAll('li');
    for (let i = 0, j = fields.length; i < j; i++) {
        params.fields.push(fields[i].getAttribute('data-field'));
    }
    sendAPI("MYMPD_API_VIEW_SAVE", params, function() {
        // refresh the settings
        getSettings(parseSettings);
    }, true);
}

/**
 * Filters the selected fields by available fields
 * @param {string} tableName the table name
 * @returns {void}
 */
function filterFields(tableName) {
    //set available tags
    const fields = setFields(tableName);
    //column name
    const set = "view" + tableName;
    settings[set].fields = settings[set].fields.filter(function(value) {
        return fields.includes(value);
    });
    logDebug('Columns for ' + set + ': ' + settings[set]);
}

/**
 * Creates the list element for fields
 * @param {string} field field name
 * @returns {HTMLElement} li element
 */
function createFieldItem(field) {
    return elCreateNodes('li', {"class": ["list-group-item", "clickable"], "draggable": "true", "data-field": field}, [
        document.createTextNode(tn(field)),
        elCreateNodes('div', {'class': ['btn-group', 'float-end', 'fieldsEnabledBtns']}, [
            elCreateText('button', {"class": ["btn", "btn-sm", "mi", "mi-sm", "pt-0", "pb-0"], 'data-action':'remove', 'title': tn('Remove')}, 'close'),
            elCreateText('button', {"class": ["btn", "btn-sm", "mi", "mi-sm", "pt-0", "pb-0"], 'data-action':'up', 'title': tn('Move up')}, 'arrow_upward'),
            elCreateText('button', {"class": ["btn", "btn-sm", "mi", "mi-sm", "pt-0", "pb-0"], 'data-action':'down', 'title': tn('Move down')}, 'arrow_downward'),
        ]),
        elCreateNodes('div', {'class': ['btn-group', 'float-end', 'fieldsAvailableBtns']}, [
            elCreateText('button', {"class": ["btn", "btn-sm", "mi", "mi-sm", "pt-0", "pb-0"], 'data-action':'add', 'title': tn('Add')}, 'add')
        ])
    ]);
}

/**
 * Handles click events for fields
 * @param {Event} event click event
 * @returns {void}
 */
function fieldClick(event) {
    event.stopPropagation();
    const target = event.target;
    const ul = target.closest('ul');
    const li = target.closest('li');
    if (target.nodeName === 'LI') {
        if (ul.classList.contains('fieldsEnabled')) {
            ul.parentNode.querySelectorAll('ul')[1].appendChild(li);
        }
        else {
            ul.parentNode.querySelector('ul').appendChild(li);
        }
    }
    else if (target.nodeName === 'BUTTON') {
        const action = target.getAttribute('data-action');
        switch(action) {
            case 'remove':
                ul.parentNode.querySelectorAll('ul')[1].appendChild(li);
                break;
            case 'add':
                ul.parentNode.querySelector('ul').appendChild(li);
                break;
            case 'up':
                ul.insertBefore(li, li.previousSibling);
                break;
            case 'down':
                li.nextSibling.after(li);
                break;
            //No Default
        }
    }
}

/**
 * Creates the view settings offcanvas body
 * @param {string} tableName table name
 * @param {HTMLElement} menu element to populate
 * @returns {void}
 */
function setViewOptions(tableName, menu) {
    menu.appendChild(
        elCreateTextTn('h6', {"class": ["dropdown-header"]}, 'Selected')
    );
    const enabledList = elCreateEmpty('ul', {"class": ["list-group", "fieldsEnabled"]});
    for (const field of settings['view' + tableName].fields) {
        enabledList.appendChild(
            createFieldItem(field)
        );
    }
    menu.appendChild(enabledList);
    enabledList.addEventListener('click', function(event) {
        fieldClick(event);
    }, false);
    dragAndDropList(enabledList);
    menu.appendChild(
        elCreateTextTn('h6', {"class": ["dropdown-header","mt-2"]}, 'Available')
    );
    const allFields = setFields(tableName);
    const availableList = elCreateEmpty('ul', {"class": ["list-group", "fieldsAvailable"]});
    for (const field of allFields) {
        if (settings['view' + tableName].fields.includes(field) === true) {
            continue;
        }
        availableList.appendChild(
            createFieldItem(field)
        );
    }
    menu.appendChild(availableList);
    availableList.addEventListener('click', function(event) {
        fieldClick(event);
    }, false);
    dragList(availableList);
}

/**
 * Initializes a list-group for drag of list-items
 * @param {object} list list to enable drag and drop
 * @returns {void}
 */
function dragList(list) {
    list.addEventListener('dragstart', function(event) {
        const target = event.target.nodeName === 'LI'
            ? event.target
            : event.target.closest('li');
        if (target.nodeName === 'LI') {
            target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragEl = target;
        }
    }, false);
}

/**
 * Initializes a list-group for drag and drop of list-items
 * @param {object} list list to enable drag and drop
 * @returns {void}
 */
function dragAndDropList(list) {
    dragList(list);

    list.addEventListener('dragenter', function(event) {
        if (event.target.closest('form') !== dragEl.closest('form')) {
            return;
        }
        const target = event.target.nodeName === 'LI'
            ? event.target
            : event.target.closest('li');
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover');
        }
    }, false);

    list.addEventListener('dragleave', function(event) {
        if (event.target.closest('form') !== dragEl.closest('form')) {
            return;
        }
        const target = event.target.nodeName === 'LI'
            ? event.target
            : event.target.closest('li');
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.remove('dragover');
        }
    }, false);

    list.addEventListener('dragover', function(event) {
        if (event.target.closest('form') !== dragEl.closest('form')) {
            event.dataTransfer.dropEffect = 'none';
            return;
        }
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
    }, false);

    list.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.closest('form') !== dragEl.closest('form')) {
            return;
        }
        if (dragEl === undefined ||
            dragEl.nodeName !== 'LI')
        {
            return;
        }
        const target = event.target.nodeName === 'LI'
            ? event.target
            : event.target.closest('li');
        target.classList.remove('dragover');
        const newField = getData(target, 'field');
        const oldField = getData(dragEl, 'field');
        if (oldField === newField) {
            return;
        }
        target.parentNode.insertBefore(dragEl, target);
    }, false);

    list.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
}

/**
 * Sets the available fields
 * @param {string} tableName table name
 * @returns {object} array of available columns
 */
function setFields(tableName) {
    switch(tableName) {
        case 'BrowsePlaylistList':
            return ["Type", "Name", "Last-Modified", "Thumbnail"];
        case 'BrowseRadioFavorites':
        case 'BrowseRadioWebradiodb':
            return ["Country", "Description", "Genre", "Homepage", "Languages", "Name", "State", "StreamUri", "Codec", "Bitrate", "Thumbnail"];
        case 'BrowseRadioRadiobrowser':
            return ["clickcount", "country", "homepage", "language", "lastchangetime", "lastcheckok", "tags", "url_resolved", "votes"];
        case 'BrowseDatabaseTagList':
            return ["Value", "Thumbnail"];
        case 'BrowseDatabaseAlbumList': {
            if (settings.albumMode === 'adv') {
                const tags = settings.tagListAlbum.slice();
                tags.push('Discs', 'SongCount', 'Duration', 'Last-Modified', 'Thumbnail');
                if (features.featDbAdded === true) {
                    tags.push('Added');
                }
                return tags.filter(function(value) {
                    return value !== 'Disc';
                });
            }
            else {
                const tags = settings.tagListAlbum.slice();
                tags.push('Thumbnail');
                return tags;
            }
        }
        case 'BrowseDatabaseAlbumDetailInfo': {
            if (settings.albumMode === 'adv') {
                const tags = settings.tagListAlbum.slice();
                tags.push('Discs', 'SongCount', 'Duration', 'Last-Modified');
                if (features.featDbAdded === true) {
                    tags.push('Added');
                }
                return tags.filter(function(value) {
                    return value !== 'Disc' &&
                        value !== 'Album';
                });
            }
            else {
                return settings.tagListAlbum;
            }
        }
        case 'QueueJukeboxAlbum': {
            const tags = settings.tagListAlbum.slice();
            tags.push('Pos', 'Discs', 'SongCount', 'Duration', 'Last-Modified');
            if (features.featDbAdded === true) {
                tags.push('Added');
            }
            return tags.filter(function(value) {
                return value !== 'Disc';
            });
        }
        // No Default
    }

    const tags = settings.tagList.slice();
    if (features.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration', 'Last-Modified', 'Filetype');
    if (tableName !== 'Playback') {
        tags.push('Thumbnail');
    }
    if (features.featDbAdded === true) {
        tags.push('Added');
    }

    switch(tableName) {
        case 'QueueCurrent':
            tags.push('AudioFormat', 'Priority');
            //fall through
        case 'BrowsePlaylistDetail':
        case 'QueueJukeboxSong':
            tags.push('Pos');
            break;
        case 'BrowseFilesystem':
            tags.push('Type', 'Filename');
            break;
        case 'Playback':
            tags.push('AudioFormat');
            if (features.featLyrics === true) {
                tags.push('Lyrics');
            }
            break;
        case 'QueueLastPlayed':
            tags.push('Pos', 'LastPlayed');
            break;
        // No Default
    }
    //sort tags 
    tags.sort();
    //append stickers
    if (features.featStickers === true) {
        for (const sticker of stickerList) {
            tags.push(sticker);
        }
    }
    return tags;
}

/**
 * Sets the data from the jsonrpc object to the dom node and
 * updates the jsonrpc object
 * @param {Element} entry Dom node representing the entry
 * @param {object} data Object data from jsonrpc response
 * @returns {void}
 */
function setEntryData(entry, data) {
    //set AlbumId
    if (data.AlbumId !== undefined) {
        setData(entry, 'AlbumId', data.AlbumId);
    }
    //and browse tags
    for (const tag of settings.tagListBrowse) {
        if (albumFilters.includes(tag) &&
            isEmptyTag(data[tag]) === false)
        {
            setData(entry, tag, data[tag]);
        }
    }
    //set Title to name if not defined - for folders and playlists
    if (data.Title === undefined) {
        data.Title = data.name;
    }

    //set Filetype
    if (data.Filetype === undefined) {
        data.Filetype = filetype(data.uri, false);
    }
    //set Thumbnail
    switch(data.Type) {
        case 'album':
            data.Thumbnail = getCssImageUri(data.FirstSongUri !== 'albumid'
                ? '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(data.FirstSongUri)
                : '/albumart-thumb/' + data.AlbumId);
            break;
        case 'song':
        case 'stream':
        case 'webradio':
            data.Thumbnail = getCssImageUri('/albumart?offset=0&uri=' + myEncodeURIComponent(data.uri));
            break;
        case 'dir': 
            data.Thumbnail = getCssImageUri('/folderart?path=' + myEncodeURIComponent(data.uri));
            break;
        case 'plist':
        case 'smartpls':
            data.Thumbnail = getCssImageUri('/playlistart?playlist=' + myEncodeURIComponent(data.uri));
            break;
        case 'webradiodb':
            data.Thumbnail = getCssImageUri(webradioDbPicsUri + data.Image);
            break;
        // No Default
    }
}
