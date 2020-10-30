"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = event.target.getAttribute('data-tag');
        if (tag === 'Playlists' || tag === 'Filesystem') {
            appGoto('Browse', tag, undefined);
            return;
        }
        
        if (app.current.app === 'Browse' && app.current.tab !== 'Database') {
            let view = app.apps.Browse.tabs.Database.active;
            appGoto('Browse', 'Database', view);
            return;
        }
        if (tag !== 'Album') {
            app.current.filter = tag;
            app.current.sort = tag;
        }
        app.current.search = '';
        document.getElementById('searchDatabaseMatch').value = 'contains';
        appGoto(app.current.app, app.current.tab, app.current.view, 
            '0', app.current.filter, app.current.sort, tag, app.current.search);
    }
}

function gotoBrowse() {
    let x = event.target;
    let tag = x.getAttribute('data-tag');
    let name = decodeURI(x.getAttribute('data-name'));
    if (tag === null) {
        tag = x.parentNode.getAttribute('data-tag');
        name = decodeURI(x.parentNode.getAttribute('data-name'));
    }
    if (tag !== '' && name !== '' && name !== '-' && settings.browsetags.includes(tag)) {
        appGoto('Browse', 'Database', 'List', '0', tag, 'AlbumArtist', 'Album', name);
    }
}

function parseFilesystem(obj) {
    let list = app.current.app + (app.current.tab === 'Filesystem' ? app.current.tab : '');
    let table = document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List');
    let tbody = table.getElementsByTagName('tbody')[0];
    let colspan = settings['cols' + list].length;
    colspan--;

    if (obj.error) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t(obj.error.message) + '</td></tr>';
        document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
        //document.getElementById('cardFooterBrowse').innerText = '';
        return;
    }
    
    if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        const imageList = document.getElementById('BrowseFilesystemImages');
        imageList.innerHTML = '';
        if ((obj.result.images.length === 0 && obj.result.bookletPath === '') || settings.publish === false) {
            imageList.classList.add('hide');
        }
        else {
            imageList.classList.remove('hide');
        }
        if (obj.result.bookletPath !== '' && settings.publish === true) {
            let img = document.createElement('div');
            img.style.backgroundImage = 'url("' + subdir + '/assets/coverimage-booklet.svg")';
            img.classList.add('booklet');
            img.setAttribute('data-href', subdir + '/browse/music/' + obj.result.bookletPath);
            img.title = t('Booklet');
            imageList.appendChild(img);
        }
        for (let i = 0; i < obj.result.images.length; i++) {
            let img = document.createElement('div');
            img.style.backgroundImage = 'url("' + subdir + '/browse/music/' + obj.result.images[i] + '"),url("assets/coverimage-loading.svg")';
            imageList.appendChild(img);
        }
    }
    let nrItems = obj.result.returnedEntities;
    let tr = tbody.getElementsByTagName('tr');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    for (let i = 0; i < nrItems; i++) {
        let uri = encodeURI(obj.result.data[i].uri);
        let row = document.createElement('tr');
        let tds = '';
        row.setAttribute('data-type', obj.result.data[i].Type);
        row.setAttribute('data-uri', uri);
        row.setAttribute('tabindex', 0);
        if (obj.result.data[i].Type === 'song') {
            row.setAttribute('data-name', obj.result.data[i].Title);
        }
        else {
            row.setAttribute('data-name', obj.result.data[i].name);
        }
        
        switch(obj.result.data[i].Type) {
            case 'dir':
            case 'smartpls':
            case 'plist':
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        if (obj.result.data[i].Type === 'dir') {
                            tds += '<span class="material-icons">folder_open</span>';
                        }
                        else {
                            tds += '<span class="material-icons">' + (obj.result.data[i].Type === 'smartpls' ? 'queue_music' : 'list') + '</span>';
                        }
                    }
                    else if (settings['cols' + list][c] === 'Title') {
                        tds += e(obj.result.data[i].name);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                break;
            case 'song':
                obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        tds += '<span class="material-icons">music_note</span>';
                    }
                    else {
                        tds += e(obj.result.data[i][settings['cols' + list][c]]);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                break;
        }
        if (i < tr.length) {
            activeRow = replaceTblRow(tr[i], row) === true ? i : activeRow;
        }
        else {
            tbody.append(row);
        }
    }
    let trLen = tr.length - 1;
    for (let i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }

    if (navigate === true) {
        focusTable(0);
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
                    
    if (nrItems === 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }
    document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
    //document.getElementById('cardFooterBrowse').innerText = t('Num entries', obj.result.totalEntities);
}

