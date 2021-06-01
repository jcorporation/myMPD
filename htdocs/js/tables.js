"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function focusTable(rownr, table) {
    if (table === undefined) {
        table = document.getElementById(app.current.app + (app.current.tab !== undefined ? app.current.tab : '') + (app.current.view !== undefined ? app.current.view : '') + 'List');
    }

    if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        const tables = document.getElementsByClassName('card-grid');
        if (tables.length === 0 ) {
            return; 
        }
        table = tables[0];
        for (let i = 0, j = tables.length; i < j; i++) {
            if (tables[i].classList.contains('selected')) {
                table = tables[i];
                break;
            }
        }
        table.focus();
        return;
    }
    else if (app.current.app === 'Home') {
        const tables = document.getElementsByClassName('home-icons');
        if (tables.length === 0 ) {
            return; 
        }
        table = tables[0];
        for (let i = 0, j = tables.length; i < j; i++) {
            if (tables[i].classList.contains('selected')) {
                table = tables[i];
                break;
            }
        }
        table.focus();
        return;
    }

    if (table !== null) {
        const sel = table.getElementsByClassName('selected');
        if (rownr === undefined) {
            if (sel.length === 0) {
                let row = table.getElementsByTagName('tbody')[0].rows[0];
                if (row === null) {
                    return;
                }
                if (row.classList.contains('not-clickable')) {
                    row = table.getElementsByTagName('tbody')[0].rows[1];
                }
                if (row === null) {
                    return;
                }
                row.focus();
                row.classList.add('selected');
            }
            else {
                sel[0].focus();
            }
        }
        else {
            if (sel && sel.length > 0) {
                sel[0].classList.remove('selected');
            }
            const rows = table.getElementsByTagName('tbody')[0].rows;
            const rowsLen = rows.length;
            if (rowsLen < rownr) {
                rownr = 0;
            }
            if (rowsLen > rownr) {
                rows[rownr].focus();
                rows[rownr].classList.add('selected');
            }
        }
        scrollFocusIntoView();
    }
}

function scrollFocusIntoView() {
    const el = document.activeElement;
    const posY = el.getBoundingClientRect().top;
    const height = el.offsetHeight;
    let headerHeight = el.parentNode.parentNode.offsetTop;
    if (window.innerHeight > window.innerWidth) {
        headerHeight += document.getElementById('header').offsetHeight;
    }
    const footerHeight = document.getElementsByTagName('footer')[0].offsetHeight;
    const parentHeight = window.innerHeight - headerHeight - footerHeight;
    const treshold = height / 2;
    //console.log('posY: ' + posY);
    //console.log('height: ' + height);
    //console.log('treshold: ' + treshold);
    //console.log('parentHeight: ' + parentHeight);
    //console.log('headerHeight:' + headerHeight);
    //console.log('footerHeight:' + footerHeight);
    if (posY <= headerHeight + treshold) {
        //console.log('0, - height');
        window.scrollBy(0, - height);
    }
    else if (posY + height > parentHeight - treshold) {
        //console.log('0, height');
        window.scrollBy(0, height);
    }
}

function navigateTable(table, keyCode) {
    const cur = document.activeElement;
    if (cur) {
        let next = null;
        let handled = false;
        if (keyCode === 'ArrowDown') {
            next = cur.nextElementSibling;
            if (next === null) {
                return;
            }
            if (next.classList.contains('not-clickable')) {
                next = next.nextElementSibling;
            }
            handled = true;
        }
        else if (keyCode === 'ArrowUp') {
            next = cur.previousElementSibling;
            if (next === null) {
                return;
            }
            if (next.classList.contains('not-clickable')) {
                next = next.previousElementSibling;
            }
            handled = true;
        }
        else if (keyCode === ' ') {
            const popupBtn = cur.lastChild.firstChild;
            if (popupBtn.nodeName === 'A') {
                popupBtn.click();
            }
            handled = true;
        }
        else if (keyCode === 'Enter') {
            cur.firstChild.click();
            handled = true;
        }
        else if (keyCode === 'Escape') {
            cur.blur();
            cur.classList.remove('selected');
            handled = true;
        }
        if (handled === true) {
            event.preventDefault();
            event.stopPropagation();
        }
        if (next) {
            cur.classList.remove('selected');
            next.classList.add('selected');
            next.focus();
            scrollFocusIntoView();
        }
    }
}

function dragAndDropTable(table) {
    const tableBody = document.getElementById(table).getElementsByTagName('tbody')[0];
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
            hideMenu();
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
        const oldSongpos = getCustomDomProperty(document.getElementById(event.dataTransfer.getData('Text')), 'data-songpos');
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
    if (table === 'QueueCurrent' || table === 'BrowsePlaylistsDetail' || table === 'QueueLastPlayed' || table === 'QueueJukebox') {
        tags.push('Pos');
    }
    if (table === 'BrowseFilesystem') {
        tags.push('Type');
        tags.push('Filename');
    }
    if (table === 'Playback') {
        tags.push('Filetype');
        tags.push('Fileformat');
        if (settings.webuiSettings.uiLyrics === true) {
            tags.push('Lyrics');
        }
    }
    if (table === 'QueueLastPlayed') {
        tags.push('LastPlayed');
    }
    tags.sort();
    tags.push('dropdownTitleSticker');
    if (features.featStickers === true) {
        for (const sticker of stickerList) {
            tags.push(sticker);
        }
    }
    return tags;
}

