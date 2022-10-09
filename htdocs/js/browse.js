"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Initialization function for the browse view
 */
function initBrowse() {
    document.getElementById('BrowseDatabaseListList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        if (app.current.tag === 'Album') {
            const target = getParent(event.target, 'DIV');
            if (target.classList.contains('card-body')) {
                appGoto('Browse', 'Database', 'Detail', 0, undefined, 'Album', 'AlbumArtist',
                    getData(target.parentNode, 'Album'),
                    getData(target.parentNode, 'AlbumArtist')
                );
            }
            else if (target.classList.contains('card-footer')){
                showPopover(event);
            }
        }
        else {
            app.current.search = '';
            document.getElementById('searchDatabaseStr').value = '';
            appGoto(app.current.card, app.current.tab, undefined, 0, undefined, 'Album', 'AlbumArtist', 'Album',
                '((' + app.current.tag + ' == \'' + escapeMPD(getData(event.target.parentNode, 'tag')) + '\'))');
        }
    }, false);

    document.getElementById('BrowseDatabaseListList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        if (app.current.tag === 'Album') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseDatabaseListList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        if (app.current.tag === 'Album') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseDatabaseDetailList').addEventListener('click', function(event) {
        if (event.target.parentNode.parentNode.nodeName === 'TFOOT') {
            return;
        }
        if (event.target.nodeName === 'TD') {
            if (event.target.parentNode.classList.contains('not-clickable')) {
                return;
            }
            clickSong(getData(event.target.parentNode, 'uri'));
        }
        else if (event.target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);

    document.getElementById('searchDatabaseTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            app.current.filter = getData(event.target, 'tag');
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
        }
    }, false);

    document.getElementById('databaseSortDesc').addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        toggleBtnChk(this, undefined);
        app.current.sort.desc = app.current.sort.desc === true ? false : true;
        appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
    }, false);

    document.getElementById('databaseSortTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            event.stopPropagation();
            app.current.sort.tag = getData(event.target, 'tag');
            appGoto(app.current.card, app.current.tab, app.current.view, 0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search);
        }
    }, false);

    for (const nav of ['BrowseDatabaseByTagDropdown', 'BrowseNavPlaylistsDropdown', 'BrowseNavFilesystemDropdown',
        'BrowseNavWebradiodbDropdown','BrowseNavRadiobrowserDropdown', 'BrowseNavRadioFavoritesDropdown'])
    {
        document.getElementById(nav).addEventListener('click', function(event) {
            navBrowseHandler(event);
        }, false);
    }

    document.getElementById('searchFilesystemStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            const value = this.value;
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, (value !== '' ? value : '-'), app.current.sort, '-', app.current.search);
            }, searchTimerTimeout);
        }
    }, false);

    document.getElementById('searchDatabaseStr').addEventListener('keyup', function(event) {
        clearSearchTimer();
        const value = this.value;
        if (event.key === 'Escape') {
            clearSearchTimer();
            this.blur();
        }
        else if (event.key === 'Enter' &&
            app.current.tag === 'Album')
        {
            if (value !== '') {
                const op = getSelectValueId('searchDatabaseMatch');
                const crumbEl = document.getElementById('searchDatabaseCrumb');
                crumbEl.appendChild(createSearchCrumb(app.current.filter, op, value));
                elShow(crumbEl);
                this.value = '';
            }
            else {
                searchTimer = setTimeout(function() {
                    searchAlbumgrid(value);
                }, searchTimerTimeout);
            }
        }
        else if (app.current.tag === 'Album') {
            searchTimer = setTimeout(function() {
                searchAlbumgrid(value);
            }, searchTimerTimeout);
        }
        else {
            searchTimer = setTimeout(function() {
                appGoto(app.current.card, app.current.tab, app.current.view,
                    0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, value);
            }, searchTimerTimeout);
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
            selectTag('searchDatabaseTags', 'searchDatabaseTagsDesc', getData(event.target,'filter-tag'));
            document.getElementById('searchDatabaseStr').value = unescapeMPD(getData(event.target, 'filter-value'));
            document.getElementById('searchDatabaseMatch').value = getData(event.target, 'filter-op');
            event.target.remove();
            app.current.filter = getData(event.target,'filter-tag');
            searchAlbumgrid(document.getElementById('searchDatabaseStr').value);
            if (document.getElementById('searchDatabaseCrumb').childElementCount === 0) {
                elHideId('searchDatabaseCrumb');
            }
        }
    }, false);

    document.getElementById('BrowseFilesystemList').addEventListener('click', function(event) {
        let target;
        switch(event.target.nodeName) {
             case 'TD':    target = event.target.parentNode; break;
             case 'DIV':   target = event.target.parentNode; break;
             case 'SPAN':
             case 'SMALL': target = event.target.parentNode.parentNode.parentNode; break;
             default:      target = event.target;
        }
        if (target.nodeName === 'TR') {
            const uri = getData(target, 'uri');
            const dataType = getData(target, 'type');
            switch(dataType) {
                case 'parentDir': {
                    const offset = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].offset : 0;
                    const scrollPos = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].scrollPos : 0;
                    app.current.filter = '-';
                    appGoto('Browse', 'Filesystem', undefined, offset, app.current.limit, app.current.filter, app.current.sort, 'dir', uri, scrollPos);
                    break;
                }
                case 'dir':
                    clickFolder(uri);
                    break;
                case 'song':
                    clickSong(uri);
                    break;
                case 'plist':
                    clickFilesystemPlaylist(uri);
                    break;
            }
        }
        else if (target.nodeName === 'A') {
            //action td
            handleActionTdClick(event);
        }
    }, false);

    document.getElementById('BrowseBreadcrumb').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            event.preventDefault();
            const uri = getData(event.target, 'uri');
            const offset = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].offset : 0;
            const scrollPos = browseFilesystemHistory[uri] !== undefined ? browseFilesystemHistory[uri].scrollPos : 0;
            appGoto('Browse', 'Filesystem', undefined, offset, app.current.limit, app.current.filter, app.current.sort, 'dir', uri, scrollPos);
        }
    }, false);
}

