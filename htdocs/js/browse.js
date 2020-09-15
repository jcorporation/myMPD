"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function gotoBrowse(x) {
    let tag = x.parentNode.getAttribute('data-tag');
    let name = decodeURI(x.parentNode.getAttribute('data-name'));
    if (tag !== '' && name !== '' && name !== '-' && settings.browsetags.includes(tag)) {
        appGoto('Browse', 'Database', tag, '0/-/-/' + name);
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
        document.getElementById('cardFooterBrowse').innerText = '';
        return;
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
    document.getElementById('cardFooterBrowse').innerText = t('Num entries', obj.result.totalEntities);
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

function parseCovergrid(obj) {
    let nrItems = obj.result.returnedEntities;
    let cardContainer = document.getElementById('BrowseCovergridList');
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
        let id = genId('covergrid' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist);
        let html;
        let picture = '';
        if (obj.result.tag === 'Album') {
            id = genId('covergrid' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist);
            picture = subdir + '/albumart/' + encodeURI(obj.result.data[i].FirstSongUri);
            html = '<div class="card card-grid clickable" data-picture="' + picture  + '" ' + 
                       'data-album="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-albumartist="' + encodeURI(obj.result.data[i].AlbumArtist) + '" tabindex="0">' +
                   '<div class="card-header covergrid-header hide unvisible"></div>' +
                   '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' +
                   '<div class="card-footer card-footer-grid p-2" title="' + obj.result.data[i].AlbumArtist + ': ' + obj.result.data[i].Album + '">' +
                   obj.result.data[i].Album + '<br/><small>' + obj.result.data[i].AlbumArtist + '</small>' +
                   '</div></div>';
        }
        else {
            id = genId('covergrid' + obj.result.data[i].value);
            picture = subdir + '/browse/pics/' + obj.result.tag + '/' + encodeURI(obj.result.data[i].value) + '.jpg';
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
                        getCovergridTitleList(id);
                    }
                    else if (event.target.classList.contains('card-footer')){
                        showMenu(event.target, event);                
                    }
                }
                else {
                    appGoto(app.current.app, app.current.card, undefined, '0/' + app.current.tag + 
                        '/AlbumArtist/Album/' + decodeURI(event.target.parentNode.getAttribute('data-tag')));
                }
            }, false);
            col.firstChild.addEventListener('transitionend', function(event) {
                if (event.target.getElementsByClassName('card-body')[0].style.backgroundImage !== '') {
                    return;
                }
                event.target.getElementsByTagName('table')[0].classList.remove('unvisible');
                event.target.getElementsByClassName('card-header')[0].classList.remove('unvisible');
            }, false);
            col.firstChild.addEventListener('keydown', function(event) {
                if (event.key === 'Escape') {
                    if (app.current.tag === 'Album') {
                        let cardBody = event.target.getElementsByClassName('card-body')[0];
                        let uri = decodeURI(cardBody.parentNode.getAttribute('data-picture'));
                        showGridImage(cardBody, uri);
                    }
                }
                else if (event.key === 'Enter') {
                    if (app.current.tag === 'Album') {
                        getCovergridTitleList(id);
                    }
                    else {
                        appGoto(app.current.app, app.current.card, undefined, '0/' + app.current.tag + 
                        '/AlbumArtist/Album/' + decodeURI(event.target.parentNode.getAttribute('data-tag')));
                    }
                    event.stopPropagation();
                    event.preventDefault();
                }
                else if (event.key === ' ') {
                    if (app.current.tag === 'Album') {
                        showMenu(event.target.getElementsByClassName('card-footer')[0], event);
                    }
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
        cardContainer.innerHTML = t('Empty list');
    }
    document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
    document.getElementById('cardFooterBrowse').innerText = gtPage('Num entries', obj.result.returnedEntities, obj.result.totalEntities);
}

function getCovergridTitleList(id) {
    let cardBody = document.getElementById(id);
    let card = cardBody.parentNode;
    card.classList.add('opacity05');
    let s = document.getElementById('BrowseCovergridList').childNodes[1];
    let width;
    if (s) {
        let p = parseInt(window.getComputedStyle(document.getElementById('cardBrowseCovergrid'), null).getPropertyValue('padding-left'));
        width = s.offsetLeft + settings.covergridSize - p;
    }
    else {
        width = settings.covergridSize * 2 + 20;
    }
    cardBody.style.width = width + 'px';
    cardBody.parentNode.style.width = width + 'px';
    sendAPI("MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", {"album": decodeURI(card.getAttribute('data-album')),
        "search": decodeURI(card.getAttribute('data-albumartist')),
        "tag": "AlbumArtist", "cols": settings.colsBrowseDatabase}, parseCovergridTitleList);
}

function parseCovergridTitleList(obj) {
    let id = genId('covergrid' + obj.result.Album + obj.result.AlbumArtist);
    let cardBody = document.getElementById(id);
    
    let titleList = '<table class="table table-hover table-sm unvisible" tabindex="0"><thead>';
    for (let i = 0; i < settings.colsBrowseDatabase.length; i++) {
        let h = settings.colsBrowseDatabase[i];
        if (h === 'Track') {
            h = '#';
        }
        titleList += '<th class="border-top-0">' + t(h) + '</th>';
    }
    titleList += '<th class="border-top-0"></th></thead><tbody class="clickable">';
    let nrItems = obj.result.returnedEntities;
    for (let i = 0; i < nrItems; i++) {
        if (obj.result.data[i].Duration) {
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        }
        titleList += '<tr tabindex="0" data-type="song" data-name="' + obj.result.data[i].Title + '" data-uri="' + encodeURI(obj.result.data[i].uri) + '">';
        for (let c = 0; c < settings.colsBrowseDatabase.length; c++) {
            titleList += '<td data-col="' + settings.colsBrowseDatabase[c] + '">' + e(obj.result.data[i][settings.colsBrowseDatabase[c]]) + '</td>';
        }
        titleList += '<td data-col="Action"><a href="#" class="material-icons color-darkgrey">' + ligatureMore + '</a></td></tr>';
    }
    titleList += '</tbody></table>';

    let uri = decodeURI(cardBody.parentNode.getAttribute('data-picture'));
    let cardFooter = cardBody.parentNode.getElementsByClassName('card-footer')[0];
    let cardHeader = cardBody.parentNode.getElementsByClassName('card-header')[0];
    cardHeader.innerHTML = '<button class="close" type="button">&times;</button><img class="covergrid-header" src="' + uri + '"/>' +
        cardFooter.innerHTML + '';
    cardHeader.classList.remove('hide');
    cardFooter.classList.add('hide');
    
    cardBody.style.backgroundImage = '';
    cardBody.classList.remove('album-cover-loading');
    cardBody.style.height = 'auto';
    
    cardBody.innerHTML = titleList;
    cardBody.parentNode.classList.remove('opacity05');
    cardHeader.getElementsByClassName('close')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        showGridImage(cardBody, uri);
    }, false);

    let table = cardBody.getElementsByTagName('table')[0];
    table.addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute('data-uri')), event.target.parentNode.getAttribute('data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);
    table.addEventListener('keydown', function(event) {
        navigateTable(this, event.key);
        if (event.key === 'Escape') {
            event.target.parentNode.parentNode.parentNode.parentNode.focus();
        }
    }, false);

    //fallback if transitionEnd is not fired
    setTimeout(function() {
        cardBody.getElementsByTagName('table')[0].classList.remove('unvisible');
        cardBody.parentNode.getElementsByClassName('card-header')[0].classList.remove('unvisible');
        scrollFocusIntoView();
    }, 500);
}

function showGridImage(cardBody, uri) {
    cardBody.innerHTML = '';
    cardBody.style.backgroundImage = 'url("' + uri + '")';
    cardBody.style.width =  'var(--mympd-covergridsize, 200px)';
    cardBody.style.height =  'var(--mympd-covergridsize, 200px)';
    cardBody.parentNode.style.width =  'var(--mympd-covergridsize, 200px)';
    cardBody.parentNode.getElementsByClassName('card-footer')[0].classList.remove('hide');
    cardBody.parentNode.getElementsByClassName('card-header')[0].classList.add('hide', 'unvisible');
}

function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = decodeURI(change.target.firstChild.getAttribute('data-picture'));
            const body = change.target.firstChild.getElementsByClassName('card-body')[0];
            if (body) {
                body.style.backgroundImage = 'url("' + uri + '")';
            }
        }
    });
}
