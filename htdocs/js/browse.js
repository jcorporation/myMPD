"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
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
                    getCustomDomProperty(event.target.parentNode, 'data-album'),
                    getCustomDomProperty(event.target.parentNode, 'data-albumartist'));
            }
            else if (event.target.classList.contains('card-footer')){
                popoverMenuAlbumCards(event);
            }
        }
        else {
            app.current.search = '';
            document.getElementById('searchDatabaseStr').value = '';
            appGoto(app.current.app, app.current.card, undefined, 0, undefined, 'Album', 'AlbumArtist', 'Album', 
                '((' + app.current.tag + ' == \'' + escapeMPD(getCustomDomProperty(event.target.parentNode, 'data-tag')) + '\'))');
        }
    }, false);
    
    document.getElementById('BrowseDatabaseListList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') || event.target.parentNode.classList.contains('not-clickable')) {
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
                        if (oldEls[i] !== undefined) {
                            oldEls[i].remove();
                        }
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
                elClear(event.target);
            }
        }, false);
    }
    
    document.getElementById('BrowseDatabaseDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            clickSong(getCustomDomProperty(event.target.parentNode, 'data-uri'), getCustomDomProperty(event.target.parentNode, 'data-name'));
        }
        else if (event.target.nodeName === 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('searchDatabaseTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getCustomDomProperty(event.target, 'data-tag');
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
            app.current.sort = getCustomDomProperty(event.target, 'data-tag');
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
            playlistSort(getCustomDomProperty(event.target, 'data-tag'));
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
                const op = getSelectValueId('searchDatabaseMatch');
                const crumbEl = document.getElementById('searchDatabaseCrumb');
                crumbEl.appendChild(createSearchCrumb(app.current.filter, op, this.value));
                elShow(crumbEl);
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
            selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', getCustomDomProperty(event.target,'data-filter-tag'));
            document.getElementById('searchDatabaseStr').value = unescapeMPD(getCustomDomProperty(event.target, 'data-filter-value'));
            document.getElementById('searchDatabaseMatch').value = getCustomDomProperty(event.target, 'data-filter-op');
            event.target.remove();
            app.current.filter = getCustomDomProperty(event.target,'data-filter-tag');
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
            if (document.getElementById('searchDatabaseCrumb').childElementCount === 0) {
                elHideId('searchDatabaseCrumb');
            }
        }
    }, false);

    document.getElementById('BrowseFilesystemList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            const uri = getCustomDomProperty(event.target.parentNode, 'data-uri');
            const name = getCustomDomProperty(event.target.parentNode, 'data-name');
            const dataType = getCustomDomProperty(event.target.parentNode, 'data-type');
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
            const uri = getCustomDomProperty(event.target, 'data-uri');
            const offset = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].offset : 0;
            const scrollPos = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].scrollPos : 0;
            appGoto('Browse', 'Filesystem', undefined, offset, app.current.limit, app.current.filter, app.current.sort, '-', uri, scrollPos);
        }
    }, false);
}

