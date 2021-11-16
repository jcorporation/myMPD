"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function hidePopover(thisEl) {
    const menuEls = document.querySelectorAll('[aria-describedby]');
    for (const el of menuEls) {
        if (thisEl === el) {
            //do not hide popover that should be opened
            continue;
        }
        el.Popover.hide();
    }
    if (menuEls.length === 0) {
        //handle popover dom nodes without a trigger element
        const popover = document.getElementsByClassName('popover')[0];
        if (popover) {
            //simply remove the popover dom node
            popover.remove();
        }
    }
}

function showPopover(event) {
    event.preventDefault();
    event.stopPropagation();
    //get the dom node to attach the popover object
    let target = event.target.nodeName === 'SPAN' ? event.target.parentNode : event.target;
    if (target.nodeName === 'TD') {
        //try to attach popover instance to action link in tables
        const actionLink = target.parentNode.lastElementChild.firstElementChild;
        if (actionLink !== null && actionLink.nodeName === 'A') {
            target = actionLink;
        }
    }
    else if (target.parentNode.classList.contains('card')) {
        //attach popover instance to card
        target = target.parentNode;
    }
    hidePopover(target);
    //popover is shown
    if (target.getAttribute('aria-describedby') !== null ||
        target.classList.contains('not-clickable'))
    {
        return;
    }
    //check for existing popover instance
    let popoverInit = target.Popover;
    //create it if no popover instance is found
    if (popoverInit === undefined) {
        const popoverType = target.getAttribute('data-popover');
        switch (popoverType) {
            case 'columns':
                //column select in table header
                popoverInit = createPopoverColumns(target);
                break;
            case 'disc':
                //disc actions in album details view
                popoverInit = createPopoverDisc(target);
                break;
            case 'queue':
            case 'database':
                //navbar icons
                popoverInit = createPopoverNavbarIcon(target, popoverType);
                break;
            case 'home':
                //home card actions
                popoverInit = createPopoverHome(target);
                break;
            case 'album':
                //album action in album list
                popoverInit = createPopoverAlbumView(target);
                break;
            default:
                popoverInit = createPopoverLists(target);
        }
    }
    //show the popover
    popoverInit.show();
}

function createPopoverInit(el, title, template) {
    if (template === undefined) {
        template = elCreateNodes('div', {"class": ["popover"]}, [
            elCreateEmpty('div', {"class": ["popover-arrow"]}),
            elCreateEmpty('h3', {"class": ["popover-header"]}),
            elCreateEmpty('div', {"class": ["popover-body"]})
        ]);
    }
    return new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false,
        title: document.createTextNode(title), template: template, content: document.createTextNode('dummy')});
}

function createPopoverClickHandler(el) {
    el.addEventListener('click', function(eventClick) {
        if (eventClick.target.nodeName === 'A') {
            const cmd = getData(eventClick.target, 'data-href');
            if (cmd) {
                parseCmd(eventClick, cmd);
                hidePopover();
            }
        }
        eventClick.preventDefault();
        eventClick.stopPropagation();
    }, false);
}

function createPopoverColumns(el) {
    const popoverInit = createPopoverInit(el, tn('Columns'));

    el.addEventListener('show.bs.popover', function() {
        const menu = elCreateEmpty('form', {});
        setColsChecklist(app.id, menu);
        menu.appendChild(elCreateText('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, tn('Apply')));
        menu.addEventListener('click', function(eventClick) {
            if (eventClick.target.nodeName === 'BUTTON' &&
                eventClick.target.classList.contains('mi'))
            {
                toggleBtnChk(eventClick.target);
                eventClick.preventDefault();
                eventClick.stopPropagation();
            }
            else if (eventClick.target.nodeName === 'BUTTON') {
                eventClick.preventDefault();
                saveCols(app.id);
            }
        }, false);
        const popoverBody = popoverInit.popover.getElementsByClassName('popover-body')[0];
        elReplaceChild(popoverBody,menu);
        popoverBody.setAttribute('id', app.id + 'ColsDropdown');
    }, false);

    el.addEventListener('shown.bs.popover', function(event) {
        //resize popover-body to prevent screen overflow
        const popoverId = event.target.getAttribute('aria-describedby');
        const popover = document.getElementById(popoverId);
        const offsetTop = popover.offsetTop;
        if (offsetTop < 0) {
            const b = popover.getElementsByClassName('popover-body')[0];
            const h = popover.getElementsByClassName('popover-header')[0];
            b.style.maxHeight = (popover.offsetHeight + offsetTop - h.offsetHeight) + 'px';
            popover.style.top = 0;
            b.style.overflow = 'auto';
        }
    }, false);
    return popoverInit;
}

function createPopoverDisc(el) {
    const disc = getData(el.parentNode.parentNode, 'data-disc');
    const album = getData(el.parentNode.parentNode, 'data-album');
    const albumArtist = getData(el.parentNode.parentNode, 'data-albumartist');

    const popoverInit = createPopoverInit(el, tn('Disc'));
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.popover.getElementsByClassName('popover-body')[0].replaceWith(popoverBody);
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album, disc]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["insertQueue", albumArtist, album]}, 'Insert after current playing song');
            addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["playQueue", albumArtist, album]}, 'Add to queue and play');
        }
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album, disc]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album, disc]}, 'Add to playlist');
        }
        createPopoverClickHandler(popoverBody);
    }, false);
    return popoverInit;
}

