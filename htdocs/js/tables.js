"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module tables_js */

/**
 * Initializes a table body for drag and drop of rows
 * @param {string} tableId table id
 */
function dragAndDropTable(tableId) {
    const tableBody = document.querySelector('#' + tableId + ' > tbody');
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
            hidePopover();
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            event.dataTransfer.setData('Text', event.target.getAttribute('id'));
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    tableBody.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TR') {
            return;
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        if (target.nodeName === 'TR') {
            target.classList.remove('dragover');
        }
    }, false);
    tableBody.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TR') {
            return;
        }
        const tr = tableBody.querySelectorAll('.dragover');
        for (let i = 0, j = tr.length; i < j; i++) {
            tr[i].classList.remove('dragover');
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        if (target.nodeName === 'TR') {
            target.classList.add('dragover');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    tableBody.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TR') {
            return;
        }
        const tr = tableBody.querySelectorAll('.dragover');
        for (let i = 0, j = tr.length; i < j; i++) {
            tr[i].classList.remove('dragover');
        }
        if (document.getElementById(event.dataTransfer.getData('Text'))) {
            document.getElementById(event.dataTransfer.getData('Text')).classList.remove('opacity05');
        }
        dragEl = undefined;
    }, false);
    tableBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TR') {
            return;
        }
        let target = event.target;
        if (event.target.nodeName === 'TD') {
            target = event.target.parentNode;
        }
        const oldSongpos = getDataId(event.dataTransfer.getData('Text'), 'songpos');
        const newSongpos = getData(target, 'songpos');
        document.getElementById(event.dataTransfer.getData('Text')).remove();
        dragEl.classList.remove('opacity05');
        // @ts-ignore
        tableBody.insertBefore(dragEl, target);
        const tr = tableBody.querySelectorAll('.dragover');
        for (let i = 0, j = tr.length; i < j; i++) {
            tr[i].classList.remove('dragover');
        }
        document.getElementById(tableId).classList.add('opacity05');
        if (app.id === 'QueueCurrent') {
            sendAPI("MYMPD_API_QUEUE_MOVE_SONG", {
                "from": oldSongpos,
                "to": newSongpos
            }, null, false);
        }
        else if (app.id === 'BrowsePlaylistsDetail') {
            playlistMoveSong(oldSongpos, newSongpos);
        }
    }, false);
}

/**
 * Initializes a table header for drag and drop of columns
 * @param {string} tableName table name
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
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    tableHeader.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TH') {
            return;
        }
        if (event.target.nodeName === 'TH') {
            event.target.classList.remove('dragover-th');
        }
    }, false);
    tableHeader.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TH') {
            return;
        }
        const th = tableHeader.querySelectorAll('.dragover-th');
        for (let i = 0, j = th.length; i < j; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (event.target.nodeName === 'TH') {
            event.target.classList.add('dragover-th');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    tableHeader.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TH') {
            return;
        }
        const th = tableHeader.querySelectorAll('.dragover-th');
        for (let i = 0, j = th.length; i < j; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']')) {
            this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']').classList.remove('opacity05');
        }
        dragEl = undefined;
    }, false);
    tableHeader.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined || dragEl.nodeName !== 'TH') {
            return;
        }
        this.querySelector('[data-col=' + event.dataTransfer.getData('Text') + ']').remove();
        dragEl.classList.remove('opacity05');
        // @ts-ignore
        tableHeader.insertBefore(dragEl, event.target);
        const th = tableHeader.querySelectorAll('.dragover-th');
        for (let i = 0, j = th.length; i < j; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (document.getElementById(tableName + 'List')) {
            document.getElementById(tableName + 'List').classList.add('opacity05');
            saveCols(tableName);
        }
        else {
            saveCols(tableName, this.parentNode.parentNode);
        }
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
        case 'BrowseDatabaseAlbumList':
            return ["Album", "AlbumArtist", "Genre", "Date", "Discs", "SongCount", "Duration", "LastModified"];
    }

    const tags = settings.tagList.slice();
    if (features.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    tags.push('LastModified');

    switch(tableName) {
        case 'QueueCurrent':
            tags.push('AudioFormat');
            tags.push('Priority');
            //fall through
        case 'BrowsePlaylistsDetail':
        case 'QueueJukebox':
            tags.push('Pos');
            break;
        case 'BrowseFilesystem':
            tags.push('Type');
            tags.push('Filename');
            break;
        case 'Playback':
            tags.push('AudioFormat');
            tags.push('Filetype');
            if (features.featLyrics === true) {
                tags.push('Lyrics');
            }
            break;
        case 'QueueLastPlayed':
            tags.push('Pos');
            tags.push('LastPlayed');
            break;
    }
    //sort tags and append stickers
    tags.sort();
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
 */