function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = getCustomDomProperty(event.target, 'data-tag');
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
    let tag = getCustomDomProperty(x, 'data-tag');
    let name = getCustomDomProperty(x, 'data-name');
    if (tag === null) {
        tag = getCustomDomProperty(x.parentNode, 'data-tag');
        name = getCustomDomProperty(x.parentNode, 'data-name');
    }
    if (tag !== '' && name !== '' && name !== '-' && settings.tagListBrowse.includes(tag)) {
        if (tag === 'Album') {
            let artist = getCustomDomProperty(x, 'data-albumartist');
            if (artist === null) {
                artist = getCustomDomProperty(x.parentNode, 'data-albumartist');
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
    elClear(imageList);

    if (checkResult(obj, 'BrowseFilesystem', null) === false) {
        elHide(imageList);
        return;
    }

    if ((obj.result.images.length === 0 && obj.result.bookletPath === '')) {
        elHide(imageList);
    }
    else {
        elShow(imageList);
    }
    if (obj.result.bookletPath !== '') {
        const img = document.createElement('div');
        img.style.backgroundImage = 'url("' + subdir + '/assets/coverimage-booklet.svg")';
        img.classList.add('booklet');
        setCustomDomProperty(img, 'data-href', subdir + '/browse/music/' + obj.result.bookletPath);
        img.title = t('Booklet');
        imageList.appendChild(img);
    }
    for (let i = 0, j = obj.result.images.length; i < j; i++) {
        const img = document.createElement('div');
        img.style.backgroundImage = 'url("' + subdir + '/browse/music/' + myEncodeURI(obj.result.images[i]) + '"),url("assets/coverimage-loading.svg")';
        imageList.appendChild(img);
    }

    const rowTitleSong = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];
    const rowTitleFolder = webuiSettingsDefault.clickFolder.validValues[settings.webuiSettings.clickFolder];
    const rowTitlePlaylist = webuiSettingsDefault.clickPlaylist.validValues[settings.webuiSettings.clickPlaylist];
    
    updateTable(obj, 'BrowseFilesystem', function(row, data) {
        setCustomDomProperty(row, 'data-type', data.Type);
        setCustomDomProperty(row, 'data-uri', data.uri);
        //set Title to name if not defined - for folders and playlists
        if (data.Title === undefined) {
            data.Title = data.name;
        }
        setCustomDomProperty(row, 'data-name', data.Title);
        row.setAttribute('title', t(data.Type === 'song' ? rowTitleSong : 
                data.Type === 'dir' ? rowTitleFolder : rowTitlePlaylist));
    });
    scrollToPosY(app.current.scrollPos);
}

//eslint-disable-next-line no-unused-vars
function addAllFromBrowseFilesystem(replace) {
    if (replace === true) {
        sendAPI("MYMPD_API_QUEUE_REPLACE_URI", {
            "uri": app.current.search
        });
        showNotification(tn('Replaced queue'), '', 'queue', 'info');
    }
    else {
        sendAPI("MYMPD_API_QUEUE_ADD_URI", {
            "uri": app.current.search
        });
        showNotification(tn('Added all songs'), '', 'queue', 'info');
    }
}

function addAllFromBrowseDatabasePlist(plist, callback) {
    if (app.current.search.length >= 2) {
        sendAPI("MYMPD_API_DATABASE_SEARCH", {
            "plist": plist,
            "filter": app.current.view,
            "searchstr": app.current.search,
            "offset": 0,
            "limit": 0,
            "cols": settings.colsSearch,
            "replace": false
        }, callback, true);
    }
}

function parseDatabase(obj) {
    const cardContainer = document.getElementById('BrowseDatabaseListList');
    setScrollViewHeight(cardContainer);
    const cols = cardContainer.getElementsByClassName('col');
    document.getElementById('BrowseDatabaseListList').classList.remove('opacity05');

    if (obj.error !== undefined) {
        elClear(cardContainer);
        const div = elCreate('div', {"class": ["ml-3", "mb-3", "not-clickable", "alert", "alert-danger"]}, '');
        addIconLine(div, 'error_outline', tn(obj.error.message, obj.error.data));
        cardContainer.appendChild(div);
        setPagination(0, 0);
        return;
    }

    const nrItems = obj.result.returnedEntities;
    if (nrItems === 0) {
        elClear(cardContainer);
        const div = elCreate('div', {"class": ["ml-3", "mb-3", "not-clickable"]}, '');
        addIconLine(div, 'info', tn('Empty list'));
        cardContainer.appendChild(div);
        setPagination(0, 0);
        return;
    }

    if (cols.length === 0) {
        elClear(cardContainer);
    }
    for (let i = 0; i < nrItems; i++) {
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = tn('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = tn('Unknown album');
        }
        const id = obj.result.tag === 'Album' ? genId('database' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist)
                                              : genId('database' + obj.result.data[i].value);

        if (cols[i] !== undefined && cols[i].firstChild.firstChild.getAttribute('id') === id) {
            continue;
        }

        let picture = '';
        const col = elCreate('div', {"class": ["col", "px-0", "flex-grow-0"]}, '');
        const card = elCreate('div', {"class": ["card", "card-grid", "clickable"], "tabindex": 0}, '');
        if (obj.result.tag === 'Album') {
            picture = subdir + '/albumart/' + obj.result.data[i].FirstSongUri;
        
            const cardBody = elCreate('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id}, '');
            const cardFooter = elCreate('div', {"class": ["card-footer", "card-footer-grid", "p-2"], 
                "title": obj.result.data[i].AlbumArtist + ': ' + obj.result.data[i].Album}, obj.result.data[i].Album);
            cardFooter.appendChild(elCreate('br', {}, ''));
            cardFooter.appendChild(elCreate('small', {}, obj.result.data[i].AlbumArtist));
            card.appendChild(cardBody);
            card.appendChild(cardFooter);
            setCustomDomProperty(card, 'data-picture', picture);
            setCustomDomProperty(card, 'data-uri', obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, ''));
            setCustomDomProperty(card, 'data-type', 'dir');
            setCustomDomProperty(card, 'data-name', obj.result.data[i].Album);
            setCustomDomProperty(card, 'data-album', obj.result.data[i].Album);
            setCustomDomProperty(card, 'data-albumartist', obj.result.data[i].AlbumArtist);
            if (isMobile === true) {
                addPlayButton(cardBody);
            }
        }
        else {
            picture = subdir + '/tagart/' + obj.result.tag + '/' + obj.result.data[i].value;

            if (obj.result.pics === true) {
                const cardBody = elCreate('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id}, '');
                card.appendChild(cardBody);
            }
            
            const cardFooter = elCreate('div', {"class": ["card-footer", "card-footer-grid", "p-2"], 
                "title": obj.result.data[i].value}, obj.result.data[i].value);
            card.appendChild(cardFooter);
            setCustomDomProperty(card, 'data-picture', picture);
            setCustomDomProperty(card, 'data-tag', obj.result.data[i].value);
        }
        col.appendChild(card);
        i < cols.length ? cols[i].replaceWith(col) : cardContainer.append(col);

        if (hasIO === true) {
            const options = {
                root: null,
                rootMargin: '0px',
            };
            const observer = new IntersectionObserver(setGridImage, options);
            observer.observe(col);
        }
        else {
            col.firstChild.firstChild.style.backgroundImage = myEncodeURI(picture);
        }
    }
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }
    
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);    
}