function createPopoverNavbarIcon(el, type) {
    const popoverInit = createPopoverInit(el, el.getAttribute('title'));
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.popover.getElementsByClassName('popover-body')[0].replaceWith(popoverBody);
        switch(type) {
            case 'queue':
                addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_CLEAR"}]}, 'Clear');
                addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_CROP"}]}, 'Crop');
                addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_SHUFFLE"}]}, 'Shuffle');
                addMenuItem(popoverBody, {"cmd": "showModal", "options": ["modalSaveQueue"]}, 'Save');
                addMenuItem(popoverBody, {"cmd": "showModal", "options": ["modalAddToQueue"]}, 'Add to queue');
                popoverBody.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "Current", undefined]}, 'Show queue');
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "LastPlayed", undefined]}, 'Show last played');
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "Jukebox", undefined]}, 'Show jukebox queue');
                break;
            case 'database':
                addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", true, false]}, 'Update database');
                addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", true, true]}, 'Rescan database');
                popoverBody.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Database", undefined]}, 'Show browse database');
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Playlists", undefined]}, 'Show browse playlists');
                addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Filesystem", undefined]}, 'Show browse filesystem');
                break;
        }
        createPopoverClickHandler(popoverBody);
    }, false);
    return popoverInit;
}

function createPopoverHome(el) {
    const popoverInit = createPopoverInit(el, tn('Home'));
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.popover.getElementsByClassName('popover-body')[0].replaceWith(popoverBody);
        const popoverHeader = popoverInit.popover.getElementsByClassName('popover-header')[0];
        createMenuHome(el, popoverHeader, popoverBody);
        createPopoverClickHandler(popoverBody);
    }, false);
    return popoverInit;
}

function createPopoverAlbumView(el) {
    const popoverInit = createPopoverInit(el, tn('Album'));
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.popover.getElementsByClassName('popover-body')[0].replaceWith(popoverBody);
        const albumArtist = getData(el, 'data-albumartist');
        const album = getData(el, 'data-album');
        addMenuItemsAlbumActions(popoverBody, albumArtist, album);
        createPopoverClickHandler(popoverBody);
    }, false);
    return popoverInit;
}