/**
 * Event handler for the navigation dropdown in the browse views
 * @param {event} event triggering event
 */
function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = getData(event.target, 'tag');
        if (tag === 'Playlists' ||
            tag === 'Filesystem' ||
            tag === 'Radio')
        {
            appGoto('Browse', tag, undefined);
            return;
        }

        if (app.current.card === 'Browse' &&
            app.current.tab !== 'Database')
        {
            appGoto('Browse', 'Database', app.cards.Browse.tabs.Database.active);
            return;
        }
        if (tag !== 'Album') {
            app.current.filter = tag;
            app.current.sort.tag = tag;
            app.current.sort.desc = false;
        }
        else {
            app.current.sort = {
                "tag": tagAlbumArtist,
                "desc": false
            };
        }
        app.current.search = '';
        document.getElementById('searchDatabaseMatch').value = 'contains';
        appGoto(app.current.card, app.current.tab, app.current.view,
            0, app.current.limit, app.current.filter, app.current.sort, tag, app.current.search);
    }
}

/**
 * Event handler for links to browse views
 * @param {event} event triggering event
 */
function gotoBrowse(event) {
    let target = event.target;
    let tag = getData(target, 'tag');
    let name = getData(target, 'name');
    let i = 0;
    while (tag === undefined) {
        i++;
        target = target.parentNode;
        tag = getData(target, 'tag');
        name = getData(target, 'name');
        if (i > 2) {
            break;
        }
    }
    if (tag !== '' &&
        name !== '' &&
        name !== '-' &&
        settings.tagListBrowse.includes(tag))
    {
        if (tag === 'Album') {
            let artist = getData(target, 'AlbumArtist');
            if (artist === undefined) {
                artist = getData(target.parentNode, 'AlbumArtist');
            }
            if (artist !== null) {
                //Show album details
                appGoto('Browse', 'Database', 'Detail', 0, undefined, tag, tagAlbumArtist, name, artist);
            }
            else {
                //show filtered album list
                gotoAlbumList(tag, name);
            }
        }
        else {
            //show filtered album list
            gotoAlbumList(tag, name);
        }
    }
}

