"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initBrowse() {
    document.getElementById('BrowseDatabaseListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
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
        if (event.target.classList.contains('row')) {
            return;
        }
        if (app.current.tag === 'Album') {
            popoverMenuAlbumCards(event);
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        if (app.current.tag === 'Album') {
            popoverMenuAlbumCards(event);
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('keydown', function(event) {
        navigateGrid(event.target, event.key);
    }, false);
    
    if (isMobile === false) {
        document.getElementById('BrowseDatabaseListList').addEventListener('mouseover', function(event) {
            if (app.current.tag !== 'Album') {
                return;
            }
            if (event.target.classList.contains('card-body') && event.target.childNodes.length === 0) {
                const oldEls = document.getElementById('BrowseDatabaseListList').getElementsByClassName('album-grid-mouseover');
                const oldElsLen = oldEls.length;
                if (oldElsLen > 1) {
                    for (let i = 0; i < oldElsLen; i++) {
                        oldEls[i].remove();
                    }
                }
                addPlayButton(event.target);
            }
        }, false);

        document.getElementById('BrowseDatabaseListList').addEventListener('mouseout', function(event) {
            if (app.current.tag !== 'Album') {
                return;
            }
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
                case 'parentDir': {
                    const offset = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].offset : 0;
                    const scrollPos = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].scrollPos : 0;
                    app.current.filter = '-';
                    appGoto('Browse', 'Filesystem', undefined, offset, app.current.limit, app.current.filter, app.current.sort, '-', uri, scrollPos);
                    break;
                }
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

    document.getElementById('BrowseBreadcrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            const uri = getAttDec(event.target, 'data-uri');
            const offset = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].offset : 0;
            const scrollPos = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].scrollPos : 0;
            appGoto('Browse', 'Filesystem', undefined, offset, app.current.limit, app.current.filter, app.current.sort, '-', uri, scrollPos);
        }
    }, false);
}

function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = getAttDec(event.target, 'data-tag');
        if (tag === 'Playlists' || tag === 'Filesystem') {
            appGoto('Browse', tag, undefined);
            return;
        }
        
        if (app.current.app === 'Browse' && app.current.tab !== 'Database') {
            appGoto('Browse', 'Database', app.apps.Browse.tabs.Database.active);
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
    if (event.target.classList.contains('row')) {
        return;
    }
    showMenu(event.target, event);
    const selCards = document.getElementById('BrowseDatabaseListList').getElementsByClassName('selected');
    for (let i = 0, j = selCards.length; i < j; i++) {
        selCards[i].classList.remove('selected');
    }
    event.target.parentNode.classList.add('selected');
    event.preventDefault();
    event.stopPropagation();
}

function gotoBrowse(event) {
    if (features.featAdvsearch === false) {
        return;
    }
    const x = event.target;
    let tag = getAttDec(x, 'data-tag');
    let name = getAttDec(x, 'data-name');
    if (tag === null) {
        tag = getAttDec(x.parentNode, 'data-tag');
        name = getAttDec(x.parentNode, 'data-name');
    }
    if (tag !== '' && name !== '' && name !== '-' && settings.tagListBrowse.includes(tag)) {
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
    //show images in folder
    const imageList = document.getElementById('BrowseFilesystemImages');
    imageList.innerHTML = '';
    if ((obj.result.images.length === 0 && obj.result.bookletPath === '') || settings.publish === false) {
        imageList.classList.add('hide');
    }
    else {
        imageList.classList.remove('hide');
    }
    if (obj.result.bookletPath !== '' && settings.publish === true) {
        const img = document.createElement('div');
        img.style.backgroundImage = 'url("' + subdir + '/assets/coverimage-booklet.svg")';
        img.classList.add('booklet');
        setAttEnc(img, 'data-href', subdir + '/browse/music/' + obj.result.bookletPath);
        img.title = t('Booklet');
        imageList.appendChild(img);
    }
    for (let i = 0, j = obj.result.images.length; i < j; i++) {
        const img = document.createElement('div');
        img.style.backgroundImage = 'url("' + subdir + '/browse/music/' + obj.result.images[i] + '"),url("assets/coverimage-loading.svg")';
        imageList.appendChild(img);
    }

    const rowTitleSong = advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong];
    const rowTitleFolder = advancedSettingsDefault.clickFolder.validValues[settings.advanced.clickFolder];
    const rowTitlePlaylist = advancedSettingsDefault.clickPlaylist.validValues[settings.advanced.clickPlaylist];
    
    updateTable(obj, 'BrowseFilesystem', function(row, data) {
        setAttEnc(row, 'data-type', data.Type);
        setAttEnc(row, 'data-uri', data.uri);
        //set Title to name if not defined - for folders and playlists
        if (data.Title === undefined) {
            data.Title = data.name;
        }
        setAttEnc(row, 'data-name', data.Title);
        row.setAttribute('title', t(data.Type === 'song' ? rowTitleSong : 
                data.Type === 'dir' ? rowTitleFolder : rowTitlePlaylist));
    });
    scrollToPosY(app.current.scrollPos);
}