function createPopoverLists(el) {
    const template = elCreateNodes('div', {"class": ["popover"]}, [
        elCreateEmpty('div', {"class": ["popover-arrow"]}),
        elCreateEmpty('h3', {"class": ["popover-header"]}),
        elCreateNodes('div', {"class": ["popover-tabs", "py-2"]}, [
            elCreateNodes('ul', {"class": ["nav", "nav-tabs", "px-2"]}, [
                elCreateNode('li', {"class": ["nav-item"]},
                    elCreateEmpty('a', {"class": ["nav-link", "active"], "href": "#"})
                ),
                elCreateNode('li', {"class": ["nav-item"]},
                    elCreateEmpty('a', {"class": ["nav-link"], "href": "#"})
                )
            ]),
            elCreateNodes('div', {"class": ["tab-content"]}, [
                elCreateEmpty('div', {"class": ["tab-pane", "pt-2", "active", "show"], "id": "popoverTab0"}),
                elCreateEmpty('div', {"class": ["tab-pane", "pt-2"], "id": "popoverTab1"})
            ])
        ])
    ]);

    const popoverInit = createPopoverInit(el, '', template);

    const tabHeader = popoverInit.popover.getElementsByClassName('nav-link');
    const tabPanes = popoverInit.popover.getElementsByClassName('tab-pane');
    for (let i = 0; i < 2; i++) {
        tabHeader[i].addEventListener('click', function(event) {
            tabHeader[i].classList.add('active');
            tabPanes[i].classList.add('active', 'show');
            const j = i === 0 ? 1 : 0;
            tabHeader[j].classList.remove('active');
            tabPanes[j].classList.remove('active', 'show');
            event.preventDefault();
            event.stopPropagation();
        }, false);

        elClear(tabPanes[i]);
        const created = i === 0 ?
            createMenuLists(el, tabHeader[0], tabPanes[0]) :
            createMenuSecondary(el, tabHeader[1], tabPanes[1]);

        if (created === true) {
            createPopoverClickHandler(tabPanes[i]);
        }
        else {
            popoverInit.popover.getElementsByClassName('popover-header')[0].textContent = tabHeader[0].textContent;
            tabHeader[0].parentNode.parentNode.remove();
        }
    }
    return popoverInit;
}

function addMenuItem(tabContent, cmd, text) {
    const a = elCreateText('a', {"class": ["dropdown-item"], "href": "#"}, tn(text));
    setData(a, 'data-href', cmd);
    tabContent.appendChild(a);
}

function addMenuItemsAlbumActions(tabContent, albumArtist, album) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["insertQueue", albumArtist, album]}, 'Insert after current playing song');
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["playQueue", albumArtist, album]}, 'Add to queue and play');
        }
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album]}, 'Replace queue');
    }
    if (features.featPlaylists === true) {
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album]}, 'Add to playlist');
    }
    tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
    addMenuItem(tabContent, {"cmd": "gotoAlbum", "options": [albumArtist, album]}, 'Album details');
    addMenuItem(tabContent, {"cmd": "gotoAlbumList", "options": [tagAlbumArtist, albumArtist]}, 'Show all albums from artist');
    if (features.featHome === true && app.id !== 'Home') {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "addAlbumToHome", "options": [albumArtist, album]}, 'Add to homescreen');
    }
}

function addMenuItemsSongActions(tabContent, uri, name) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["song", uri]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["song", uri, 0, 1, false]}, 'Insert after current playing song');
            addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["song", uri, 0, 1, true]}, 'Add to queue and play');
        }
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": ["song", uri]}, 'Replace queue');
    }
    if (features.featPlaylists === true) {
        addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
    }
    if (isStreamUri(uri) === false) {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
    }
    if (features.featHome === true && app.id !== 'Home') {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "addSongToHome", "options": [uri, name]}, 'Add to homescreen');
    }
}

function addMenuItemsSearchActions(tabContent, uri) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["search", uri]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["search", uri, 0, 1, false]}, 'Insert after current playing song');
            addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["search", uri, 0, 1, true]}, 'Add to queue and play');
        }
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": ["search", uri]}, 'Replace queue');
    }
}

function addMenuItemsDirectoryActions(tabContent, baseuri) {
    //songs must be arragend in one album per folder
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["dir", baseuri]}, 'Append to queue');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["dir", baseuri, 0, 1, false]}, 'Insert after current playing song');
        addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["song", baseuri, 0, 1, true]}, 'Add to queue and play');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": ["dir", baseuri]}, 'Replace queue');
    if (features.featPlaylists === true) {
        addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, 'Add to playlist');
    }
    if (app.id === 'BrowseFilesystem') {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "updateDB", "options": [baseuri, true]}, 'Update directory');
        addMenuItem(tabContent, {"cmd": "rescanDB", "options": [baseuri, true]}, 'Rescan directory');
    }
    else {
        addMenuItem(tabContent, {"cmd": "gotoFilesystem", "options": [baseuri]}, 'Show directory');
    }
    if (features.featHome === true && app.id !== 'Home') {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "addDirToHome", "options": [baseuri, baseuri]}, 'Add to homescreen');
    }
}