/**
 * Go's to the album detail view
 * @param {Array} artist albumartist names
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function gotoAlbum(artist, album) {
    appGoto('Browse', 'Database', 'Detail', 0, undefined, 'Album', tagAlbumArtist, album, artist);
}

/**
 * Go's to a filtered album list
 * @param {String} tag tag to search
 * @param {Array} value array of values to match
 */
//eslint-disable-next-line no-unused-vars
function gotoAlbumList(tag, value) {
    if (typeof value === 'string') {
        //convert string to array
        value = [value];
    }
    document.getElementById('searchDatabaseStr').value = '';
    let expression = '(';
    for (let i = 0, j = value.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND '
        }
        expression += '(' + tag + ' == \'' + escapeMPD(value[i]) + '\')';
    }
    expression += ')';
    appGoto('Browse', 'Database', 'List', 0, undefined, tag, {"tag": tagAlbumArtist, "desc": false}, 'Album', expression);
}

/**
 * Go's to the filesystem view
 * @param {String} uri uri to list
 * @param {*} type "dir" or "plist"
 */
//eslint-disable-next-line no-unused-vars
function gotoFilesystem(uri, type) {
    document.getElementById('searchFilesystemStr').value = '';
    appGoto('Browse', 'Filesystem', undefined, 0, undefined, '-', '-', type, uri);
}

/**
 * Parses the MYMPD_API_DATABASE_FILESYSTEM_LIST response
 * @param {Object} obj jsonrpc response object
 */
function parseFilesystem(obj) {
    //show images in folder
    const imageList = document.getElementById('BrowseFilesystemImages');
    elClear(imageList);

    const table = document.getElementById('BrowseFilesystemList');
    const tfoot = table.querySelector('tfoot');
    elClear(tfoot);

    if (checkResultId(obj, 'BrowseFilesystemList') === false) {
        elHide(imageList);
        return;
    }

    if (obj.result.images !== undefined) {
        if (obj.result.images.length === 0 &&
            obj.result.bookletPath === '')
        {
            elHide(imageList);
        }
        else {
            elShow(imageList);
        }
        if (obj.result.bookletPath !== '') {
            const img = elCreateEmpty('div', {"class": ["booklet"], "title": tn('Booklet')});
            img.style.backgroundImage = 'url("' + subdir + '/assets/coverimage-booklet.svg")';
            setData(img, 'href', subdir + myEncodeURI(obj.result.bookletPath));
            imageList.appendChild(img);
        }
        for (let i = 0, j = obj.result.images.length; i < j; i++) {
            if (isThumbnailfile(obj.result.images[i]) === true) {
                continue;
            }
            const img = elCreateEmpty('div', {});
            img.style.backgroundImage = 'url("' + subdir + myEncodeURI(obj.result.images[i]) + '"),' +
                'url("assets/coverimage-loading.svg")';
            imageList.appendChild(img);
        }
    }
    else {
        //playlist response
        elHide(imageList);
        obj.result.totalEntities++;
        obj.result.returnedEntities++;
        const parentUri = dirname(obj.result.plist);
        obj.result.data.unshift({"Type": "parentDir", "name": "parentDir", "uri": parentUri});
    }

    const rowTitleSong = webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong];
    const rowTitleFolder = 'Open directory';
    const rowTitlePlaylist = webuiSettingsDefault.clickFilesystemPlaylist.validValues[settings.webuiSettings.clickFilesystemPlaylist];

    updateTable(obj, 'BrowseFilesystem', function(row, data) {
        setData(row, 'type', data.Type);
        setData(row, 'uri', data.uri);
        //set Title to name if not defined - for folders and playlists
        setData(row, 'name', data.Title === undefined ? data.name : data.Title);
        row.setAttribute('title', tn(data.Type === 'song' ? rowTitleSong :
            data.Type === 'dir' ? rowTitleFolder : rowTitlePlaylist));
    });

    const colspan = settings.colsBrowseFilesystem.length + 1;
    tfoot.appendChild(
        elCreateNode('tr', {},
            elCreateTextTnNr('td', {"colspan": colspan}, 'Num entries', obj.result.totalEntities)
        )
    );
}

