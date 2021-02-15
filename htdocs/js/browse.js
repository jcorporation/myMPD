"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initBrowse() {
    document.getElementById('BrowseDatabaseListList').addEventListener('click', function(event) {
        if (app.current.tag === 'Album') {
            if (event.target.classList.contains('card-body')) {
                appGoto('Browse', 'Database', 'Detail', 0, undefined, 'Album', 'AlbumArtist', 
                    getAttDec(event.target.parentNode, 'data-album'),
                    getAttDec(event.target.parentNode, 'data-albumartist'));
            }
            else if (event.target.classList.contains('card-footer')){
                popoverMenuAlbumCards(event);
            }
        }
        else {
            app.current.search = '';
            document.getElementById('searchDatabaseStr').value = '';
            appGoto(app.current.app, app.current.card, undefined, 0, undefined, 'Album', 'AlbumArtist', 'Album', 
                '((' + app.current.tag + ' == \'' + escapeMPD(getAttDec(event.target.parentNode, 'data-tag')) + '\'))');
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('contextmenu', function(event) {
        if (app.current.tag === 'Album') {
            popoverMenuAlbumCards(event);
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('long-press', function(event) {
        if (app.current.tag === 'Album') {
            popoverMenuAlbumCards(event);
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('keydown', function(event) {
        navigateGrid(event.target, event.key);
    }, false);
    
    if (isMobile === false) {
        document.getElementById('BrowseDatabaseListList').addEventListener('mouseover', function(event) {
            if (event.target.classList.contains('card-body') && event.target.childNodes.length === 0) {
                const oldEls = document.getElementById('BrowseDatabaseListList').getElementsByClassName('album-grid-mouseover');
                if (oldEls.length > 1) {
                    for (let i = 0; i < oldEls.length; i++) {
                        oldEls[i].remove();
                    }
                }
                addPlayButton(event.target);
            }
        }, false);

        document.getElementById('BrowseDatabaseListList').addEventListener('mouseout', function(event) {
            if (event.target.classList.contains('card-body') && (event.relatedTarget === null || ! event.relatedTarget.classList.contains('album-grid-mouseover'))) {
                event.target.innerHTML = '';
            }
        }, false);
    }
    
    document.getElementById('BrowseDatabaseDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            clickSong(getAttDec(event.target.parentNode, 'data-uri'), getAttDec(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('searchDatabaseTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getAttDec(event.target, 'data-tag');
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
        }
    }, false);
    
    document.getElementById('databaseSortDesc').addEventListener('click', function(event) {
        toggleBtnChk(this);
        event.stopPropagation();
        event.preventDefault();
        if (app.current.sort.charAt(0) === '-') {
            app.current.sort = app.current.sort.substr(1);
        }
        else {
            app.current.sort = '-' + app.current.sort;
        }
        appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    document.getElementById('databaseSortTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            event.stopPropagation();
            app.current.sort = getAttDec(event.target, 'data-tag');
            appGoto(app.current.app, app.current.tab, app.current.view, '0', app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);

    document.getElementById('BrowseDatabaseByTagDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);
    
    document.getElementById('BrowseNavPlaylistsDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);
    
    document.getElementById('BrowseNavFilesystemDropdown').addEventListener('click', function(event) {
        navBrowseHandler(event);
    }, false);

    document.getElementById('dropdownSortPlaylistTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            playlistSort(getAttDec(event.target, 'data-tag'));
        }
    }, false);

    document.getElementById('searchFilesystemStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, (this.value !== '' ? this.value : '-'), app.current.sort, '-', app.current.search);
        }
    }, false);
    
    document.getElementById('searchPlaylistsStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);

    document.getElementById('searchDatabaseStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else if (event.key === 'Enter' && app.current.tag === 'Album') {
            if (this.value !== '') {
                const op = getSelectValue(document.getElementById('searchDatabaseMatch'));
                const crumbEl = document.getElementById('searchDatabaseCrumb');
                crumbEl.appendChild(createSearchCrumb(app.current.filter, op, this.value));
                crumbEl.classList.remove('hide');
                this.value = '';
            }
            else {
                searchAlbumgrid(this.value);
            }
        }
        else if (app.current.tag === 'Album') {
            searchAlbumgrid(this.value);
        }
        else {
            appGoto(app.current.app, app.current.tab, app.current.view, 
                '0', app.current.limit, app.current.filter, app.current.sort, app.current.tag, this.value);
        }
    }, false);

    document.getElementById('searchDatabaseMatch').addEventListener('change', function() {
        searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
    });

    document.getElementById('searchDatabaseCrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'SPAN') {
            //remove search expression
            event.preventDefault();
            event.stopPropagation();
            event.target.parentNode.remove();
            searchAlbumgrid('');
        }
        else if (event.target.nodeName === 'BUTTON') {
            //edit search expression
            event.preventDefault();
            event.stopPropagation();
            selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', getAttDec(event.target,'data-filter-tag'));
            document.getElementById('searchDatabaseStr').value = unescapeMPD(getAttDec(event.target, 'data-filter-value'));
            document.getElementById('searchDatabaseMatch').value = getAttDec(event.target, 'data-filter-op');
            event.target.remove();
            app.current.filter = getAttDec(event.target,'data-filter-tag');
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
            if (document.getElementById('searchDatabaseCrumb').childElementCount === 0) {
                document.getElementById('searchDatabaseCrumb').classList.add('hide');
            }
        }
    }, false);

    document.getElementById('BrowseFilesystemList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            const uri = getAttDec(event.target.parentNode, 'data-uri');
            const name = getAttDec(event.target.parentNode, 'data-name');
            const dataType = getAttDec(event.target.parentNode, 'data-type');
            switch(dataType) {
                case 'parentDir':
                    app.current.filter = '-';
                    appGoto('Browse', 'Filesystem', undefined, '0', app.current.limit, app.current.filter, app.current.sort, '-', uri);
                    break;
                case 'dir':
                    clickFolder(uri, name);
                    break;
                case 'song':
                    clickSong(uri, name);
                    break;
                case 'plist':
                    clickPlaylist(uri, name);
                    break;
            }
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemBookmarks').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            let id = getAttDec(event.target.parentNode.parentNode, 'data-id');
            let type = getAttDec(event.target.parentNode.parentNode, 'data-type');
            let uri = getAttDec(event.target.parentNode.parentNode, 'data-uri');
            let name = event.target.parentNode.parentNode.firstChild.innerText;
            let href = getAttDec(event.target, 'data-href');
            
            if (href === 'delete') {
                sendAPI("MYMPD_API_BOOKMARK_RM", {"id": id}, function() {
                    sendAPI("MYMPD_API_BOOKMARK_LIST", {"offset": 0, "limit": 0}, parseBookmarks);
                });
                event.preventDefault();
                event.stopPropagation();
            }
            else if (href === 'edit') {
                showBookmarkSave(id, name, uri, type);
            }
            else if (href === 'goto') {
                appGoto('Browse', 'Filesystem', undefined, '0', undefined, '-','-','-', uri);
            }
        }
    }, false);

    document.getElementById('BrowseBreadcrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            appGoto('Browse', 'Filesystem', undefined, '0', app.current.limit, app.current.filter, app.current.sort, '-', getAttDec(event.target, 'data-uri'));
        }
    }, false);

    document.getElementById('BrowseFilesystemBookmark').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI("MYMPD_API_BOOKMARK_LIST", {"offset": 0, "limit": 0}, parseBookmarks);
    });
}

