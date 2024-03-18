"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module tables_js */

/**
 * Switches the select mode of current displayed table
 * @param {EventTarget} target triggering button
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function switchTableMode(target) {
    const table = elGetById(app.id + 'List');
    const mode = table.getAttribute('data-mode');

    if (mode === null) {
        table.setAttribute('data-mode', 'select');
        target.classList.add('selected');
        target.classList.remove('rounded-end');
        target.nextElementSibling.classList.remove('d-none');
    }
    else {
        table.removeAttribute('data-mode');
        target.classList.remove('selected');
        target.classList.add('rounded-end');
        target.nextElementSibling.classList.add('d-none');
        selectAllRows(table, false);
    }
}

/**
 * Selects all rows in table body
 * @param {HTMLElement} table table element
 * @param {boolean} select true = select all rows, false = clear selection
 * @returns {void}
 */
function selectAllRows(table, select) {
    const rows = table.querySelectorAll('tbody > tr');
    let firstType = undefined;
    for (const row of rows) {
        if (row.lastElementChild.lastElementChild !== null) {
            firstType = getData(row, 'type');
            break;
        }
    }
    for (const row of rows) {
        const check = row.lastElementChild.lastElementChild;
        if (check === null ||
            row.classList.contains('not-clickable') ||
            (getData(row, 'type') !== firstType && select === true))
        {
            continue;
        }
        if (select === true) {
            row.classList.add('selected');
            check.textContent = ligatures['checked'];
        }
        else {
            row.classList.remove('selected');
            check.textContent = ligatures['unchecked'];
        }
    }
    showTableSelectionCount();
}

/**
 * Checks if table is in select mode and selects the row(s)
 * @param {Event} event triggering event
 * @returns {boolean} true if table in select mode, else false
 */
function selectRow(event) {
    const table = event.target.closest('TABLE');
    const mode = table.getAttribute('data-mode');
    if (event.ctrlKey &&
        mode === null)
    {
        //enable select mode
        switchTableMode(elGetById(app.id + 'SelectModeBtn'));
    }
    else if (mode === null) {
        return false;
    }
    //in row select mode
    const row = event.target.closest('TR');
    if (row.classList.contains('not-clickable') &&
        event.target.parentNode.nodeName !== 'TH') {
        return true;
    }
    if (event.target.parentNode.nodeName === 'TH') {
        const select = event.target.textContent === ligatures['unchecked']
            ? true
            : false;
        event.target.textContent = select === true
            ? ligatures['checked']
            : ligatures['unchecked'];
        selectAllRows(table, select);
    }
    else if (event.shiftKey) {
        let lastPos = getData(table, 'last-selected');
        if (lastPos === undefined) {
            lastPos = 0;
        }
        const pos = elGetIndex(row);
        setData(table, 'last-selected', pos);
        let first;
        let last;
        if (lastPos < pos) {
            first = lastPos;
            last = pos;
        }
        else {
            first = pos;
            last = lastPos;
        }
        const rows = table.querySelector('tbody').querySelectorAll('tr');
        const firstType = getData(rows[first], 'type');
        for (let i = first; i <= last; i++) {
            if (getData(rows[i], 'type') !== firstType) {
                continue;
            }
            selectSingleRow(rows[i], true);
        }
    }
    else {
        selectSingleRow(row, null);
        setData(table, 'last-selected', elGetIndex(row));
    }
    showTableSelectionCount();
    event.preventDefault();
    event.stopPropagation();
    return true;
}

/**
 * Selects / unselects a single row
 * @param {HTMLElement} row row to select or unselect
 * @param {boolean} [select] true = select, false = unselect, null = toggle
 * @returns {void}
 */
function selectSingleRow(row, select) {
    const check = row.lastElementChild.lastElementChild;
    if (check === null) {
        return;
    }
    if ((select === null && row.classList.contains('selected')) ||
        select === false)
    {
        check.textContent = ligatures['unchecked'];
        row.classList.remove('selected');
    }
    else {
        check.textContent = ligatures['checked'];
        row.classList.add('selected');
    }
}