//eslint-disable-next-line no-unused-vars
function addAllFromBrowseFilesystem(replace) {
    if (replace === true) {
        sendAPI("MYMPD_API_QUEUE_REPLACE_TRACK", {"uri": app.current.search});
        showNotification(t('Replaced queue'), '', 'queue', 'info');
    }
    else {
        sendAPI("MYMPD_API_QUEUE_ADD_TRACK", {"uri": app.current.search});
        showNotification(t('Added all songs'), '', 'queue', 'info');
    }
}

function addAllFromBrowseDatabasePlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI("MYMPD_API_DATABASE_SEARCH", {"plist": plist, "filter": app.current.view, "searchstr": app.current.search, "offset": 0, "limit": 0, "cols": settings.colsSearch, "replace": false});
    }
}

function parseDatabase(obj) {
    const nrItems = obj.result.returnedEntities;
    const cardContainer = document.getElementById('BrowseDatabaseListList');
    const cols = cardContainer.getElementsByClassName('col');
    const hasIO = 'IntersectionObserver' in window ? true : false;

    document.getElementById('BrowseDatabaseListList').classList.remove('opacity05');

    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        const col = document.createElement('div');
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
            picture = subdir + '/tagart/' + obj.result.tag + '/' + encodeURI(obj.result.data[i].value);
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
            if (hasIO === true) {
                const options = {
                    root: null,
                    rootMargin: '0px',
                };
                const observer = new IntersectionObserver(setGridImage, options);
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
    for (let i = cols.length -1; i >= nrItems; i --) {
        cols[i].remove();
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div class="ml-3 mb-3 not-clickable"><span class="mi">info</span>&nbsp;&nbsp;' + t('Empty list') + '</div>';
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
        (obj.result.bookletPath === '' || features.featLibrary === false ? '' : 
            '<span class="text-light mi">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>') +
        '</p>';

    const rowTitle = t(advancedSettingsDefault.clickSong.validValues[settings.advanced.clickSong]);
    updateTable(obj, 'BrowseDatabaseDetail', function(row, data) {
        setAttEnc(row, 'data-type', 'song');
        setAttEnc(row, 'data-name', data.Title);
        setAttEnc(row, 'data-uri', data.uri);
        row.setAttribute('title', rowTitle);
    });

    const table = document.getElementById('BrowseDatabaseDetailList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    const colspan = settings.colsBrowseDatabaseDetail.length;
    tfoot.innerHTML = '<tr><td colspan="' + (colspan + 1) + '"><small>' + 
        t('Num songs', obj.result.totalEntities) + '&nbsp;&ndash;&nbsp;' + 
        beautifyDuration(obj.result.totalTime) + '</small></td></tr>';
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
