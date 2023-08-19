"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
 * Initializes a table header for drag and drop of columns
 * @param {string} tableName table name
 * @returns {void}
 */
function dragAndDropTableHeader(tableName) {
    const tableHeader = document.querySelector('#' + tableName + 'List > thead > tr');

    tableHeader.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TH') {
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            event.dataTransfer.setData('Text', event.target.getAttribute('data-col'));
            dragEl = event.target;
        }
    }, false);

    tableHeader.addEventListener('dragenter', function(event) {
        if (dragEl !== undefined &&
            dragEl.nodeName === event.target.nodeName)
        {
            event.target.classList.add('dragover-th');
        }
    }, false);

    tableHeader.addEventListener('dragleave', function(event) {
        if (dragEl !== undefined &&
            dragEl.nodeName === event.target.nodeName)
        {
            event.target.classList.remove('dragover-th');
        }
    }, false);

    tableHeader.addEventListener('dragover', function(event) {
        // prevent default to allow drop
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
    }, false);

    tableHeader.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined ||
            dragEl.nodeName !== 'TH')
        {
            return;
        }
        event.target.classList.remove('dragover-th');
        if (event.dataTransfer.getData('Text') === event.target.getAttribute('data-col')) {
            return;
        }
        // move element
        // @ts-ignore
        tableHeader.insertBefore(dragEl, event.target);
        // save this state
        setUpdateViewId(tableName + 'List');
        saveCols(tableName);
    }, false);

    tableHeader.addEventListener('dragend', function() {
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
        case 'BrowseRadioWebradiodb':
            return ["Country", "Description", "Genre", "Homepage", "Language", "Name", "StreamUri", "Codec", "Bitrate"];
        case 'BrowseRadioRadiobrowser':
            return ["clickcount", "country", "homepage", "language", "lastchangetime", "lastcheckok", "tags", "url_resolved", "votes"];
        case 'BrowseDatabaseAlbumList': {
            const tags = settings.tagListAlbum.slice();
            tags.push('Discs', 'SongCount', 'Duration', 'LastModified');
            return tags.filter(function(value) {
                return value !== 'Disc';
            });
        }
        case 'BrowseDatabaseAlbumDetailInfo': {
            const tags = settings.tagListAlbum.slice();
            tags.push('Discs', 'SongCount', 'Duration', 'LastModified');
            return tags.filter(function(value) {
                return value !== 'Disc' &&
                       value !== 'Album';
            });
        }
        case 'QueueJukeboxAlbum': {
            const tags = settings.tagListAlbum.slice();
            tags.push('Pos', 'Discs', 'SongCount', 'Duration', 'LastModified');
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
    tags.push('Duration', 'LastModified');

    switch(tableName) {
        case 'QueueCurrent':
            tags.push('AudioFormat', 'Priority');
            //fall through
        case 'BrowsePlaylistDetail':
        case 'QueueJukeboxSong':
        case 'QueueJukeboxAlbum':
            tags.push('Pos');
            break;
        case 'BrowseFilesystem':
            tags.push('Type', 'Filename');
            break;
        case 'Playback':
            tags.push('AudioFormat', 'Filetype');
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
        tags.push('dropdownTitleSticker');
        for (const sticker of stickerList) {
            tags.push(sticker);
        }
    }
    return tags;
}

/**
 * Creates the select columns checkbox list
 * @param {string} tableName table name
 * @param {HTMLElement} menu element to populate
 * @returns {void}
 */
function setColsChecklist(tableName, menu) {
    const tags = setColTags(tableName);
    for (let i = 0, j = tags.length; i < j; i++) {
        if (tableName === 'Playback' &&
            (tags[i] === 'Title' || tags[i].indexOf('MUSICBRAINZ_') === 0))
        {
            continue;
        }
        if (tags[i] === 'dropdownTitleSticker') {
            menu.appendChild(
                elCreateTextTn('h6', {"class": ["dropdown-header"]}, 'Sticker')
            );
        }
        else {
            const btn = elCreateText('button', {"class": ["btn", "btn-secondary", "btn-xs", "clickable", "mi", "mi-sm", "me-2"],
                "name": tags[i]}, 'radio_button_unchecked');
            if (settings['cols' + tableName].includes(tags[i])) {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            const div = elCreateNodes('div', {"class": ["form-check"]}, [
                btn,
                elCreateTextTn('label', {"class": ["form-check-label"], "for": tags[i]}, tags[i])
            ]);
            menu.appendChild(div);
        }
    }
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
    if (colName === 'Duration' ||
        colName === 'AudioFormat' ||
        colName.indexOf('sticker') === 0)
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

    for (let i = 0, j = settings['cols' + tableName].length; i < j; i++) {
        const hname = settings['cols' + tableName][i];
        const clickable = isColSortable(tableName, hname)
            ? 'clickable'
            : 'not-clickable';
        const th = elCreateTextTn('th', {"class": [clickable], "draggable": "true", "data-col": settings['cols' + tableName][i]}, hname);
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
    if (features.featTags === true) {
        th.appendChild(
            pEl.columnsBtn.cloneNode(true)
        );
    }
    th.appendChild(
        pEl.selectAllBtn.cloneNode(true)
    );
    thead.appendChild(th);
}

/**
 * Saves the selected columns for the table
 * @param {string} tableName table name
 * @param {HTMLElement} [tableEl] table element or undefined
 * @returns {void}
 */
function saveCols(tableName, tableEl) {
    const colsDropdown = elGetById(tableName + 'ColsDropdown');
    if (tableEl === undefined) {
        //select the table by name
        tableEl = elGetById(tableName + 'List');
    }
    const header = tableEl.querySelector('tr');
    if (colsDropdown !== null) {
        //apply the columns select list to the table header
        const colInputs = colsDropdown.firstChild.querySelectorAll('button');
        for (let i = 0, j = colInputs.length; i < j; i++) {
            if (colInputs[i].getAttribute('name') === null) {
                continue;
            }
            let th = header.querySelector('[data-col=' + colInputs[i].name + ']');
            if (colInputs[i].classList.contains('active') === false) {
                if (th) {
                    th.remove();
                }
            }
            else if (!th) {
                th = elCreateTextTn('th', {"data-col": colInputs[i].name}, colInputs[i].name);
                header.insertBefore(th, header.lastChild);
            }
        }
    }
    //construct columns to save from actual table header
    const params = {"table": "cols" + tableName, "cols": []};
    const ths = header.querySelectorAll('th');
    for (let i = 0, j = ths.length; i < j; i++) {
        const name = ths[i].getAttribute('data-col');
        if (name !== 'Action' &&
            name !== null)
        {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings, true);
}

/**
 * Saves the fields for the playback card
 * @param {string} tableName table name
 * @param {string} dropdownId id fo the column select dropdown
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveColsDropdown(tableName, dropdownId) {
    const params = {"table": tableName, "cols": []};
    const colInputs = document.querySelectorAll('#' + dropdownId + ' button.active');
    for (let i = 0, j = colInputs.length; i < j; i++) {
        const name = colInputs[i].getAttribute('name');
        if (name) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings, true);
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
 * Conditionally replaces a table row, if metadata or cols are changed.
 * @param {HTMLElement} row row to replace
 * @param {HTMLElement} el replacement row
 * @returns {void}
 */
function replaceTblRow(row, el) {
    if (getData(row, 'uri') === getData(el, 'uri') &&
        getData(row, 'cols') === getData(el, 'cols') &&
        getData(row, 'name') === getData(el, 'name') &&
        getData(row, 'songid') === getData(el, 'songid') &&
        getData(row, 'AlbumId') === getData(el, 'AlbumId'))
    {
        return;
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
    const tbody = table.querySelector('tbody');
    const colspan = settings['cols' + list] !== undefined ? settings['cols' + list].length : 0;

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
    let lastDisc = obj.result.data.length > 0 && obj.result.data[0].Disc !== undefined ? Number(obj.result.data[0].Disc) : 0;
    if (obj.result.Discs !== undefined && obj.result.Discs > 1) {
        const row = addDiscRow(1, obj.result.AlbumId, colspan);
        if (z < tr.length) {
            replaceTblRow(tr[z], row);
        }
        else {
            tbody.append(row);
        }
        z++;
    }
    for (let i = 0; i < nrItems; i++) {
        //disc handling for album view
        if (obj.result.data[0].Disc !== undefined && lastDisc < Number(obj.result.data[i].Disc)) {
            const row = addDiscRow(obj.result.data[i].Disc, obj.result.AlbumId, colspan);
            if (i + z < tr.length) {
                replaceTblRow(tr[i + z], row);
            }
            else {
                tbody.append(row);
            }
            z++;
            lastDisc = obj.result.data[i].Disc;
        }
        const row = elCreateEmpty('tr', {});
        if (perRowCallback !== undefined && typeof(perRowCallback) === 'function') {
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

        if (createRowCellsCallback !== undefined && typeof(createRowCellsCallback) === 'function') {
            //custom row content
            createRowCellsCallback(row, obj.result.data[i]);
        }
        else {
            //default row content
            tableRow(row, obj.result.data[i], list, colspan, smallWidth);
        }
        if (i + z < tr.length) {
            replaceTblRow(tr[i + z], row);
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
    if (data.Type === 'parentDir') {
        row.appendChild(elCreateText('td', {"colspan": (colspan + 1), "data-title-phrase": "Open parent folder"}, '..'));
    }
    else {
        if (smallWidth === true) {
            const td = elCreateEmpty('td', {"colspan": colspan});
            for (let c = 0, d = settings['cols' + list].length; c < d; c++) {
                td.appendChild(
                    elCreateNodes('div', {"class": ["row"]}, [
                        elCreateTextTn('small', {"class": ["col-3"]}, settings['cols' + list][c]),
                        elCreateNode('span', {"data-col": settings['cols' + list][c], "class": ["col-9"]},
                            printValue(settings['cols' + list][c], data[settings['cols' + list][c]])
                        )
                    ])
                );
            }
            row.appendChild(td);
        }
        else {
            for (let c = 0, d = settings['cols' + list].length; c < d; c++) {
                row.appendChild(
                    elCreateNode('td', {"data-col": settings['cols' + list][c]},
                        printValue(settings['cols' + list][c], data[settings['cols' + list][c]])
                    )
                );
            }
        }
        setData(row, 'cols', settings['cols' + list].join(':'));
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
 * Checks the json response for an error object and displays the error in the table body
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
        return window.innerWidth < 576 ? true : false;
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
        handleActionTdClick(event);
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