/**
 * Shows the number of selections in the dropdown
 * @returns {void}
 */
function showTableSelectionCount() {
    const table = elGetById(app.id + 'List');
    const dropdown = document.querySelector('#' + app.id + 'SelectionDropdown');
    const rows = table.querySelectorAll('tbody > tr.selected');
    const count = rows.length;
    let validSelection = true;
    if (count > 1) {
        const firstType = getData(rows[0], 'type');
        for (const row of rows) {
            if (getData(row, 'type') !== firstType) {
                validSelection = false;
                break;
            }
        }
    }
    if (validSelection === true) {
        dropdown.querySelector('small').textContent = count + ' ' + tn('selected');
    }
    else {
        dropdown.querySelector('small').textContent = tn('Invalid selection');
    }
    const btns = dropdown.querySelectorAll('button');
    for (const btn of btns) {
        if (count === 0 ||
            validSelection === false)
        {
            btn.setAttribute('disabled', 'disabled');
        }
        else {
            btn.removeAttribute('disabled');
        }
    }
}

/**
 * Initializes a table body for drag and drop of rows
 * @param {string} tableId table id
 * @returns {void}
 */
function dragAndDropTable(tableId) {
    const tableBody = document.querySelector('#' + tableId + ' > tbody');
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragEl = event.target;
        }
    }, false);

    tableBody.addEventListener('dragenter', function(event) {
        const target = event.target.nodeName === 'TD'
            ? event.target.parentNode
            : event.target;
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover');
        }
    }, false);

    tableBody.addEventListener('dragleave', function(event) {
        const target = event.target.nodeName === 'TD'
            ? event.target.parentNode
            : event.target;
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.remove('dragover');
        }
    }, false);

    tableBody.addEventListener('dragover', function(event) {
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
    }, false);

    tableBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined ||
            dragEl.nodeName !== 'TR')
        {
            return;
        }
        const target = event.target.closest('TR');
        target.classList.remove('dragover');
        const newSongPos = getData(target, 'pos');
        const oldSongPos = getData(dragEl, 'pos');
        if (oldSongPos === newSongPos) {
            return;
        }
        // set dragged element uri to undefined to force table row replacement
        setData(dragEl, 'uri', undefined);
        elHide(dragEl);
        // apply new order
        setUpdateViewId(tableId);
        switch(app.id) {
            case 'QueueCurrent': {
                queueMoveSong(oldSongPos, newSongPos);
                break;
            }
            case 'BrowsePlaylistDetail': {
                currentPlaylistMoveSong(oldSongPos, newSongPos);
                break;
            }
            // No Default
        }
    }, false);

    tableBody.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
}

/**
 * Sets the available table columns
 * @param {string} tableName table name
 * @returns {object} array of available columns
 */