function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = getAttDec(event.target, 'data-tag');
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
            '0', app.current.limit, app.current.filter, app.current.sort, tag, app.current.search);
    }
}

function popoverMenuAlbumCards(event) {
    showMenu(event.target, event);
    const selCards = document.getElementById('BrowseDatabaseListList').getElementsByClassName('selected');
    for (let i = 0; i < selCards.length; i++) {
        selCards[i].classList.remove('selected');
    }
    event.target.parentNode.classList.add('selected');
    event.preventDefault();
    event.stopPropagation();
}

function gotoBrowse(event) {
    if (settings.featAdvsearch === false) {
        return;
    }
    const x = event.target;
    let tag = getAttDec(x, 'data-tag');
    let name = getAttDec(x, 'data-name');
    if (tag === null) {
        tag = getAttDec(x.parentNode, 'data-tag');
        name = getAttDec(x.parentNode, 'data-name');
    }
    if (tag !== '' && name !== '' && name !== '-' && settings.browsetags.includes(tag)) {
        if (tag === 'Album') {
            let artist = getAttDec(x, 'data-albumartist');
            if (artist === null) {
                artist = getAttDec(x.parentNode, 'data-albumartist');
            }
            if (artist !== null) {
                //Show album details
                appGoto('Browse', 'Database', 'Detail', '0', undefined, tag, tagAlbumArtist, name, artist);
            }
            else {
                //show filtered album list
                document.getElementById('searchDatabaseStr').value = '';
                appGoto('Browse', 'Database', 'List', '0', undefined, tag, tagAlbumArtist, 'Album', '((' + tag + ' == \'' + escapeMPD(name) + '\'))');
            }
        }
        else {
            //show filtered album list
            document.getElementById('searchDatabaseStr').value = '';
            appGoto('Browse', 'Database', 'List', '0', undefined, tag, tagAlbumArtist, 'Album', '((' + tag + ' == \'' + escapeMPD(name) + '\'))');
        }
    }
}

