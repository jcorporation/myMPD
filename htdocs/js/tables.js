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
        for (let i = 0; i < tables.length; i++) {
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
        for (let i = 0; i < tables.length; i++) {
            if (tables[i].classList.contains('selected')) {
                table = tables[i];
                break;
            }
        }
        table.focus();
        return;
    }

    if (table !== null) {
        let sel = table.getElementsByClassName('selected');
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
            let rows = table.getElementsByTagName('tbody')[0].rows;
            let rowsLen = rows.length;
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
    let el = document.activeElement;
    let posY = el.getBoundingClientRect().top;
    let height = el.offsetHeight;
    let headerHeight = el.parentNode.parentNode.offsetTop;
    if (window.innerHeight > window.innerWidth) {
        headerHeight += document.getElementById('header').offsetHeight;
    }
    let footerHeight = document.getElementsByTagName('footer')[0].offsetHeight;
    let parentHeight = window.innerHeight - headerHeight - footerHeight;
    let treshold = height / 2;
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
    let cur = document.activeElement;
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
            let popupBtn = cur.lastChild.firstChild;
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
    let tableBody = document.getElementById(table).getElementsByTagName('tbody')[0];
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName === 'TR') {
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
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
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
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
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
        let oldSongpos = getAttDec(document.getElementById(event.dataTransfer.getData('Text')), 'data-songpos');
        let newSongpos = getAttDec(target, 'data-songpos');
        document.getElementById(event.dataTransfer.getData('Text')).remove();
        dragEl.classList.remove('opacity05');
        tableBody.insertBefore(dragEl, target);
        let tr = tableBody.getElementsByClassName('dragover');
        let trLen = tr.length;
        for (let i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        document.getElementById(table).classList.add('opacity05');
        if (app.current.app === 'Queue' && app.current.tab === 'Current') {
            sendAPI("MPD_API_QUEUE_MOVE_TRACK", {"from": oldSongpos, "to": newSongpos});
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
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
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
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
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
        let th = tableHeader.getElementsByClassName('dragover-th');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
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
    let tags = settings.tags.slice();
    if (settings.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    if (table === 'QueueCurrent' || table === 'BrowsePlaylistsDetail' || table === 'QueueLastPlayed' || table === 'QueueJukebox') {
        tags.push('Pos');
    }
    if (table === 'BrowseFilesystem') {
        tags.push('Type');
    }
    if (table === 'QueueLastPlayed') {
        tags.push('LastPlayed');
    }
    if (table === 'Search') {
        tags.push('LastModified');
    }
    if (table === 'Playback') {
        tags.push('Filetype');
        tags.push('Fileformat');
        tags.push('LastModified');
        if (settings.featLyrics === true) {
            tags.push('Lyrics');
        }
    }
    tags.sort();
    return tags;
}

function setColsChecklist(table) {
    let tagChks = '';
    let tags = setColTags(table);
    for (let i = 0; i < tags.length; i++) {
        if (table === 'Playback' && tags[i] === 'Title') {
            continue;
        }
        tagChks += '<div>' +
            '<button class="btn btn-secondary btn-xs clickable mi mi-small' +
            (settings['cols' + table].includes(tags[i]) ? ' active' : '') + '" name="' + tags[i] + '">' +
            (settings['cols' + table].includes(tags[i]) ? 'check' : 'radio_button_unchecked') + '</button>' +
            '<label class="form-check-label" for="' + tags[i] + '">&nbsp;&nbsp;' + t(tags[i]) + '</label>' +
            '</div>';
    }
    return tagChks;
}

function setCols(table) {
    let colsChkList = document.getElementById(table + 'ColsDropdown');
    if (colsChkList) {
        colsChkList.firstChild.innerHTML = setColsChecklist(table);
    }
    let sort = app.current.sort;
    
    if (table === 'Search' && app.apps.Search.sort === 'Title') {
        if (settings.tags.includes('Title')) {
            sort = 'Title';
        }
        else if (settings.featTags === false) {
            sort = 'Filename';
        }
        else {
            sort = '-';
        }
    }
    
    if (table !== 'Playback') {
        let heading = '';
        for (let i = 0; i < settings['cols' + table].length; i++) {
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
        if (settings.featTags === true) {
            heading += '<th data-col="Action"><a data-title-phrase="' +t('Columns') + '" href="#" class="text-secondary align-middle mi mi-small">settings</a></th>';
        }
        else {
            heading += '<th></th>';
        }
        document.getElementById(table + 'List').getElementsByTagName('tr')[0].innerHTML = heading;
    }
}

function saveCols(table, tableEl) {
    let colsDropdown = document.getElementById(table + 'ColsDropdown');
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
        let colInputs = colsDropdown.firstChild.getElementsByTagName('button');
        for (let i = 0; i < colInputs.length; i++) {
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
    
    let params = {"table": "cols" + table, "cols": []};
    let ths = header.getElementsByTagName('th');
    for (let i = 0; i < ths.length; i++) {
        let name = ths[i].getAttribute('data-col');
        if (name !== 'Action' && name !== null) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveColsPlayback(table) {
    let colInputs = document.getElementById(table + 'ColsDropdown').firstChild.getElementsByTagName('button');
    let header = document.getElementById('cardPlaybackTags');

    for (let i = 0; i < colInputs.length -1; i++) {
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
            setAttEnc(th, 'data-tag', colInputs[i].name);
            header.appendChild(th);
        }
    }
    
    let params = {"table": "cols" + table, "cols": []};
    let ths = header.getElementsByTagName('div');
    for (let i = 0; i < ths.length; i++) {
        let name = getAttDec(ths[i], 'data-tag');
        if (name) {
            params.cols.push(name);
        }
    }
    sendAPI("MYMPD_API_COLS_SAVE", params, getSettings);
}

function replaceTblRow(row, el) {
    let menuEl = row.querySelector('[data-popover]');
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
