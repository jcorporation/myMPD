"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function dragAndDropTable(table) {
    const tableBody = document.getElementById(table).getElementsByTagName('tbody')[0];
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
            hidePopover();
            event.target.classList.add('opacity05');
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
        const tr = tableBody.getElementsByClassName('dragover');
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
        const tr = tableBody.getElementsByClassName('dragover');
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
        const oldSongpos = getCustomDomPropertyId(event.dataTransfer.getData('Text'), 'data-songpos');
        const newSongpos = getCustomDomProperty(target, 'data-songpos');
        document.getElementById(event.dataTransfer.getData('Text')).remove();
        dragEl.classList.remove('opacity05');
        tableBody.insertBefore(dragEl, target);
        const tr = tableBody.getElementsByClassName('dragover');
        for (let i = 0, j = tr.length; i < j; i++) {
            tr[i].classList.remove('dragover');
        }
        document.getElementById(table).classList.add('opacity05');
        if (app.current.app === 'Queue' && app.current.tab === 'Current') {
            sendAPI("MYMPD_API_QUEUE_MOVE_SONG", {"from": oldSongpos, "to": newSongpos});
        }
        else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
            playlistMoveTrack(oldSongpos, newSongpos);
        }
    }, false);
}

function dragAndDropTableHeader(table) {
    let tableHeader;
    if (document.getElementById(table + 'List')) {
        tableHeader = document.getElementById(table + 'List').getElementsByTagName('tr')[0];
    }
    else {
        tableHeader = table.getElementsByTagName('tr')[0];
        table = 'BrowseDatabase';
    }

    tableHeader.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TH') {
            event.target.classList.add('opacity05');
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
        const th = tableHeader.getElementsByClassName('dragover-th');
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
        const th = tableHeader.getElementsByClassName('dragover-th');
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
        tableHeader.insertBefore(dragEl, event.target);
        const th = tableHeader.getElementsByClassName('dragover-th');
        for (let i = 0, j = th.length; i < j; i++) {
            th[i].classList.remove('dragover-th');
        }
        if (document.getElementById(table + 'List')) {
            document.getElementById(table + 'List').classList.add('opacity05');
            saveCols(table);
        }
        else {
            saveCols(table, this.parentNode.parentNode);
        }
    }, false);
}