function parseFilesystem(obj) {
    let list = app.current.app + (app.current.tab === 'Filesystem' ? app.current.tab : '');
    let table = document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List');
    let tbody = table.getElementsByTagName('tbody')[0];
    let colspan = settings['cols' + list].length;

    if (obj.error) {
        tbody.innerHTML = '<tr><td><span class="mi">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t(obj.error.message) + '</td></tr>';
        document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
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
            setAttEnc(img, 'data-href', subdir + '/browse/music/' + obj.result.bookletPath);
            img.title = t('Booklet');
            imageList.appendChild(img);
        }
        for (let i = 0; i < obj.result.images.length; i++) {
            let img = document.createElement('div');
            img.style.backgroundImage = 'url("' + subdir + '/browse/music/' + obj.result.images[i] + '"),url("assets/coverimage-loading.svg")';
            imageList.appendChild(img);
        }
    }
    const rowTitleSong = advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong];
    const rowTitleFolder = advancedSettingsDefault.clickFolder.validValues[settings.advanced.clickFolder];
    const rowTitlePlaylist = advancedSettingsDefault.clickPlaylist.validValues[settings.advanced.clickPlaylist];
    let nrItems = obj.result.returnedEntities;
    let tr = tbody.getElementsByTagName('tr');
    let navigate = document.activeElement.parentNode.parentNode === table ? true : false;
    let activeRow = 0;
    for (let i = 0; i < nrItems; i++) {
        let row = document.createElement('tr');
        let tds = '';
        setAttEnc(row, 'data-type', obj.result.data[i].Type);
        setAttEnc(row, 'data-uri', obj.result.data[i].uri);
        row.setAttribute('tabindex', 0);
        if (app.current.app === 'Search' && settings.featTags === true && settings.featAdvsearch === true) {
            //add artist and album information for album actions in search app
            if (obj.result.data[i].Album !== undefined) {
                setAttEnc(row, 'data-album', obj.result.data[i].Album);
            }
            if (obj.result.data[i][tagAlbumArtist] !== undefined) {
                setAttEnc(row, 'data-albumartist', obj.result.data[i][tagAlbumArtist]);
            }
        }
        if (obj.result.data[i].Type === 'song') {
            setAttEnc(row, 'data-name', obj.result.data[i].Title);
        }
        else {
            setAttEnc(row, 'data-name', obj.result.data[i].name);
        }
        
        switch(obj.result.data[i].Type) {
            case 'parentDir':
                row.innerHTML = '<td colspan="' + (colspan + 1) + '">..</td>';
                row.setAttribute('title', t('Open parent folder'));
                break;
            case 'dir':
            case 'smartpls':
            case 'plist':
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        if (obj.result.data[i].Type === 'dir') {
                            tds += '<span class="mi">folder_open</span>';
                        }
                        else {
                            tds += '<span class="mi">' + (obj.result.data[i].Type === 'smartpls' ? 'queue_music' : 'list') + '</span>';
                        }
                    }
                    else if (settings['cols' + list][c] === 'Title') {
                        tds += e(obj.result.data[i].name);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                row.setAttribute('title', t(obj.result.data[i].Type === 'dir' ? rowTitleFolder : rowTitlePlaylist));
                break;
            case 'song':
                if (obj.result.data[i].Duration !== undefined) {
                    obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
                }
                if (obj.result.data[i].LastModified !== undefined) {
                    obj.result.data[i].LastModified = localeDate(obj.result.data[i].LastModified);
                }
                for (let c = 0; c < settings['cols' + list].length; c++) {
                    tds += '<td data-col="' + settings['cols' + list][c] + '">';
                    if (settings['cols' + list][c] === 'Type') {
                        tds += '<span class="mi">music_note</span>';
                    }
                    else {
                        tds += e(obj.result.data[i][settings['cols' + list][c]]);
                    }
                    tds += '</td>';
                }
                tds += '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td>';
                row.innerHTML = tds;
                row.setAttribute('title', t(rowTitleSong));
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
        tbody.innerHTML = '<tr class="not-clickable"><td><span class="mi">error_outline</span></td>' +
                          '<td colspan="' + colspan + '">' + t('Empty list') + '</td></tr>';
    }
    document.getElementById(app.current.app + (app.current.tab === undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
}

//eslint-disable-next-line no-unused-vars
function addAllFromBrowseFilesystem(replace) {
    if (replace === true) {
        sendAPI("MPD_API_QUEUE_REPLACE_TRACK", {"uri": app.current.search});
        showNotification(t('Replaced queue'), '', '', 'success');
    }
    else {
        sendAPI("MPD_API_QUEUE_ADD_TRACK", {"uri": app.current.search});
        showNotification(t('Added all songs'), '', '', 'success');
    }
}

function addAllFromBrowseDatabasePlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI("MPD_API_DATABASE_SEARCH", {"plist": plist, "filter": app.current.view, "searchstr": app.current.search, "offset": 0, "limit": 0, "cols": settings.colsSearch, "replace": false});
    }
}