/**
 * Parsed the MYMPD_API_DATABASE_ALBUMS_GET and MYMPD_API_DATABASE_TAG_LIST response
 * @param {Object} obj jsonrpc response object
 */
function parseDatabase(obj) {
    const cardContainer = document.getElementById('BrowseDatabaseListList');
    unsetUpdateView(cardContainer);
    const cols = cardContainer.querySelectorAll('.col');

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        setPagination(0, 0);
        return;
    }

    const nrItems = obj.result.returnedEntities;
    if (nrItems === 0) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, 'Empty list')
        );
        setPagination(0, 0);
        return;
    }

    if (cardContainer.querySelector('.not-clickable') !== null) {
        elClear(cardContainer);
    }
    for (let i = 0; i < nrItems; i++) {
        //id is used only to check if card should be refreshed
        const id = obj.result.tag === 'Album' ? genId('database' + obj.result.data[i].Album + obj.result.data[i].AlbumArtist)
                                              : genId('database' + obj.result.data[i].value);

        if (cols[i] !== undefined && cols[i].firstChild.firstChild.getAttribute('id') === id) {
            continue;
        }

        let image = '';
        const card = elCreateEmpty('div', {"data-popover": "album", "class": ["card", "card-grid", "clickable"]});
        if (obj.result.tag === 'Album') {
            image = subdir + '/albumart-thumb?offset=0&uri=' + myEncodeURIComponent(obj.result.data[i].FirstSongUri);
            card.appendChild(
                elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id})
            );
            card.appendChild(
                elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "p-2"],
                    "title": obj.result.data[i][tagAlbumArtist] + ': ' + obj.result.data[i].Album}, [
                        printValue('Album', obj.result.data[i].Album),
                        elCreateEmpty('br', {}),
                        elCreateNode('small', {}, printValue("AlbumArtist", obj.result.data[i].AlbumArtist))
                ])
            );
            setData(card, 'image', image);
            setData(card, 'uri', obj.result.data[i].FirstSongUri.replace(/\/[^/]+$/, ''));
            setData(card, 'type', 'album');
            setData(card, 'name', obj.result.data[i].Album);
            setData(card, 'Album', obj.result.data[i].Album);
            setData(card, 'AlbumArtist', obj.result.data[i].AlbumArtist);
            addAlbumPlayButton(card.firstChild);
        }
        else {
            image = subdir + '/tagart?uri=' + obj.result.tag + '/' + obj.result.data[i].value;
            if (obj.result.pics === true) {
                card.appendChild(
                    elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id})
                );
            }
            card.appendChild(
                elCreateText('div', {"class": ["card-footer", "card-footer-grid", "p-2"],
                    "title": obj.result.data[i].value}, obj.result.data[i].value)
            );
            setData(card, 'image', image);
            setData(card, 'tag', obj.result.data[i].value);
        }
        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }

        if (userAgentData.hasIO === true) {
            const observer = new IntersectionObserver(setGridImage, {root: null, rootMargin: '0px'});
            observer.observe(col);
        }
        else {
            col.firstChild.firstChild.style.backgroundImage = myEncodeURI(image);
        }
    }
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
    scrollToPosY(cardContainer.parentNode, app.current.scrollPos);
}

/**
 * Callback function for intersection observer to lazy load cover images
 * @param {Object} changes IntersectionObserverEntry objects
 * @param {Object} observer IntersectionObserver
 */
