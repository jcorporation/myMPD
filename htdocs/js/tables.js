"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module tables_js */

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
            app.cards.Search.sort.tag = '';
        }
    }
    const thead = document.querySelector('#' + tableName + 'List > thead > tr');
    elClear(thead);

    for (let i = 0, j = settings['view' + tableName].fields.length; i < j; i++) {
        const hname = settings['view' + tableName].fields[i];
        const th = elCreateTextTn('th', {"data-col": settings['view' + tableName].fields[i]}, hname);
        thead.appendChild(th);
    }
    //append action column
    const th = elCreateEmpty('th', {"data-col": "Action"});
    th.appendChild(
        pEl.selectAllBtn.cloneNode(true)
    );
    thead.appendChild(th);
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
    const colspan = settings['view' + list].fields !== undefined
        ? settings['view' + list].fields.length
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
        tbody.appendChild(emptyMsgEl(colspan + 1, 'table'));
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