function addMenuItemsPlaylistActions(tabContent, type, uri, name) {
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri]}, 'Append to queue');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertQueue", "options": [type, uri, 0, 1, false]}, 'Add after current playing song');
        addMenuItem(tabContent, {"cmd": "insertQueue", "options": [type, uri, 0, 1, true]}, 'Add to queue and play');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri]}, 'Replace queue');
    if (features.featHome === true && app.id !== 'Home') {
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        addMenuItem(tabContent, {"cmd": "addPlistToHome", "options": [uri, type, name]}, 'Add to homescreen');
    }
}

function createMenuLists(el, tabHeader, tabContent) {
    const dataNode = el.parentNode.parentNode;
    const type = getData(dataNode, 'data-type');
    const uri = getData(dataNode, 'data-uri');
    const name = getData(dataNode, 'data-name');

    let pType = type;
    switch(type) {
        case 'song':
            pType = isStreamUri(uri) === false ? 'Song' : 'Stream';
            break;
        case 'dir':      pType = 'Directory'; break;
        case 'album':    pType = 'Album'; break;
        case 'smartpls': pType = 'Smart playlist'; break;
        case 'plist':    pType = 'Playlist'; break;
    }
    tabHeader.textContent = tn(pType);

    switch(app.id) {
        case 'BrowseFilesystem':
        case 'Search':
        case 'BrowseDatabaseDetail': {
            switch(type) {
                case 'song':
                    addMenuItemsSongActions(tabContent, uri, name);
                    break;
                case 'dir':
                    addMenuItemsDirectoryActions(tabContent, uri);
                    break;
                case 'plist':
                case 'smartpls':
                    addMenuItemsPlaylistActions(tabContent, type, uri, name);
                    break;
                default:
                    return false;
            }
            return true;
        }
        case 'BrowsePlaylistsList': {
            const smartplsOnly = getData(dataNode, 'data-smartpls-only');
            if (smartplsOnly === false ||
                type !== 'smartpls')
            {
                addMenuItemsPlaylistActions(tabContent, type, uri, name);
                tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
                if (settings.smartpls === true && type === 'smartpls') {
                    addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
                }
                else {
                    addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'Edit playlist');
                }
                addMenuItem(tabContent, {"cmd": "showRenamePlaylist", "options": [uri]}, 'Rename playlist');
            }
            if (settings.smartpls === true &&
                type === 'smartpls')
            {
                addMenuItem(tabContent, {"cmd": "showSmartPlaylist", "options": [uri]}, 'Edit smart playlist');
                addMenuItem(tabContent, {"cmd": "updateSmartPlaylist", "options": [uri]}, 'Update smart playlist');
            }
            addMenuItem(tabContent, {"cmd": "showDelPlaylist", "options": [uri, smartplsOnly]}, 'Delete playlist');
            return true;
        }
        case 'BrowsePlaylistsDetail': {
            const table = document.getElementById('BrowsePlaylistsDetailList');
            addMenuItemsSongActions(tabContent, uri, name);
            tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
            if (getData(table, 'data-ro') === 'false') {
                const plist = getData(table, 'data-uri');
                const songpos = getData(dataNode, 'data-songpos');
                const playlistLength = getData(table, 'data-playlistlength');
                addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["single", plist, songpos]}, 'Remove');
                if (features.featPlaylistRmRange === true) {
                    if (songpos > 0) {
                        addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["range", plist, 0, songpos]}, 'Remove all upwards');
                    }
                    if (songpos + 1 < playlistLength) {
                        addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["range", plist, songpos + 1, -1]}, 'Remove all downwards');
                    }
                }
            }
            return true;
        }
        case 'QueueCurrent': {
            const trackid = getData(dataNode, 'data-trackid');
            const songpos = getData(dataNode, 'data-songpos');
            addMenuItemsSongActions(tabContent, uri, name);
            tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
            if (trackid !== currentState.currentSongId) {
                addMenuItem(tabContent, {"cmd": "playAfterCurrent", "options": [trackid, songpos]}, 'Play after current playing song');
            }
            addMenuItem(tabContent, {"cmd": "showSetSongPriority", "options": [trackid]}, 'Set priority');
            tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
            addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["single", trackid]}, 'Remove');
            if (songpos > 0) {
                addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", 0, songpos]}, 'Remove all upwards');
            }
            if (songpos + 1 < currentState.queueLength) {
                addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", songpos + 1, -1]}, 'Remove all downwards');
            }
            return true;
        }
        case 'QueueLastPlayed': {
            addMenuItemsSongActions(tabContent, uri, name);
            return true;
        }
        case 'QueueJukebox': {
            const pos = Number(getData(dataNode, 'data-pos'));
            const vAlbum = getData(dataNode, 'data-album');
            const vAlbumArtist = getData(dataNode, 'data-albumartist');
            if (settings.jukeboxMode === 1) {
                addMenuItemsSongActions(tabContent, uri, name);
            }
            else if (settings.jukeboxMode === 2) {
                addMenuItemsAlbumActions(tabContent, vAlbumArtist, vAlbum)
            }
            tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
            addMenuItem(tabContent, {"cmd": "delQueueJukeboxSong", "options": [pos]}, 'Remove');
            return true;
        }
    }
    return false;
}

