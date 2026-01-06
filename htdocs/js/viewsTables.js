"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewsTables_js */

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
        const newPos = getData(target, 'pos');
        const oldPos = getData(dragEl, 'pos');
        if (oldPos === newPos) {
            return;
        }
        // set dragged element uri to undefined to force table row replacement
        setData(dragEl, 'uri', undefined);
        elHide(dragEl);
        // apply new order
        setUpdateViewId(tableId);
        switch(app.id) {
            case 'QueueCurrent': {
                queueMoveSong(oldPos, newPos);
                break;
            }
            case 'BrowsePlaylistDetail': {
                currentPlaylistMoveSong(oldPos, newPos);
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
 * Return the displayname of a header
 * @param {string} header Header fieldname
 * @returns {string} Header displayname
 */
function getHeaderName(header) {
    switch (header) {
        case 'Track':
        case 'Pos':
            return '#';
        case 'Thumbnail':
            return '';
        default:
            return header;
    }
}

/**
 * Sets the table header columns
 * @param {string} tableName table name
 * @returns {void}
 */
function setCols(tableName) {
    const thead = document.querySelector('#' + tableName + 'List > thead > tr');
    elClear(thead);

    for (const field of settings['view' + tableName].fields) {
        thead.appendChild(
            elCreateTextTn('th', {"data-col": field}, getHeaderName(field))
        );
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
 * @param {boolean} inSelectMode the selection mode
 * @param {HTMLElement} row row to replace
 * @param {HTMLElement} el replacement row
 * @returns {void}
 */
function replaceTblRow(inSelectMode, row, el) {
    if (inSelectMode === true) {
        const prevRow = row.previousElementSibling;
        const prevUri = prevRow !== null
            ? getData(prevRow, 'uri')
            : null;

        const nextRow = row.nextElementSibling;
        const nextUri = nextRow !== null
            ? getData(nextRow, 'uri')
            : null;

        const newUri = getData(el, 'uri');
        if (getData(row, 'uri') === newUri) {
            copyRowSelection(row, el);
        }
        else if (nextUri === newUri) {
            copyRowSelection(nextRow, el);
        }
        else if (prevUri === newUri) {
            copyRowSelection(prevRow, el);
        }
    }
    row.replaceWith(el);
}

/**
 * Copy the selection state
 * @param {HTMLElement} row row to replace
 * @param {HTMLElement} el replacement row
 * @returns {void}
 */
function copyRowSelection(row, el) {
    if (row.lastElementChild.lastElementChild.textContent === ligatures.checked) {
        el.lastElementChild.lastElementChild.textContent = ligatures.checked;
        el.classList.add('selected');
    }
}

/**
 * Adds a row with discnumber to the table
 * @param {number} disc discnumber
 * @param {string} albumId the albumid
 * @param {string} albumName the album name
 * @param {number} colspan column count
 * @returns {HTMLElement} the created row
 */
function addDiscRow(disc, albumId, albumName, colspan) {
    const actionTd = elCreateEmpty('td', {"data-col": "Action"});
    addActionLinks(actionTd, 'disc');
    const row = elCreateNodes('tr', {"class": ["not-clickable"]}, [
        elCreateNode('td', {},
            elCreateText('span', {"class": ["mi"]}, 'album')
        ),
        elCreateTextTnNr('td', {"colspan": (colspan - 1)}, 'Discnum', disc),
        actionTd
    ]);
    setData(row, 'Disc', disc);
    setData(row, 'AlbumId', albumId);
    setData(row, 'type', 'disc');
    setData(row, 'name', albumName + ' (' + tn('Disc') + ' ' + disc + ')');
    return row;
}

/**
 * Adds a row with the work to the table.
 * @param {string} work The work name
 * @param {string} albumId the albumid
 * @param {string} albumName the album name
 * @param {number} colspan column count
 * @returns {HTMLElement} the created row
 */
function addWorkRow(work, albumId, albumName, colspan) {
    const actionTd = elCreateEmpty('td', {"data-col": "Action"});
    addActionLinks(actionTd, 'work');
    const row = elCreateNodes('tr', {"class": ["not-clickable"]}, [
        elCreateNode('td', {},
            elCreateText('span', {"class": ["mi"]}, 'music_note')
        ),
        elCreateText('td', {"colspan": (colspan - 1)}, work),
        actionTd
    ]);
    setData(row, 'Work', work);
    setData(row, 'AlbumId', albumId);
    setData(row, 'type', 'work');
    setData(row, 'name', albumName + ' (' + work + ')');
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

    let tr = tbody.querySelectorAll('tr');
    const smallWidth = uiSmallWidthTagRows();

    if (smallWidth === true) {
        table.classList.add('smallWidth');
    }
    else {
        table.classList.remove('smallWidth');
    }

    const actionTd = elCreateEmpty('td', {"data-col": "Action"});
    addActionLinks(actionTd);

    //disc handling for album view
    let z = 0;
    let lastDisc = 0;
    let lastWork = '';

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        if (list === 'BrowseDatabaseAlbumDetail') {
            // Disc divider
            if (obj.result.DiscCount > 1 &&
                obj.result.data[0].Disc !== undefined &&
                lastDisc < Number(obj.result.data[i].Disc))
            {
                const row = addDiscRow(obj.result.data[i].Disc, obj.result.AlbumId, obj.result.Album, colspan);
                if (i + z < tr.length) {
                    replaceTblRow(mode, tr[i + z], row);
                }
                else {
                    tbody.append(row);
                }
                z++;
                lastDisc = obj.result.data[i].Disc;
            }
            // Work divider
            if (settings.webuiSettings.showWorkTagAlbumDetail === true &&
                obj.result.data[0].Work !== undefined &&
                lastWork !== obj.result.data[i].Work)
            {
                const row = addWorkRow(obj.result.data[i].Work, obj.result.AlbumId, obj.result.Album, colspan);
                if (i + z < tr.length) {
                    replaceTblRow(mode, tr[i + z], row);
                } else {
                    tbody.append(row);
                }
                z++;
                lastWork = obj.result.data[i].Work;
            }
        }

        const row = elCreateEmpty('tr', {});
        if (perRowCallback !== undefined &&
            typeof(perRowCallback) === 'function')
        {
            perRowCallback(row, obj.result.data[i], obj.result);
        }
        //data row
        setEntryData(row, obj.result.data[i]);
        if (createRowCellsCallback !== undefined &&
            typeof(createRowCellsCallback) === 'function')
        {
            //custom row content
            createRowCellsCallback(row, obj.result.data[i], obj.result);
        }
        else {
            //default row content
            tableRow(row, obj.result.data[i], list, colspan, smallWidth, actionTd);
        }
        if (features.featPagination === true ||
            obj.result.offset === 0)
        {
            if (i + z < tr.length) {
                replaceTblRow(mode, tr[i + z], row);
            }
            else {
                tbody.append(row);
            }
        }
        else {
            tbody.append(row);
        }
    }
    //remove obsolete rows
    if (features.featPagination === true ||
        obj.result.offset === 0)
    {
        tr = tbody.querySelectorAll('tr');
        for (let i = tr.length - 1; i >= obj.result.returnedEntities + z; i --) {
            tr[i].remove();
        }
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    unsetUpdateView(table);
    setScrollViewHeight(table);
    restoreScrollPos(obj.result.offset, table.parentNode, app.current.scrollPos);
}

/**
 * Creates the columns in the row
 * @param {HTMLElement} row the row to populate
 * @param {object} data data to populate
 * @param {string} list table name
 * @param {number} colspan number of columns
 * @param {boolean} smallWidth true = print data in rows, false = print data in columns
 * @param {Element} actionTd action table cell element
 * @returns {void}
 */
function tableRow(row, data, list, colspan, smallWidth, actionTd) {
    const fieldCount = settings['view' + list].fields.length;
    if (smallWidth === true) {
        const td = elCreateEmpty('td', {"colspan": colspan});
        if (fieldCount === 1) {
            td.appendChild(
                elCreateNodes('div', {"class": ["row"]}, [
                    elCreateNode('span', {"data-col": settings['view' + list].fields[0], "class": ["col-12"]},
                        printValue(settings['view' + list].fields[0], data[settings['view' + list].fields[0]], data)
                    )
                ])
            );
        }
        else {
            for (let c = 0; c < fieldCount; c++) {
                td.appendChild(
                    elCreateNodes('div', {"class": ["row"]}, [
                        elCreateTextTn('small', {"class": ["col-3"]}, settings['view' + list].fields[c]),
                        elCreateNode('span', {"data-col": settings['view' + list].fields[c], "class": ["col-9"]},
                            printValue(settings['view' + list].fields[c], data[settings['view' + list].fields[c]], data)
                        )
                    ])
                );
            }
        }
        row.appendChild(td);
    }
    else {
        for (let c = 0; c < fieldCount; c++) {
            row.appendChild(
                elCreateNode('td', {"data-col": settings['view' + list].fields[c]},
                    printValue(settings['view' + list].fields[c], data[settings['view' + list].fields[c]], data)
                )
            );
        }
    }
    row.appendChild(actionTd.cloneNode(true));
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
 * Updates the table footer
 * @param {Element} tfoot Element to insert the footer row
 * @param {Element} content Dom node to insert
 * @returns {void}
 */
function addTblFooter(tfoot, content) {
    const colspan = settings['view' + app.id].fields.length + 1;
    tfoot.appendChild(
        elCreateNode('tr', {"class": ["not-clickable"]},
            elCreateNode('td', {"colspan": colspan}, content)
        )
    );
}