function setColTags(tableName) {
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
 * Creates the select columns checkbox list
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
    const allTags = setColTags(tableName);
    const availableList = elCreateEmpty('ul', {"class": ["list-group", "fieldsAvailable"]});
    for (const field of allTags) {
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
 * Filters the selected column by available tags
 * @param {string} tableName the table name
 * @returns {void}
 */
function filterCols(tableName) {
    //set available tags
    const tags = setColTags(tableName);
    //column name
    const set = "view" + tableName;
    settings[set].fields = settings[set].fields.filter(function(value) {
        return tags.includes(value);
    });
    logDebug('Columns for ' + set + ': ' + settings[set]);
}

/**
 * Checks if a table column is sortable
 * @param {string} tableName name of the table
 * @param {string} colName name of the column
 * @returns {boolean} true if clickable, else false
 */
function isColSortable(tableName, colName) {
    if (tableName === 'QueueCurrent' &&
        features.featAdvqueue === false)
    {
        return false;
    }
    if (tableName !== 'Search' &&
        tableName !== 'QueueCurrent')
    {
        return false;
    }
    // @ts-ignore
    if (colName === 'Duration' ||
        colName === 'AudioFormat' ||
        // @ts-ignore
        stickerList.includes(colName) === true)
    {
        return false;
    }
    return true;
}

/**
 * Sets the table header columns
 * @param {string} tableName table name
 * @returns {void}
 */
function setCols(tableName) {
    if (tableName === 'Search' &&
        app.cards.Search.sort.tag === 'Title')
    {
        if (settings.tagList.includes('Title')) {
            app.cards.Search.sort.tag = 'Title';
        }
        else if (features.featTags === false) {
            app.cards.Search.sort.tag = 'Filename';
        }
        else {
            app.cards.Search.sort.tag = '-';
        }
    }

    const thead = document.querySelector('#' + tableName + 'List > thead > tr');
    elClear(thead);

    for (let i = 0, j = settings['view' + tableName].fields.length; i < j; i++) {
        const hname = settings['view' + tableName].fields[i];
        const clickable = isColSortable(tableName, hname)
            ? 'clickable'
            : 'not-clickable';
        const th = elCreateTextTn('th', {"class": [clickable], "draggable": "true", "data-col": settings['view' + tableName].fields[i]}, hname);
        thead.appendChild(th);

        const sort = tableName === 'Search'
            ? app.cards.Search.sort
            : tableName === 'BrowseRadioWebradiodb'
                ? app.cards.Browse.tabs.Radio.views.Webradiodb.sort
                : tableName === 'QueueCurrent'
                    ? app.cards.Queue.tabs.Current.sort
                    : undefined;
        if ((tableName === 'Search' && hname === sort.tag) ||
            (tableName === 'BrowseRadioWebradiodb' && hname === sort.tag) ||
            (tableName === 'QueueCurrent' && hname === sort.tag)
           )
        {
            addSortIndicator(th, sort.desc);
        }
    }
    //append action column
    const th = elCreateEmpty('th', {"data-col": "Action"});
    th.appendChild(
        pEl.selectAllBtn.cloneNode(true)
    );
    thead.appendChild(th);
}

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
    const colsDropdown = elGetById(tableName + 'FieldsSelect');
    const colInputs = colsDropdown.querySelector('ul').querySelectorAll('li');
    for (let i = 0, j = colInputs.length; i < j; i++) {
        params.fields.push(colInputs[i].getAttribute('data-field'));
    }
    sendAPI("MYMPD_API_VIEW_SAVE", params, saveViewCheckError, true);
}

/**
 * Handles the jsonrpc response for MYMPD_API_VIEW_SAVE
 * @returns {void}
 */
function saveViewCheckError() {
    // refresh the settings
    getSettings(parseSettings);
}

/**
 * Toggles the sorting of the table
 * @param {EventTarget} th clicked table header column
 * @param {string} colName column name
 * @returns {void}
 */
function toggleSort(th, colName) {
    if (th.nodeName !== 'TH' ||
        th.textContent === '' ||
        th.getAttribute('data-col') === 'Action')
    {
        return;
    }

    if (app.current.sort.tag === colName) {
        //toggle sort direction
        app.current.sort.desc = app.current.sort.desc === false
            ? true
            : false;
    }
    else {
        //sort by new colum ascending
        app.current.sort.desc = false;
        app.current.sort.tag = colName;
    }
    addSortIndicator(th, app.current.sort.desc);
}

/**
 * Add the sort indicator and removes old ones.
 * @param {HTMLElement | EventTarget} th header element
 * @param {boolean} desc descending?
 * @returns {void}
 */
function addSortIndicator(th, desc) {
    // remove old sort indicator
    const oldIndicators = th.parentNode.querySelectorAll('.sort-dir');
    for (const i of oldIndicators) {
        i.classList.remove('sort-dir', 'sort-desc', 'sort-asc');
    }
    const order = desc === false
        ? 'asc'
        : 'desc';
    // add new sort indicator
    th.classList.add('sort-dir', 'sort-' + order);
}

/**
 * Replaces a table row and tries to keep the selection state
 * @param {boolean} mode the selection mode
 * @param {HTMLElement} row row to replace
 * @param {HTMLElement} el replacement row
 * @returns {void}
 */
function replaceTblRow(mode, row, el) {
    if (getData(row, 'uri') === getData(el, 'uri') &&
        mode === true &&
        row.lastElementChild.lastElementChild.textContent === ligatures.checked)
    {
        el.lastElementChild.lastElementChild.textContent = ligatures.checked;
        el.classList.add('selected');
    }
    row.replaceWith(el);
}

/**
 * Adds a row with discnumber to the table
 * @param {number} disc discnumber
 * @param {string} albumId the albumid
 * @param {number} colspan column count
 * @returns {HTMLElement} the created row
 */
function addDiscRow(disc, albumId, colspan) {
    const row = elCreateNodes('tr', {"class": ["not-clickable"]}, [
        elCreateNode('td', {},
            elCreateText('span', {"class": ["mi"]}, 'album')
        ),
        elCreateTextTnNr('td', {"colspan": (colspan - 1)}, 'Discnum', disc),
        elCreateNode('td', {"data-col": "Action"},
            elCreateText('a', {"data-action": "popover", "data-contextmenu": "disc", "href": "#", "class": ["mi", "color-darkgrey"],
                "data-title-phrase":"Actions"}, ligatures['more'])
        )
    ]);
    setData(row, 'Disc', disc);
    setData(row, 'AlbumId', albumId);
    return row;
}

/**
 * Updates the table from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list table name to populate
 * @param {Function} [perRowCallback] callback per row
 * @param {Function} [createRowCellsCallback] callback to create the row
 * @returns {void}
 */
function updateTable(obj, list, perRowCallback, createRowCellsCallback) {
    const table = elGetById(list + 'List');
    const mode = table.getAttribute('data-mode') === 'select' 
        ? true
        : false;
    const tbody = table.querySelector('tbody');
    const colspan = settings['view' + list] !== undefined
        ? settings['view' + list].length
        : 0;

    const nrItems = obj.result.returnedEntities;
    let tr = tbody.querySelectorAll('tr');
    const smallWidth = uiSmallWidthTagRows();

    if (smallWidth === true) {
        table.classList.add('smallWidth');
    }
    else {
        table.classList.remove('smallWidth');
    }

    //disc handling for album view
    let z = 0;
    let lastDisc = obj.result.data.length > 0 && obj.result.data[0].Disc !== undefined
        ? Number(obj.result.data[0].Disc)
        : 0;
    if (obj.result.Discs !== undefined &&
        obj.result.Discs > 1)
    {
        const row = addDiscRow(1, obj.result.AlbumId, colspan);
        if (z < tr.length) {
            replaceTblRow(mode, tr[z], row);
        }
        else {
            tbody.append(row);
        }
        z++;
    }
    for (let i = 0; i < nrItems; i++) {
        //disc handling for album view
        if (obj.result.data[0].Disc !== undefined &&
            lastDisc < Number(obj.result.data[i].Disc))
        {
            const row = addDiscRow(obj.result.data[i].Disc, obj.result.AlbumId, colspan);
            if (i + z < tr.length) {
                replaceTblRow(mode, tr[i + z], row);
            }
            else {
                tbody.append(row);
            }
            z++;
            lastDisc = obj.result.data[i].Disc;
        }
        const row = elCreateEmpty('tr', {});
        if (perRowCallback !== undefined &&
            typeof(perRowCallback) === 'function')
        {
            perRowCallback(row, obj.result.data[i]);
        }
        //data row
        //set AlbumId
        if (obj.result.data[i].AlbumId !== undefined) {
            setData(row, 'AlbumId', obj.result.data[i].AlbumId);
        }
        //and browse tags
        for (const tag of settings.tagListBrowse) {
            if (albumFilters.includes(tag) &&
                isEmptyTag(obj.result.data[i][tag]) === false)
            {
                setData(row, tag, obj.result.data[i][tag]);
            }
        }
        //set Title to name if not defined - for folders and playlists
        if (obj.result.data[i].Title === undefined) {
            obj.result.data[i].Title = obj.result.data[i].name;
        }

        //set Filetype
        if (obj.result.data[i].Filetype === undefined) {
            obj.result.data[i].Filetype = filetype(obj.result.data[i].uri, false);
        }
        //set Thumbnail
        switch(obj.result.data[i].Type) {
            case 'song':
            case 'stream':
            case 'webradio':
                obj.result.data[i].Thumbnail = getCssImageUri('/albumart?offset=0&uri=' + myEncodeURIComponent(obj.result.data[i].uri));
                break;
            case 'dir': 
                obj.result.data[i].Thumbnail = getCssImageUri('/folderart?path=' + myEncodeURIComponent(obj.result.data[i].uri));
                break;
            case 'plist':
            case 'smartpls':
                obj.result.data[i].Thumbnail = getCssImageUri('/playlistart?playlist=' + myEncodeURIComponent(obj.result.data[i].uri));
                break;
            case 'webradiodb':
                obj.result.data[i].Thumbnail = getCssImageUri(webradioDbPicsUri + obj.result.data[i].Image);
                break;
            // No Default
        }
        if (createRowCellsCallback !== undefined &&
            typeof(createRowCellsCallback) === 'function')
        {
            //custom row content
            createRowCellsCallback(row, obj.result.data[i]);
        }
        else {
            //default row content
            tableRow(row, obj.result.data[i], list, colspan, smallWidth);
        }
        if (i + z < tr.length) {
            replaceTblRow(mode, tr[i + z], row);
        }
        else {
            tbody.append(row);
        }
    }
    //remove obsolete lines
    tr = tbody.querySelectorAll('tr');
    for (let i = tr.length - 1; i >= nrItems + z; i --) {
        tr[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);

    if (nrItems === 0) {
        tbody.appendChild(emptyRow(colspan + 1));
    }
    unsetUpdateView(table);
    setScrollViewHeight(table);
    scrollToPosY(table.parentNode, app.current.scrollPos);
}

/**
 * Creates the columns in the row
 * @param {HTMLElement} row the row to populate
 * @param {object} data data to populate
 * @param {string} list table name
 * @param {number} colspan number of columns
 * @param {boolean} smallWidth true = print data in rows, false = print data in columns
 * @returns {void}
 */
function tableRow(row, data, list, colspan, smallWidth) {
    if (smallWidth === true) {
        const td = elCreateEmpty('td', {"colspan": colspan});
        for (let c = 0, d = settings['view' + list].fields.length; c < d; c++) {
            td.appendChild(
                elCreateNodes('div', {"class": ["row"]}, [
                    elCreateTextTn('small', {"class": ["col-3"]}, settings['view' + list].fields[c]),
                    elCreateNode('span', {"data-col": settings['view' + list].fields[c], "class": ["col-9"]},
                        printValue(settings['view' + list].fields[c], data[settings['view' + list].fields[c]])
                    )
                ])
            );
        }
        row.appendChild(td);
    }
    else {
        for (let c = 0, d = settings['view' + list].fields.length; c < d; c++) {
            row.appendChild(
                elCreateNode('td', {"data-col": settings['view' + list].fields[c]},
                    printValue(settings['view' + list].fields[c], data[settings['view' + list].fields[c]])
                )
            );
        }
    }
    switch(app.id) {
        case 'BrowsePlaylistDetail':
            // add quick play and remove action
            row.appendChild(
                pEl.actionPlaylistDetailTd.cloneNode(true)
            );
            break;
        case 'QueueCurrent':
            // add quick remove action
            row.appendChild(
                pEl.actionQueueTd.cloneNode(true)
            );
            break;
        case 'QueueJukeboxSong':
        case 'QueueJukeboxAlbum':
            // add quick play and remove action
            row.appendChild(
                pEl.actionJukeboxTd.cloneNode(true)
            );
            break;
        default:
            // add quick play action
            row.appendChild(
                pEl.actionTd.cloneNode(true)
            );
    }
}

/**
 * Creates an empty list hint
 * @param {number} colspan column count
 * @returns {HTMLElement} created row
 */
function emptyRow(colspan) {
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-secondary"]}, 'Empty list')
        )
    );
}