function setColsChecklist(table) {
    let tagChks = '';
    const tags = setColTags(table);
    for (let i = 0, j = tags.length; i < j; i++) {
        if (table === 'Playback' && tags[i] === 'Title') {
            continue;
        }
        if (tags[i] === 'dropdownTitleSticker') {
            tagChks += '<h6 class="dropdown-header pl-0">' + t('Sticker') + '</h6>';
        }
        else {
            tagChks += '<div>' +
                '<button class="btn btn-secondary btn-xs clickable mi mi-small' +
                (settings['cols' + table].includes(tags[i]) ? ' active' : '') + '" name="' + tags[i] + '">' +
                (settings['cols' + table].includes(tags[i]) ? 'check' : 'radio_button_unchecked') + '</button>' +
                '<label class="form-check-label" for="' + tags[i] + '">&nbsp;&nbsp;' + t(tags[i]) + '</label>' +
                '</div>';
        }
    }
    return tagChks;
}

function setCols(table) {
    const colsChkList = document.getElementById(table + 'ColsDropdown');
    if (colsChkList) {
        colsChkList.firstChild.innerHTML = setColsChecklist(table);
    }
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
    
    if (table !== 'Playback') {
        let heading = '';
        for (let i = 0, j = settings['cols' + table].length; i < j; i++) {
            let h = settings['cols' + table][i];
            heading += '<th draggable="true" data-col="' + h  + '">';
            if (h === 'Track' || h === 'Pos') {
                h = '#';
            }
            heading += t(h);

            if (table === 'Search' && (h === sort || '-' + h === sort) ) {
                let sortdesc = false;
                if (app.current.sort.indexOf('-') === 0) {
                    sortdesc = true;
                }
                heading += '<span class="sort-dir mi pull-right">' + (sortdesc === true ? 'arrow_drop_up' : 'arrow_drop_down') + '</span>';
            }
            heading += '</th>';
        }
        if (features.featTags === true) {
            heading += '<th data-col="Action"><a data-title-phrase="' +t('Columns') + '" href="#" class="text-secondary align-middle mi mi-small">settings</a></th>';
        }
        else {
            heading += '<th></th>';
        }
        document.getElementById(table + 'List').getElementsByTagName('tr')[0].innerHTML = heading;
    }
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
                th.innerText = colInputs[i].name;
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
    let result = false;
    if (menuEl) {
        hideMenu();
    }
    if (row.classList.contains('selected')) {
        el.classList.add('selected');
        el.focus();
        result = true;
    }
    row.replaceWith(el);
    return result;
}

function updateTable(obj, list, perRowCallback, createRowCellsCallback) {
    const table = document.getElementById(list + 'List');
    const tbody = table.getElementsByTagName('tbody')[0];
    const colspan = settings['cols' + list] !== undefined ? settings['cols' + list].length : 0;

    if (obj.error) {
        tbody.innerHTML = '<tr class="not-clickable"><td colspan="' + (colspan + 1) + '">' +
            '<div class="alert alert-danger">' +
            '<span class="mi">error_outline</span>&nbsp;&nbsp;' + t(obj.error.message, obj.error.data) + '</div></td></tr>';
        table.classList.remove('opacity05');
        return;
    }

    const nrItems = obj.result.returnedEntities;
    const tr = tbody.getElementsByTagName('tr');
    const navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    //disc handling for album view
    let z = 0;
    let lastDisc = obj.result.data.length > 0 && obj.result.data[0].Disc !== undefined ? Number(obj.result.data[0].Disc) : 0;
    if (obj.result.Discs !== undefined && obj.result.Discs > 1) {
        const row = document.createElement('tr');
        row.classList.add('not-clickable');
        row.innerHTML = '<td><span class="mi">album</span></td><td colspan="' + colspan +'">' + t('Disc 1') + '</td>';
        if (z < tr.length) {
            activeRow = replaceTblRow(tr[z], row) === true ? z : activeRow;
        }
        else {
            tbody.append(row);
        }
        z++;
    }
    for (let i = 0; i < nrItems; i++) {
        if (obj.result.data[0].Disc !== undefined && lastDisc < Number(obj.result.data[i].Disc)) {
            const row = document.createElement('tr');
            row.classList.add('not-clickable');
            row.innerHTML = '<td><span class="mi">album</span></td><td colspan="' + colspan +'">' + 
                t('Disc') + ' ' + e(obj.result.data[i].Disc) + '</td></tr>';
            if (i + z < tr.length) {
                activeRow = replaceTblRow(tr[i + z], row) === true ? i + z : activeRow;
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
        let tds = '';
        row.setAttribute('tabindex', 0);
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
            activeRow = replaceTblRow(tr[i + z], row) === true ? i + z : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    const trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems + z; i --) {
        tr[i].remove();
    }

    if (navigate === true) {
        focusTable(activeRow);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);

    if (nrItems === 0) {
        tbody.innerHTML = '<tr class="not-clickable"><td colspan="' + (colspan + 1) + '">' +
            '<span class="mi">info</span>&nbsp;&nbsp;' + t('Empty list') + '</td></tr>';
    }
    table.classList.remove('opacity05');
}