function setColTags(table) {
    const tags = settings.tagList.slice();
    if (features.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    tags.push('LastModified');

    switch(table) {
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
    tags.push('dropdownTitleSticker');
    if (features.featStickers === true) {
        for (const sticker of stickerList) {
            tags.push(sticker);
        }
    }
    return tags;
}

function setColsChecklist(table, menu) {
    const tags = setColTags(table);
    for (let i = 0, j = tags.length; i < j; i++) {
        if (table === 'Playback' && tags[i] === 'Title') {
            continue;
        }
        if (tags[i] === 'dropdownTitleSticker') {
            menu.appendChild(elCreate('h6', {"class": ["dropdown-header"]}, tn('Sticker')));
        }
        else {
            const div = elCreate('div', {"class": ["form-check"]}, '');
            const btn = elCreate('button', {"class": ["btn", "btn-secondary", "btn-xs", "clickable", "mi", "mi-small", "me-2"], "name": tags[i]}, 'radio_button_unchecked');
            if (settings['cols' + table].includes(tags[i])) {
                btn.classList.add('active');
                btn.textContent = 'check'
            }
            div.appendChild(btn);
            div.appendChild(elCreate('lable', {"class": ["form-check-label"], "for": tags[i]}, tn(tags[i])));
            menu.appendChild(div);
        }
    }
}

function setCols(table) {
    let sort = app.current.sort;
    
    if (table === 'Search' && app.apps.Search.sort === 'Title') {
        if (settings.tagList.includes('Title')) {
            sort = 'Title';
        }
        else if (features.featTags === false) {
            sort = 'Filename';
        }
        else {
            sort = '-';
        }
    }
    
    const thead = document.getElementById(table + 'List').getElementsByTagName('tr')[0];
    elClear(thead);

    for (let i = 0, j = settings['cols' + table].length; i < j; i++) {
        const hname = settings['cols' + table][i];
        const th = elCreate('th', {"draggable": "true", "data-col": settings['cols' + table][i]}, tn(hname));
        if (hname === 'Track' || hname === 'Pos') {
            th.textContent = '#';
        }

        if (table === 'Search' && (hname === sort || ('-' + hname) === sort) ) {
            let sortdesc = false;
            if (app.current.sort.indexOf('-') === 0) {
                sortdesc = true;
            }
            th.appendChild(elCreate('span', {"class": ["sort-dir", "mi", "float-end"]}, (sortdesc === true ? 'arrow_drop_up' : 'arrow_drop_down')));
        }
        thead.appendChild(th);
    }
    //append action column
    const th = elCreate('th', {"data-col": "Action"}, '');
    if (features.featTags === true) {
        th.appendChild(elCreate('a', {"class": ["align-middle", "mi", "mi-small", "clickable"], "data-title-phrase": tn('Columns')}, 'settings'));
    }
    thead.appendChild(th);
}

function saveCols(table, tableEl) {
    const colsDropdown = document.getElementById(table + 'ColsDropdown');
    let header;
    if (tableEl === undefined) {
        header = document.getElementById(table + 'List').getElementsByTagName('tr')[0];
    }
    else if (typeof(tableEl) === 'string') {
        header = document.querySelector(tableEl).getElementsByTagName('tr')[0];
    }
    else {
        header = tableEl.getElementsByTagName('tr')[0];
    }
    if (colsDropdown) {
        const colInputs = colsDropdown.firstChild.getElementsByTagName('button');
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
                th = document.createElement('th');
                th.textContent = colInputs[i].name;
                th.setAttribute('data-col', colInputs[i].name);
                header.insertBefore(th, header.lastChild);
            }
        }
    }
    
    const params = {"table": "cols" + table, "cols": []};
    const ths = header.getElementsByTagName('th');
    for (let i = 0, j = ths.length; i < j; i++) {
        const name = ths[i].getAttribute('data-col');
        if (name !== 'Action' && name !== null) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveColsPlayback(table) {
    const colInputs = document.getElementById(table + 'ColsDropdown').firstChild.getElementsByTagName('button');
    const header = document.getElementById('cardPlaybackTags');

    for (let i = 0, j = colInputs.length - 1; i < j; i++) {
        let th = document.getElementById('current' + colInputs[i].name);
        if (colInputs[i].classList.contains('active') === false) {
            if (th) {
                th.remove();
            }
        } 
        else if (!th) {
            th = document.createElement('div');
            th.innerHTML = '<small>' + t(colInputs[i].name) + '</small><p></p>';
            th.setAttribute('id', 'current' + colInputs[i].name);
            setCustomDomProperty(th, 'data-tag', colInputs[i].name);
            header.appendChild(th);
        }
    }
    
    const params = {"table": "cols" + table, "cols": []};
    const ths = header.getElementsByTagName('div');
    for (let i = 0, j = ths.length; i < j; i++) {
        const name = getCustomDomProperty(ths[i], 'data-tag');
        if (name) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

function replaceTblRow(row, el) {
    const menuEl = row.querySelector('[data-popover]');
    if (menuEl) {
        hidePopover();
    }
    row.replaceWith(el);
}

function updateTable(obj, list, perRowCallback, createRowCellsCallback) {
    const table = document.getElementById(list + 'List');
    setScrollViewHeight(table);
    const tbody = table.getElementsByTagName('tbody')[0];
    const colspan = settings['cols' + list] !== undefined ? settings['cols' + list].length : 0;

    const nrItems = obj.result.returnedEntities;
    const tr = tbody.getElementsByTagName('tr');
    //disc handling for album view
    let z = 0;
    let lastDisc = obj.result.data.length > 0 && obj.result.data[0].Disc !== undefined ? Number(obj.result.data[0].Disc) : 0;
    if (obj.result.Discs !== undefined && obj.result.Discs > 1) {
        const row = document.createElement('tr');
        row.classList.add('not-clickable');
        setCustomDomProperty(row, 'data-disc', '1');
        setCustomDomProperty(row, 'data-album', obj.result.data[0].Album);
        setCustomDomProperty(row, 'data-albumartist', obj.result.data[0][tagAlbumArtist]);
        row.innerHTML = '<td><span class="mi">album</span></td><td colspan="' + (colspan - 1) +'">' + t('Disc 1') + '</td>' +
            '<td data-col="Action"><a data-popover="disc" href="#" class="mi color-darkgrey" title="' + t('Actions') + '">' + ligatureMore + '</a></td>';
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
            const row = document.createElement('tr');
            row.classList.add('not-clickable');
            setCustomDomProperty(row, 'data-disc', obj.result.data[i].Disc);
            setCustomDomProperty(row, 'data-album', obj.result.data[i].Album);
            setCustomDomProperty(row, 'data-albumartist', obj.result.data[i][tagAlbumArtist]);
            row.innerHTML = '<td><span class="mi">album</span></td><td colspan="' + (colspan - 1) +'">' + 
                t('Disc') + ' ' + e(obj.result.data[i].Disc) + '</td>' +
                '<td data-col="Action"><a data-popover="disc" href="#" class="mi color-darkgrey" title="' + t('Actions') + '">' + ligatureMore + '</a></td>';
            if (i + z < tr.length) {
                replaceTblRow(tr[i + z], row);
            }
            else {
                tbody.append(row);
            }
            z++;
            lastDisc = obj.result.data[i].Disc;
        }
        const row = document.createElement('tr');
        if (perRowCallback !== undefined && typeof(perRowCallback) === 'function') {
            perRowCallback(row, obj.result.data[i]);
        }
        //data row
        let tds = '';
        row.setAttribute('tabindex', 0);
        //set artist and album data
        if (obj.result.data[i].Album !== undefined) {
            setCustomDomProperty(row, 'data-album', obj.result.data[i].Album);
        }
        if (obj.result.data[i][tagAlbumArtist] !== undefined) {
            setCustomDomProperty(row, 'data-albumartist', obj.result.data[i][tagAlbumArtist]);
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
            if (obj.result.data[i].Type === 'parentDir') {
                row.innerHTML = '<td colspan="' + (colspan + 1) + '">..</td>';
                row.setAttribute('title', t('Open parent folder'));
            }
            else {
                for (let c = 0, d = settings['cols' + list].length; c < d; c++) {
                    tds += '<td data-col="' + encodeURI(settings['cols' + list][c]) + '">' +
                        printValue(settings['cols' + list][c], obj.result.data[i][settings['cols' + list][c]]) +
                        '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="mi color-darkgrey" title="' + t('Actions') + '">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
            }
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

    if (nrItems === 1000) {
        const row = document.createElement('tr');
        row.classList.add('not-clickable');
        row.innerHTML = '<td><span class="mi">warning</span></td><td colspan="' + colspan +'">' + t('Too many results, list is cropped') + '</td>';
        tbody.append(row);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);

    if (nrItems === 0) {
        tbody.appendChild(emptyRow(colspan + 1));
    }
    table.classList.remove('opacity05');
}

function emptyRow(colspan) {
    const tr = elCreate('tr', {"class": ["not-clickable"]}, '');
    const td = elCreate('td', {"colspan": colspan}, '');
    addIconLine(td, 'info', tn('Empty list'));
    tr.appendChild(td);
    return tr;
}

function errorRow(obj, colspan) {
    const tr = elCreate('tr', {"class": ["not-clickable"]}, '');
    const td = elCreate('td', {"colspan": colspan}, '');
    const div = elCreate('div', {"class": ["alert", "alert-danger"]}, '');
    addIconLine(div, 'error_outline', tn(obj.error.message, obj.error.data));
    td.appendChild(div);
    tr.appendChild(td);
    return tr;
}

function checkResult(obj, tbody, colspan) {
    const list = tbody;
    if (typeof tbody === 'string') {
        tbody = document.getElementById(tbody + 'List').getElementsByTagName('tbody')[0];
    }
    if (colspan === null) {
        colspan = settings['cols' + list] !== undefined ? settings['cols' + list].length : 0;
        colspan++;
    }

    if (obj.error) {
        elClear(tbody);
        tbody.appendChild(errorRow(obj, colspan));
        tbody.parentNode.classList.remove('opacity05');
        return false;
    }
    if (obj.result.returnedEntities === 0) {
        elClear(tbody);
        tbody.appendChild(emptyRow(colspan));
        tbody.parentNode.classList.remove('opacity05');
        return false;
    }
    return true;
}