function parseBookmarks(obj) {
    let list = '<table class="table table-sm table-dark table-borderless mb-0">';
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        list += '<tr data-id="' + obj.result.data[i].id + '" data-type="' + obj.result.data[i].type + '" ' +
                'data-uri="' + encodeURI(obj.result.data[i].uri) + '">' +
                '<td class="nowrap"><a class="text-light" href="#" data-href="goto">' + e(obj.result.data[i].name) + '</a></td>' +
                '<td><a class="text-light mi mi-small" href="#" data-href="edit">edit</a></td><td>' +
                '<a class="text-light mi mi-small" href="#" data-href="delete">delete</a></td></tr>';
    }
    if (obj.result.returnedEntities === 0) {
        list += '<tr><td class="text-light nowrap">' + t('No bookmarks found') + '</td></tr>';
    }
    list += '</table>';
    document.getElementById('BrowseFilesystemBookmarks').innerHTML = list;
}

function showBookmarkSave(id, name, uri, type) {
    removeIsInvalid(document.getElementById('modalSaveBookmark'));
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
    let cardContainer = document.getElementById('BrowseDatabaseListList');
    let cols = cardContainer.getElementsByClassName('col');
    const has_io = 'IntersectionObserver' in window ? true : false;

    document.getElementById('BrowseDatabaseListList').classList.remove('opacity05');

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
            picture = subdir + '/albumart/' + obj.result.data[i].FirstSongUri;
            html = '<div class="card card-grid clickable" data-picture="' + encodeURI(picture)  + '" ' + 
                       'data-uri="' + encodeURI(obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, '')) + '" ' +
                       'data-type="dir" data-name="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-album="' + encodeURI(obj.result.data[i].Album) + '" ' +
                       'data-albumartist="' + encodeURI(obj.result.data[i].AlbumArtist) + '" tabindex="0">' +
                   '<div class="card-body album-cover-loading album-cover-grid bg-white d-flex" id="' + id + '"></div>' +
                   '<div class="card-footer card-footer-grid p-2" title="' + e(obj.result.data[i].AlbumArtist) + ': ' + e(obj.result.data[i].Album) + '">' +
                   e(obj.result.data[i].Album) + '<br/><small>' + e(obj.result.data[i].AlbumArtist) + '</small>' +
                   '</div></div>';
        }
        else {
            id = genId('database' + obj.result.data[i].value);
            picture = subdir + '/tagpics/' + obj.result.tag + '/' + encodeURI(obj.result.data[i].value);
            html = '<div class="card card-grid clickable" data-picture="' + encodeURI(picture) + '" data-tag="' + encodeURI(obj.result.data[i].value) + '" tabindex="0">' +
                   (obj.result.pics === true ? '<div class="card-body album-cover-loading album-cover-grid bg-white" id="' + id + '"></div>' : '') +
                   '<div class="card-footer card-footer-grid p-2" title="' + e(obj.result.data[i].value) + '">' +
                   e(obj.result.data[i].value) + '<br/>' +
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
        if (replaced === true) {
            if (has_io === true) {
                let options = {
                    root: null,
                    rootMargin: '0px',
                };
                let observer = new IntersectionObserver(setGridImage, options);
                observer.observe(col);
            }
            else {
                col.firstChild.firstChild.style.backgroundImage = picture;
            }
            if (obj.result.tag === 'Album' && isMobile === true) {
                addPlayButton(document.getElementById(id));
            }
        }
    }
    let colsLen = cols.length - 1;
    for (let i = colsLen; i >= nrItems; i --) {
        cols[i].remove();
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div class="ml-3 mb-3 not-clickable"><span class="mi">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
    }
}