function setColsChecklist(tableName, menu) {
    const tags = setColTags(tableName);
    for (let i = 0, j = tags.length; i < j; i++) {
        if (tableName === 'Playback' && tags[i] === 'Title') {
            continue;
        }
        if (tags[i] === 'dropdownTitleSticker') {
            menu.appendChild(
                elCreateTextTn('h6', {"class": ["dropdown-header"]}, 'Sticker')
            );
        }
        else {
            const btn = elCreateText('button', {"class": ["btn", "btn-secondary", "btn-xs", "clickable", "mi", "mi-small", "me-2"],
                "name": tags[i]}, 'radio_button_unchecked');
            if (settings['cols' + tableName].includes(tags[i])) {
                btn.classList.add('active');
                btn.textContent = 'check'
            }
            const div = elCreateNodes('div', {"class": ["form-check"]}, [
                btn,
                elCreateTextTn('lable', {"class": ["form-check-label"], "for": tags[i]}, tags[i])
            ]);
            menu.appendChild(div);
        }
    }
}

/**
 * Sets the table header columns
 * @param {string} tableName table name
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
        const th = elCreateTextTn('th', {"draggable": "true", "data-col": settings['cols' + tableName][i]}, hname);
        if (hname === 'Track' ||
            hname === 'Pos')
        {
            th.textContent = '#';
        }
        if ((tableName === 'Search' && hname === app.cards.Search.sort.tag) ||
            (tableName === 'BrowseRadioWebradiodb' && hname === app.cards.Browse.tabs.Radio.views.Webradiodb.sort.tag)
           )
        {
            th.appendChild(
                elCreateText('span', {"class": ["sort-dir", "mi", "float-end"]}, (app.cards.Search.sort.desc === true ? 'arrow_drop_up' : 'arrow_drop_down'))
            );
        }
        thead.appendChild(th);
    }
    //append action column
    const th = elCreateEmpty('th', {"data-col": "Action"});
    if (features.featTags === true) {
        th.appendChild(
            elCreateText('a', {"href": "#", "data-action": "popover", "data-popover": "columns",
                "class": ["align-middle", "mi", "mi-small", "clickable"], "data-title-phrase": "Columns"}, 'settings')
        );
    }
    thead.appendChild(th);
}

/**
 * Saves the selected columns for the table
 * @param {string} tableName table name
 * @param {HTMLElement} [tableEl] table element or undefined
 */
function saveCols(tableName, tableEl) {
    const colsDropdown = document.getElementById(tableName + 'ColsDropdown');
    if (tableEl === undefined) {
        //select the table by name
        tableEl = document.getElementById(tableName + 'List');
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
        if (name !== 'Action' && name !== null) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings, true);
}

/**
 * Saves the fields for the playback card
 * @param {string} tableName table name
 * @param {string} dropdownId id fo the column select dropdown
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
        app.current.sort.desc = app.current.sort.desc === false ? true : false;
    }
    else {
        app.current.sort.desc = false;
        app.current.sort.tag = colName;
    }
    //remove old sort indicator
    const sdi = th.parentNode.querySelectorAll('.sort-dir');
    for (const s of sdi) {
        s.remove();
    }
    //set new sort indicator
    // @ts-ignore
    th.appendChild(
        elCreateText('span', {"class": ["sort-dir", "mi", "float-end"]}, (app.current.sort.desc === true ? 'arrow_drop_up' : 'arrow_drop_down'))
    );
}

/**
 * Replaces a table row
 * @param {HTMLElement} row row to replace
 * @param {HTMLElement} el replacement row
 */
function replaceTblRow(row, el) {
    const menuEl = row.querySelector('[data-popover]');
    if (menuEl) {
        hidePopover();
    }
    row.replaceWith(el);
}