function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = getData(change.target.firstChild, 'image');
            const body = change.target.firstChild.querySelector('.card-body');
            if (body) {
                body.style.backgroundImage = 'url("' + uri + '"),' +
                    'url("' + subdir + '/assets/coverimage-loading.svg")';
            }
        }
    });
}

/**
 * Adds the album play button
 * @param {HTMLElement | ChildNode} parentEl parent element for the button
 */
function addAlbumPlayButton(parentEl) {
    const div = pEl.coverPlayBtn.cloneNode(true);
    parentEl.appendChild(div);
    div.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        clickAlbumPlay(getData(event.target.parentNode.parentNode, 'AlbumArtist'), getData(event.target.parentNode.parentNode, 'Album'));
    }, false);
}

/**
 * Parses the MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST response
 * @param {Object} obj jsonrpc response object
 */
function parseAlbumDetails(obj) {
    const table = document.getElementById('BrowseDatabaseDetailList');
    const tfoot = table.querySelector('tfoot');
    const colspan = settings.colsBrowseDatabaseDetail.length;
    const infoEl = document.getElementById('viewDetailDatabaseInfo');

    if (checkResultId(obj, 'BrowseDatabaseDetailList') === false) {
        elClear(infoEl);
        elClear(tfoot);
        return;
    }

    const coverEl = document.getElementById('viewDetailDatabaseCover');
    coverEl.style.backgroundImage = 'url("' + subdir + '/albumart?offset=0&uri=' + myEncodeURIComponent(obj.result.data[0].uri) + '"),' +
        'url("' + subdir + '/assets/coverimage-loading.svg")';
    setData(coverEl, 'images', obj.result.images);
    setData(coverEl, 'embeddedImageCount', obj.result.embeddedImageCount);
    setData(coverEl, 'uri', obj.result.data[0].uri);

    elClear(infoEl);
    infoEl.appendChild(
        elCreateText('h1', {}, obj.result.Album)
    );
    for (const tag of [tagAlbumArtist, 'Genre']) {
        if (settings.tagList.includes(tag)) {
            const p = elCreateEmpty('p', {});
            infoEl.appendChild(
                elCreateTextTn('small', {}, tag)
            );
            printBrowseLink(p, tag, obj.result[tag]);
            infoEl.appendChild(p);
        }
    }

    if (obj.result.bookletPath !== '' &&
        features.featLibrary === true)
    {
        infoEl.appendChild(
            elCreateNodes('p', {}, [
                elCreateText('span', {"class": ["mi", "me-2"]}, 'description'),
                elCreateTextTn('a', {"target": "_blank", "href": subdir + myEncodeURI(obj.result.bookletPath)}, 'Download booklet')
            ])
        );
    }

    if (obj.result.MusicBrainzAlbumId !== '-' ||
        checkTagValue(obj.result.MusicBrainzAlbumArtistId, '-') === false)
    {
        infoEl.appendChild(
            elCreateTextTn('small', {}, 'MusicBrainz')
        );
        if (obj.result.MusicBrainzAlbumId !== '-') {
            const albumLink = getMBtagLink('MUSICBRAINZ_ALBUMID', obj.result.MusicBrainzAlbumId);
            albumLink.textContent = tn('Goto album');
            infoEl.appendChild(
                elCreateNode('p', {"class": ["mb-1"]}, albumLink)
            );
        }
        if (checkTagValue(obj.result.MusicBrainzAlbumArtistId, '-') === false) {
            for (let i = 0, j = obj.result.MusicBrainzAlbumArtistId.length; i < j; i++) {
                const artistLink = getMBtagLink('MUSICBRAINZ_ALBUMARTISTID', obj.result.MusicBrainzAlbumArtistId[i]);
                artistLink.textContent = tn('Goto artist') + ': ' + obj.result.AlbumArtist[i];
                infoEl.appendChild(
                    elCreateNode('p', {"class": ["mb-1"]}, artistLink)
                );
            }
        }
    }

    const rowTitle = tn(webuiSettingsDefault.clickSong.validValues[settings.webuiSettings.clickSong]);
    updateTable(obj, 'BrowseDatabaseDetail', function(row, data) {
        setData(row, 'type', 'song');
        setData(row, 'name', data.Title);
        setData(row, 'uri', data.uri);
        row.setAttribute('title', rowTitle);
    });

    elReplaceChild(tfoot,
        elCreateNode('tr', {},
            elCreateNode('td', {"colspan": colspan + 1},
                elCreateText('small', {},
                    tn('Num songs', obj.result.totalEntities) +
                        smallSpace + nDash + smallSpace + fmtDuration(obj.result.totalTime)
                )
            )
        )
    );
}