function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = getAttDec(change.target.firstChild, 'data-picture');
            const body = change.target.firstChild.getElementsByClassName('card-body')[0];
            if (body) {
                body.style.backgroundImage = 'url("' + uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
            }
        }
    });
}

function addPlayButton(parentEl) {
    const div = document.createElement('div');
    div.classList.add('align-self-end', 'album-grid-mouseover', 'mi', 'rounded-circle', 'clickable');
    div.innerText = 'play_arrow';
    div.title = t(advancedSettingsDefault.clickAlbumPlay.validValues[settings.advanced.clickAlbumPlay]);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickAlbumPlay(getAttDec(event.target.parentNode.parentNode, 'data-albumartist'), getAttDec(event.target.parentNode.parentNode, 'data-album'));
    }, false);
}

function parseAlbumDetails(obj) {
    const coverEl = document.getElementById('viewDetailDatabaseCover');
    coverEl.style.backgroundImage = 'url("' + subdir + '/albumart/' + obj.result.data[0].uri + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    setAttEnc(coverEl, 'data-images', obj.result.images.join(';;'));
    setAttEnc(coverEl, 'data-uri', obj.result.data[0].uri);
    const infoEl = document.getElementById('viewDetailDatabaseInfo');
    infoEl.innerHTML = '<h1>' + e(obj.result.Album) + '</h1>' +
        '<small> ' + t('AlbumArtist') + '</small><p>' + e(obj.result.AlbumArtist) + '</p>' +
        (obj.result.bookletPath === '' || settings.featBrowse === false ? '' : 
            '<span class="text-light mi">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>') +
        '</p>';
    const table = document.getElementById('BrowseDatabaseDetailList');
    const tbody = table.getElementsByTagName('tbody')[0];
    const nrCols = settings.colsBrowseDatabaseDetail.length;
    let titleList = '';
    if (obj.result.Discs > 1) {
        titleList = '<tr class="not-clickable"><td><span class="mi">album</span></td><td colspan="' + nrCols +'">' + t('Disc 1') + '</td></tr>';
    }
    let nrItems = obj.result.returnedEntities;
    let lastDisc = parseInt(obj.result.data[0].Disc);
    const rowTitle = t(advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong]);
    for (let i = 0; i < nrItems; i++) {
        if (lastDisc < parseInt(obj.result.data[i].Disc)) {
            titleList += '<tr class="not-clickable"><td><span class="mi">album</span></td><td colspan="' + nrCols +'">' + 
                t('Disc') + ' ' + e(obj.result.data[i].Disc) + '</td></tr>';
        }
        if (obj.result.data[i].Duration) {
            obj.result.data[i].Duration = beautifySongDuration(obj.result.data[i].Duration);
        }
        titleList += '<tr tabindex="0" title="' + t(rowTitle) + '"data-type="song" data-name="' + encodeURI(obj.result.data[i].Title) + 
            '" data-uri="' + encodeURI(obj.result.data[i].uri) + '">';
        for (let c = 0; c < settings.colsBrowseDatabaseDetail.length; c++) {
            titleList += '<td data-col="' + settings.colsBrowseDatabaseDetail[c] + '">' + 
                e(obj.result.data[i][settings.colsBrowseDatabaseDetail[c]]) + '</td>';
        }
        titleList += '<td data-col="Action"><a href="#" class="mi color-darkgrey">' + ligatureMore + '</a></td></tr>';
        lastDisc = obj.result.data[i].Disc;
    }
    tbody.innerHTML = titleList;
    const tfoot = table.getElementsByTagName('tfoot')[0];
    let colspan = settings.colsBrowseDatabaseDetail.length;
    tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + 
        t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + 
        beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
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
    _addAlbum(action, albumArtist, album);
}

function _addAlbum(action, albumArtist, album) {
    const expression = '((Album == \'' + escapeMPD(album) + '\') AND (' + tagAlbumArtist + ' == \'' + escapeMPD(albumArtist) + '\'))';
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
    const expression = createSearchExpression(document.getElementById('searchDatabaseCrumb'), app.current.filter, getSelectValue('searchDatabaseMatch'), x);
    appGoto(app.current.app, app.current.tab, app.current.view, 
        '0', app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression);
}