function addAllFromBrowseFilesystem() {
    sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": app.current.search});
    showNotification(t('Added all songs'), '', '', 'success');
}

function addAllFromBrowseDatabasePlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI("MPD_API_DATABASE_SEARCH", {"plist": plist, "filter": app.current.view, "searchstr": app.current.search, "offset": 0, "cols": settings.colsSearch, "replace": false});
    }
}

function parseBookmarks(obj) {
    let list = '<table class="table table-sm table-dark table-borderless mb-0">';
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        list += '<tr data-id="' + obj.result.data[i].id + '" data-type="' + obj.result.data[i].type + '" ' +
                'data-uri="' + encodeURI(obj.result.data[i].uri) + '">' +
                '<td class="nowrap"><a class="text-light" href="#" data-href="goto">' + e(obj.result.data[i].name) + '</a></td>' +
                '<td><a class="text-light material-icons material-icons-small" href="#" data-href="edit">edit</a></td><td>' +
                '<a class="text-light material-icons material-icons-small" href="#" data-href="delete">delete</a></td></tr>';
    }
    if (obj.result.returnedEntities === 0) {
        list += '<tr><td class="text-light nowrap">' + t('No bookmarks found') + '</td></tr>';
    }
    list += '</table>';
    document.getElementById('BrowseFilesystemBookmarks').innerHTML = list;
}

function showBookmarkSave(id, name, uri, type) {
    document.getElementById('saveBookmarkName').classList.remove('is-invalid');
    document.getElementById('saveBookmarkId').value = id;
    document.getElementById('saveBookmarkName').value = name;
    document.getElementById('saveBookmarkUri').value = uri;
    document.getElementById('saveBookmarkType').value = type;
    modalSaveBookmark.show();
}

//eslint-disable-next-line no-unused-vars
function saveBookmark() {
    let id = parseInt(document.getElementById('saveBookmarkId').value);
    let name = document.getElementById('saveBookmarkName').value;
    let uri = document.getElementById('saveBookmarkUri').value;
    let type = document.getElementById('saveBookmarkType').value;
    if (name !== '') {
        sendAPI("MYMPD_API_BOOKMARK_SAVE", {"id": id, "name": name, "uri": uri, "type": type});
        modalSaveBookmark.hide();
    }
    else {
        document.getElementById('saveBookmarkName').classList.add('is-invalid');
    }
}