function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            //use URI encoded attribute
            const uri = getCustomDomProperty(change.target.firstChild, 'data-picture');
            const body = change.target.firstChild.getElementsByClassName('card-body')[0];
            if (body) {
                body.style.backgroundImage = 'url("' + myEncodeURI(uri) + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
            }
        }
    });
}

function addPlayButton(parentEl) {
    const div = document.createElement('div');
    div.classList.add('align-self-end', 'album-grid-mouseover', 'mi', 'rounded-circle', 'clickable');
    div.textContent = 'play_arrow';
    div.title = t(webuiSettingsDefault.clickAlbumPlay.validValues[settings.webuiSettings.clickAlbumPlay]);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickAlbumPlay(getCustomDomProperty(event.target.parentNode.parentNode, 'data-albumartist'), getCustomDomProperty(event.target.parentNode.parentNode, 'data-album'));
    }, false);
}

function parseAlbumDetails(obj) {
    const table = document.getElementById('BrowseDatabaseDetailList');
    const tfoot = table.getElementsByTagName('tfoot')[0];
    const colspan = settings.colsBrowseDatabaseDetail.length;
    const infoEl = document.getElementById('viewDetailDatabaseInfo');

    if (checkResult(obj, null, 3) === false) {
        elClear(infoEl);
        elClear(tfoot);
        return;
    }

    const coverEl = document.getElementById('viewDetailDatabaseCover');
    coverEl.style.backgroundImage = 'url("' + subdir + '/albumart/' + myEncodeURI(obj.result.data[0].uri) + '"), url("' + subdir + '/assets/coverimage-loading.svg")';
    setCustomDomProperty(coverEl, 'data-images', obj.result.images);
    setCustomDomProperty(coverEl, 'data-uri', obj.result.data[0].uri);
    
    elClear(infoEl);
    infoEl.appendChild(elCreate('h1', {}, obj.result.Album));
    infoEl.appendChild(elCreate('small', {}, tn('AlbumArtist')));
    const p = elCreate('p', {}, '');
    
    if (settings.tagListBrowse.includes(tagAlbumArtist)) {
        const artistLink = elCreate('a', {"href": "#", "class": ["text-light"]}, obj.result.AlbumArtist);
        setCustomDomProperty(artistLink, 'data-tag', tagAlbumArtist);
        setCustomDomProperty(artistLink, 'data-name', obj.result.AlbumArtist);
        artistLink.addEventListener('click', function(event) {
            event.preventDefault();
            gotoBrowse(event);
        }, false);
        p.appendChild(artistLink);
    }
    else {
        p.textContent = obj.result.AlbumArtist;
    }
    infoEl.appendChild(p);
    if (obj.result.bookletPath !== '' && features.featLibrary === true) {
        const booklet = elCreate('p', {}, '');
        booklet.appendChild(elCreate('span', {"class": ["text-light", "mi", "me-2"]}, 'description'));
        booklet.appendChild(elCreate('a', {"class": ["text-light"], "target": "_blank", "href": subdir + '/browse/music/' + 
            myEncodeURI(obj.result.bookletPath)}, tn('Download booklet')));
        infoEl.appendChild(booklet);
    }
    
    const rowTitle = tn(webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong]);
    updateTable(obj, 'BrowseDatabaseDetail', function(row, data) {
        setCustomDomProperty(row, 'data-type', 'song');
        setCustomDomProperty(row, 'data-name', data.Title);
        setCustomDomProperty(row, 'data-uri', data.uri);
        row.setAttribute('title', rowTitle);
    });

    const tr = elCreate('tr', {}, '');
    const td = elCreate('td', {"colspan": colspan + 1}, '');
    const small = elCreate('small', {}, '');
    small.appendChild(elCreate('span', {}, tn('Num songs', obj.result.totalEntities)));
    small.appendChild(elCreate('span', {}, ' – '));
    small.appendChild(elCreate('span', {}, beautifyDuration(obj.result.totalTime)));
    td.appendChild(small);
    tr.appendChild(td);
    elClear(tfoot);
    tfoot.appendChild(tr);
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
    const expression = createSearchExpression(document.getElementById('searchDatabaseCrumb'), app.current.filter, getSelectValueId('searchDatabaseMatch'), x);
    appGoto(app.current.app, app.current.tab, app.current.view, 
        '0', app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}