/**
 * Creates a loading list hint
 * @param {number} colspan column count
 * @returns {HTMLElement} created row
 */
function loadingRow(colspan) {
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-secondary"]}, 'Loading...')
        )
    );
}

/**
 * Creates a row with the error message
 * @param {object} obj jsonrpc error object
 * @param {number} colspan column count
 * @returns {HTMLElement} created row
 */
function errorRow(obj, colspan) {
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-danger"]}, obj.error.message, obj.error.data)
        )
    );
}

/**
 * Creates a row with the warning message
 * @param {string} message phrase to display
 * @param {number} colspan column count
 * @returns {HTMLElement} created row
 */
//eslint-disable-next-line no-unused-vars
function warningRow(message, colspan) {
    return elCreateNode('tr', {"class": ["not-clickable"]},
        elCreateNode('td', {"colspan": colspan},
            elCreateTextTn('div', {"class": ["alert", "alert-warning"]}, message)
        )
    );
}

/**
 * Wrapper for checkResult with id selector
 * @param {object} obj jsonrpc object to check
 * @param {string} id table id
 * @returns {boolean} true = result is not an error, else false
 */
function checkResultId(obj, id) {
    return checkResult(obj, document.querySelector('#' + id + ' > tbody'));
}

/**
 * Checks the json response for an error object or empty result
 * and displays the error in the table body.
 * @param {object} obj jsonrpc object to check
 * @param {HTMLElement} tbody body of the table
 * @returns {boolean} false = result is  empty or an error, else true
 */
