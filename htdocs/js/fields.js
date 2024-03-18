"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module fields_js */

/**
 * Saves the fields for views
 * @param {string} tableName table name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveView(tableName) {
    const params = {
        "view": "view" + tableName,
        "mode": settings['view' + tableName].mode,
        "fields": []
    };
    const fieldsForm = elGetById(tableName + 'FieldsSelect');
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
    const item = elCreateNodes('li', {"class": ["list-group-item", "clickable"], "draggable": "true", "data-field": field}, [
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
    return item;
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
        case 'BrowseRadioWebradiodb':
            return ["Country", "Description", "Genre", "Homepage", "Languages", "Name", "State", "StreamUri", "Codec", "Bitrate", "Thumbnail"];
        case 'BrowseRadioRadiobrowser':
            return ["clickcount", "country", "homepage", "language", "lastchangetime", "lastcheckok", "tags", "url_resolved", "votes"];
        case 'BrowseDatabaseAlbumList': {
            if (settings.albumMode === 'adv') {
                const tags = settings.tagListAlbum.slice();
                tags.push('Discs', 'SongCount', 'Duration', 'Last-Modified');
                if (features.featDbAdded === true) {
                    tags.push('Added');
                }
                return tags.filter(function(value) {
                    return value !== 'Disc';
                });
            }
            else {
                return settings.tagListAlbum;
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