function createMenuSecondary(el, tabHeader, tabContent) {
    switch(app.id) {
        case 'Search':
        case 'QueueCurrent':
        case 'QueueLastPlayed':
        case 'QueueJukebox':
        case 'BrowseFilesystem':
        case 'BrowsePlaylistsDetail': {
            const dataNode = el.parentNode.parentNode;
            const type = getData(dataNode, 'data-type');
            const uri = getData(dataNode, 'data-uri');
            const name = getData(dataNode, 'data-name');

            if (isStreamUri(uri) === true ||
                (app.id === 'BrowseFilesystem' && type === 'dir') ||
                (app.id === 'BrowseFilesystem' && type === 'plist') ||
                (app.id === 'BrowseFilesystem' && type === 'smartpls') ||
                (app.id === 'QueueJukebox' && settings.jukeboxMode === 1))
            {
                return false;
            }
            const album = getData(dataNode, 'data-album');
            const albumArtist = getData(dataNode, 'data-albumartist');
            if (album !== undefined &&
                albumArtist !== undefined &&
                album !== '-' && albumArtist !== '-')
            {
                tabHeader.textContent = tn('Album');
                addMenuItemsAlbumActions(tabContent, albumArtist, album);
            }
            else {
                tabHeader.textContent = tn('Directory');
                const baseuri = dirname(uri);
                addMenuItemsDirectoryActions(tabContent, baseuri, name);
            }
            return true;
        }
    }
    return false;
}

function createMenuHome(el, tabHeader, tabContent) {
    const pos = getData(el, 'data-pos');
    const href = getData(el, 'data-href');
    if (href === null || href === undefined) {
        return;
    }
    let type = '';
    let title = '';
    let actionDesc = '';
    switch(href.cmd) {
        case 'appGoto':
            type = 'view';
            title = 'View';
            actionDesc = 'Goto view';
            break;
        case 'execScriptFromOptions':
            type = 'script';
            title = 'Script';
            actionDesc = 'Execute script';
            break;
        case 'replaceQueueAlbum':
        case 'appendQueueAlbum':
        case 'playQueueAlbum':
        case 'insertQueueAlbum':
            type = href.options[0];
            title = typeFriendly[href.options[0]];
            break;
        case 'replaceQueue':
        case 'appendQueue':
        case 'insertAndPlayQueue':
        case 'insertAfterCurrentQueue':
            type = href.options[0];
            title = typeFriendly[href.options[0]];
    }
    tabHeader.textContent = tn(title);
    switch(type) {
        case 'plist':
        case 'smartpls':
            addMenuItemsPlaylistActions(tabContent, type, href.options[1], href.options[1]);
            break;
        case 'dir':
            addMenuItemsDirectoryActions(tabContent, href.options[1]);
            break;
        case 'song':
        case 'stream':
            addMenuItemsSongActions(tabContent, href.options[1], href.options[1]);
            break;
        case 'search':
            addMenuItemsSearchActions(tabContent, href.options[1]);
            break;
        case 'album':
            addMenuItemsAlbumActions(tabContent, href.options[1], href.options[2]);
            break;
        case 'view':
        case 'script':
            addMenuItem(tabContent, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
    }
    tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
    tabContent.appendChild(elCreateText('h2', {"class": ["dropdown-header"]}, tn('Home icon')));
    addMenuItem(tabContent, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
    addMenuItem(tabContent, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
    addMenuItem(tabContent, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
    return true;
}