/**
 * Go's to the last browse database grid view
 */
//eslint-disable-next-line no-unused-vars
function backToAlbumGrid() {
    appGoto('Browse', 'Database', 'List');
}

/**
 * Wrapper for _addAlbum for add buttons in the album detail view
 * @param {String} action action to perform
 */
//eslint-disable-next-line no-unused-vars
function addAlbum(action) {
    // @ts-ignore search in this view an array
    _addAlbum(action, app.current.search, app.current.tag, undefined);
}

/**
 * Appends an album to the queue.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function appendQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('appendQueue', albumArtist, album, undefined);
}

/**
 * Appends an album to the queue and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function appendPlayQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('appendPlayQueue', albumArtist, album, undefined);
}

/**
 * Replaces the queue with an album.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function replaceQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('replaceQueue', albumArtist, album, undefined);
}

/**
 * Replaces the queue with an album and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function replacePlayQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('replacePlayQueue', albumArtist, album, undefined);
}

/**
 * Inserts the an album after the current playing song.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function insertAfterCurrentQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('insertQueue', albumArtist, album, undefined);
}

/**
 * Inserts the an album after the current playing song and plays it.
 * Wrapper for _addAlbum for home icon action.
 * @param {*} type not used
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 */
//eslint-disable-next-line no-unused-vars
function insertPlayAfterCurrentQueueAlbum(type, albumArtist, album) {
    //type not used but required for home icon cmd
    _addAlbum('insertPlayQueue', albumArtist, album, undefined);
}

/**
 * Adds an album to the queue or a playlist
 * @param {String} action action to perform
 * @param {Array} albumArtist array of albumartists
 * @param {String} album album name
 * @param {String} disc optional disc number, "undefined" to add the whole album
 */
function _addAlbum(action, albumArtist, album, disc) {
    let expression = '((Album == \'' + escapeMPD(album) + '\')';
    for (const artist of albumArtist) {
        expression += ' AND (' + tagAlbumArtist + ' == \'' + escapeMPD(artist) + '\')';
    }
    if (disc !== undefined) {
        expression += ' AND (Disc == \'' + escapeMPD(disc) + '\')';
    }
    expression += ')';

    switch(action) {
        case 'appendQueue':
            appendQueue('search', expression);
            break;
        case 'appendPlayQueue':
            appendPlayQueue('search', expression);
            break;
        case 'replaceQueue':
            replaceQueue('search', expression);
            break;
        case 'replacePlayQueue':
            replacePlayQueue('search', expression);
            break;
        case 'insertAfterCurrentQueue':
            insertAfterCurrentQueue('search', expression);
            break;
        case 'insertPlayAfterCurrentQueue':
            insertPlayAfterCurrentQueue('search', expression);
            break;
        case 'addPlaylist':
            showAddToPlaylist('ALBUM', expression);
            break;
    }
}

/**
 * Creates and executes the mpd filter expression from the search crumbs and current search values
 * for the album grid search.
 * @param {*} searchStr string to search
 */
function searchAlbumgrid(searchStr) {
    const expression = createSearchExpression(document.getElementById('searchDatabaseCrumb'), app.current.filter, getSelectValueId('searchDatabaseMatch'), searchStr);
    appGoto(app.current.card, app.current.tab, app.current.view,
        0, app.current.limit, app.current.filter, app.current.sort, app.current.tag, expression, 0);
}