function parseDatabase(obj) {
    let nrItems = obj.result.returnedEntities;
    let cardContainer = document.getElementById('BrowseDatabaseCards');
    let cols = cardContainer.getElementsByClassName('col');
    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        let col = document.createElement('div');
        col.classList.add('col', 'px-0', 'flex-grow-0');
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = t('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = t('Unknown album');
        }
        let id;
        let html;
        let picture = '';
        if (obj.result.tag === 'Album') {
            id = genId('database' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist);
            picture = subdir + '/albumart/' + encodeURI(obj.result.data[i].FirstSongUri);
            html = '<div class="card card-grid clickable" data-picture="' + picture  + '" ' + 
                       'data-uri="' + encodeURI(obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, '')) + '" ' +
                       'data-type="dir" data-name="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-album="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-albumartist="' + encodeURI(obj.result.data[i].AlbumArtist) + '" tabindex="0">' +
                   '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' +
                   '<div class="card-footer card-footer-grid p-2" title="' + obj.result.data[i].AlbumArtist + ': ' + obj.result.data[i].Album + '">' +
                   obj.result.data[i].Album + '<br/><small>' + obj.result.data[i].AlbumArtist + '</small>' +
                   '</div></div>';
        }
        else {
            id = genId('database' + obj.result.data[i].value);
            picture = subdir + '/tagpics/' + obj.result.tag + '/' + encodeURI(obj.result.data[i].value);
            html = '<div class="card card-grid clickable" data-picture="' + picture + '" data-tag="' + encodeURI(obj.result.data[i].value) + '" tabindex="0">' +
                   (obj.result.pics === true ? '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' : '') +
                   '<div class="card-footer card-footer-grid p-2" title="' + obj.result.data[i].value + '">' +
                   obj.result.data[i].value + '<br/>' +
                   '</div></div>';
        }
        col.innerHTML = html;
        let replaced = false;
        if (i < cols.length) {
            if (cols[i].firstChild.getAttribute('data-picture') !== col.firstChild.getAttribute('data-picture')) {
                cols[i].replaceWith(col);
                replaced = true;
            }
        }
        else {
            cardContainer.append(col);
            replaced = true;
        }
        if ('IntersectionObserver' in window && replaced === true) {
            let options = {
                root: null,
                rootMargin: '0px',
            };
            let observer = new IntersectionObserver(setGridImage, options);
            observer.observe(col);
        }
        else if (replaced === true) {
            col.firstChild.firstChild.style.backgroundImage = picture;
        }
        if (replaced === true) {
            col.firstChild.addEventListener('click', function(event) {
                if (app.current.tag === 'Album') {
                    if (event.target.classList.contains('card-body')) {
                        appGoto('Browse', 'Database', 'Detail', '0', 'Album', 'AlbumArtist', 
                            decodeURI(event.target.parentNode.getAttribute('data-album')), 
                            decodeURI(event.target.parentNode.getAttribute('data-albumartist')));
                    }
                    else if (event.target.classList.contains('card-footer')){
                        showMenu(event.target, event);                
                    }
                }
                else {
                    app.current.search = '';
                    document.getElementById('searchDatabaseStr').value = '';
                    appGoto(app.current.app, app.current.card, undefined, '0', 'Album', 'AlbumArtist', 'Album', 
                        '(' + app.current.tag + ' == \'' + decodeURI(event.target.parentNode.getAttribute('data-tag')) + '\')');
                }
            }, false);
            col.firstChild.addEventListener('keydown', function(event) {
                let handled = false;
                if (event.key === 'Enter') {
                    if (app.current.tag === 'Album') {
                        appGoto('Browse', 'Database', 'Detail', '0','Album','AlbumArtist', 
                            decodeURI(event.target.getAttribute('data-album')),
                            decodeURI(event.target.getAttribute('data-albumartist')));
                    }
                    else {
                        app.current.search = '';
                        document.getElementById('searchDatabaseStr').value = '';
                        appGoto(app.current.app, app.current.card, undefined, '0', 'Album', 'AlbumArtist', 'Album',
                            '(' + app.current.tag + ' == \'' + decodeURI(event.target.getAttribute('data-tag')) + '\')');
                    }
                    handled = true;
                }
                else if (event.key === ' ') {
                    if (app.current.tag === 'Album') {
                        showMenu(event.target.getElementsByClassName('card-footer')[0], event);
                    }
                    handled = true;
                }
                else if (event.key === 'ArrowDown' || event.key === 'ArrowUp') {
                    const cur = event.target;
                    const next = event.key === 'ArrowDown' ? (event.target.parentNode.nextElementSibling !== null ? event.target.parentNode.nextElementSibling.firstChild : null)
                                                           : (event.target.parentNode.previousElementSibling !== null ? event.target.parentNode.previousElementSibling.firstChild : null);
                    if (next !== null) {
                        next.focus();
                        cur.classList.remove('selected');
                        next.classList.add('selected');
                        handled = true;
                        scrollFocusIntoView();
                    }
                }
                else if (event.key === 'Escape') {
                    const cur = event.target;
                    cur.blur();
                    cur.classList.remove('selected');
                    handled = true;
                }
                if (handled === true) {
                    event.stopPropagation();
                    event.preventDefault();
                }
            }, false);
        }
    }
    let colsLen = cols.length - 1;
    for (let i = colsLen; i >= nrItems; i --) {
        cols[i].remove();
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div><span class="material-icons">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
    }
    //document.getElementById('cardFooterBrowse').innerText = gtPage('Num entries', obj.result.returnedEntities, obj.result.totalEntities);
}