function checkResult(obj, tbody) {
    //remove old alerts
    const alert = tbody.querySelector('.alert');
    if (alert) {
        alert.parentNode.parentNode.remove();
    }
    if (obj.error ||
        obj.result.returnedEntities === 0)
    {
        const thead = tbody.parentNode.querySelector('tr');
        const colspan = thead !== null
            ? thead.querySelectorAll('th').length
            : 0;
        elClear(tbody);
        const tfoot = tbody.parentNode.querySelector('tfoot');
        if (tfoot !== null) {
            elClear(tfoot);
        }
        if (obj.error) {
            tbody.appendChild(errorRow(obj, colspan));
        }
        else {
            tbody.appendChild(emptyRow(colspan));
        }
        unsetUpdateView(tbody.parentNode);
        setPagination(0, 0);
        return false;
    }
    return true;
}

/**
 * Checks if we should display data in rows or cols
 * @returns {boolean} true if window is small and the uiSmallWidthTagRows settings is true, else false
 */
function uiSmallWidthTagRows() {
    if (settings.webuiSettings.smallWidthTagRows === true) {
        return window.innerWidth < 576
            ? true
            : false;
    }
    return false;
}

/**
 * Handles the click on the actions column
 * @param {MouseEvent} event click event
 * @returns {void}
 */