/**
 * Adds a row with discnumber to the table
 * @param {number} disc discnumber
 * @param {string} album album
 * @param {object} albumartist album artists 
 * @param {number} colspan column count
 * @returns {HTMLElement} the created row
 */
function addDiscRow(disc, album, albumartist, colspan) {
    const row = elCreateNodes('tr', {"class": ["not-clickable"]}, [
        elCreateNode('td', {},
            elCreateText('span', {"class": ["mi"]}, 'album')
        ),
        elCreateTextTnNr('td', {"colspan": (colspan - 1)}, 'Discnum', disc),
        elCreateNode('td', {"data-col": "Action"},
            elCreateText('a', {"data-action": "popover", "data-popover": "disc", "href": "#", "class": ["mi", "color-darkgrey"],
                "data-title-phrase":"Actions"}, ligatureMore)
        )
    ]);
    setData(row, 'Disc', disc);
    setData(row, 'Album', album);
    setData(row, 'AlbumArtist', albumartist);
    return row;
}

/**
 * Updates the table from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list table name to populate
 * @param {Function} [perRowCallback] callback per row
 * @param {Function} [createRowCellsCallback] callback to create the row
 */
function updateTable(obj, list, perRowCallback, createRowCellsCallback) {
    const table = document.getElementById(list + 'List');
    const tbody = table.querySelector('tbody');
    const colspan = settings['cols' + list] !== undefined ? settings['cols' + list].length : 0;

    const nrItems = obj.result.returnedEntities;
    const tr = tbody.querySelectorAll('tr');
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
        const row = addDiscRow(1, obj.result.data[0].Album, obj.result.data[0][tagAlbumArtist], colspan);
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
            const row = addDiscRow(obj.result.data[i].Disc, obj.result.data[i].Album, obj.result.data[i][tagAlbumArtist], colspan);
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
        //set artist and album data
        if (obj.result.data[i].Album !== undefined) {
            setData(row, 'Album', obj.result.data[i].Album);
        }
        if (obj.result.data[i][tagAlbumArtist] !== undefined) {
            setData(row, 'AlbumArtist', obj.result.data[i][tagAlbumArtist]);
        }
        //and other browse tags
        for (const tag of settings.tagListBrowse) {
            if (albumFilters.includes(tag) &&
                obj.result.data[i][tag] !== undefined &&
                checkTagValue(obj.result.data[i][tag], '-') === false)
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

    const trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems + z; i --) {
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
        switch(app.id) {
            case 'QueueCurrent':
            case 'BrowsePlaylistsDetail':
                row.appendChild(
                    pEl.actionQueueTd.cloneNode(true)
                );
                break;
            default:
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
 * 
 * @param {object} obj jsonrpc object to check
 * @param {HTMLElement} tbody body of the table
 * @returns {boolean} true = result is not an error, else false
 */
function checkResult(obj, tbody) {
    const thead = tbody.parentNode.querySelector('tr');
    const colspan = thead !== undefined ? thead.querySelectorAll('th').length : 0;
    const tfoot = tbody.parentNode.querySelector('tfoot');
    if (obj.error) {
        elClear(tbody);
        if (tfoot !== null) {
            elClear(tfoot);
        }
        tbody.appendChild(errorRow(obj, colspan));
        unsetUpdateView(tbody.parentNode);
        setPagination(0, 0);
        return false;
    }
    if (obj.result.returnedEntities === 0) {
        elClear(tbody);
        if (tfoot !== null) {
            elClear(tfoot);
        }
        tbody.appendChild(emptyRow(colspan));
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
    if (settings.webuiSettings.uiSmallWidthTagRows === true) {
        return window.innerWidth < 576 ? true : false;
    }
    return false;
}

/**
 * Handles the click on the actions column
 * @param {MouseEvent} event click event
 */
function handleActionTdClick(event) {
    event.preventDefault();
    switch(event.target.getAttribute('data-action')) {
        case 'popover':
            showPopover(event);
            break;
        case 'quickPlay':
            clickQuickPlay(event.target);
            break;
        case 'quickRemove':
            clickQuickRemove(event.target);
            break;
    }
}