function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = decodeURI(change.target.firstChild.getAttribute('data-picture'));
            const body = change.target.firstChild.getElementsByClassName('card-body')[0];
            if (body) {
                body.style.backgroundImage = 'url("' + uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
            }
        }
    });
}

function parseAlbumDetails(obj) {
    const coverEl = document.getElementById('viewDetailDatabaseCover');
    coverEl.style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.data[0].uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    coverEl.setAttribute('data-images', obj.result.images.join(';;'));
    const infoEl = document.getElementById('viewDetailDatabaseInfo');
    infoEl.innerHTML = '<h1>' + e(obj.result.Album) + '</h1>' +
        '<small> ' + t('AlbumArtist') + '</small><p>' + e(obj.result.AlbumArtist) + '</p>' +
        (obj.result.bookletPath === '' || settings.featBrowse === false ? '' : 
            '<span class="text-light material-icons">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>') +
        '</p>';
    const table = document.getElementById('BrowseDatabaseDetailList');
    const tbody = table.getElementsByTagName('tbody')[0];
    const nrCols = settings.colsBrowseDatabaseDetail.length;
    let titleList = '';
    if (obj.result.Discs > 1) {
        titleList = '<tr class="not-clickable"><td><span class="material-icons">album</span></td><td colspan="' + nrCols +'">' + t('Disc 1') + '</td></tr>';
    }
    let nrItems = obj.result.returnedEntities;
    let lastDisc = parseInt(obj.result.data[0].Disc);
    for (let i = 0; i < nrItems; i++) {
        if (lastDisc < parseInt(obj.result.data[i].Disc)) {
            titleList += '<tr class="not-clickable"><td><span class="material-icons">album</span></td><td colspan="' + nrCols +'">' + 
                t('Disc') + ' ' + e(obj.result.data[i].Disc) + '</td></tr>';
        }
        if (obj.result.data[i].Duration) {
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        }
        titleList += '<tr tabindex="0" data-type="song" data-name="' + obj.result.data[i].Title + '" data-uri="' + encodeURI(obj.result.data[i].uri) + '">';
        for (let c = 0; c < settings.colsBrowseDatabaseDetail.length; c++) {
            titleList += '<td data-col="' + settings.colsBrowseDatabaseDetail[c] + '">' + e(obj.result.data[i][settings.colsBrowseDatabaseDetail[c]]) + '</td>';
        }
        titleList += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td></tr>';
        lastDisc = obj.result.data[i].Disc;
    }
    tbody.innerHTML = titleList;
    //document.getElementById('cardFooterBrowse').innerHTML = t('Num songs', obj.result.totalEntities) + ' &ndash; ' + beautifyDuration(obj.result.totalTime);
    document.getElementById('BrowseDatabaseDetailList').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function backToAlbumGrid() {
    appGoto('Browse', 'Database', 'List');
}  

//eslint-disable-next-line no-unused-vars
function addAlbum(action) {
    const album = decodeURI(app.current.tag);
    const albumArtist = decodeURI(app.current.search);
    const expression = '((Album == \'' + album + '\') AND (AlbumArtist == \'' + albumArtist + '\'))';
    if (action === 'appendQueue') {
        addAllFromSearchPlist('queue', expression, false);
    }
    else if (action === 'replaceQueue') {
        addAllFromSearchPlist('queue', expression, true);
    }
    else if (action === 'addPlaylist') {
        showAddToPlaylist('ALBUM', expression);
    }
}

function searchAlbumgrid(x) {
    let expression = '';
    let crumbs = document.getElementById('searchDatabaseCrumb').children;
    for (let i = 0; i < crumbs.length; i++) {
        if (i > 0) {
            expression += ' AND ';
        }
        expression += '(' + decodeURI(crumbs[i].getAttribute('data-filter')) + ')';
    }
    if (x !== '') {
        if (expression !== '') {
            expression += ' AND ';
        }
        let match = document.getElementById('searchDatabaseMatch');
        expression += '(' + app.current.filter + ' ' + match.options[match.selectedIndex].value + ' \'' + x +'\')';
    }
    
    if (expression.length <= 2) {
        expression = '';
    }
    appGoto(app.current.app, app.current.tab, app.current.view, 
        '0', app.current.filter, app.current.sort, app.current.tag, expression);
}