function handleActionTdClick(event) {
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
        default:
            logError('Invalid action: ' + action);
    }
}

/**
 * Central table click handler.
 * Handles clicks on table header and body.
 * @param {MouseEvent} event the event to handle
 * @returns {HTMLElement} the event target (row) to handle or null if it was handled or should not be handled
 */
function tableClickHandler(event) {
    if (event.target.nodeName === 'CAPTION') {
        return null;
    }
    //select mode
    if (selectRow(event) === true) {
        return null;
    }
    //action td
    if (event.target.nodeName === 'A') {
        if (event.target.parentNode.getAttribute('data-col') === 'Action') {
            handleActionTdClick(event);
        }
        else {
            // allow default link action
        }
        return null;
    }
    //table header
    if (event.target.nodeName === 'TH') {
        if (features.featAdvqueue === false) {
            return null;
        }
        const colName = event.target.getAttribute('data-col');
        if (isColSortable(app.id, colName) === false) {
            //by this fields can not be sorted
            return null;
        }
        toggleSort(event.target, colName);
        appGoto(app.current.card, app.current.tab, app.current.view,
            app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
        return null;
    }
    //table body
    const target = event.target.closest('TR');
    if (target === null) {
        return null;
    }
    if (target.parentNode.nodeName === 'TBODY' &&
        checkTargetClick(target) === true)
    {
        return target;
    }
    return null;
}
