"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function hidePopover(el) {
    const menuEl = document.querySelector('[aria-describedby]');
    if (el === menuEl) {
        //do not hide popover that should be opened
        return;
    }
    if (menuEl) {
        menuEl.Popover.hide();
    }
    else {
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
    hidePopover(event.target);
    //popover is shown
    if (event.target.getAttribute('aria-describedby') !== null ||
        event.target.classList.contains('not-clickable'))
    {
        return;
    }
    //check for existing popover instance
    let popoverInit = event.target.Popover;
    if (popoverInit === undefined) {
        //create it if no popover instance is found
        if (event.target.parentNode.nodeName === 'TH') {
            popoverInit = createPopoverTh(event.target);
        }
        else if (event.target.getAttribute('data-popover') === 'disc') {
            popoverInit = createPopoverDisc(event.target);
        }
        else {
            popoverInit = createPopoverTd(event.target);
        }
    }
    //show the popover
    popoverInit.show();
}

function createPopoverTh(el) {
    const template = elCreateNodes('div', {"class": ["popover"]}, [
        elCreateEmpty('div', {"class": ["popover-arrow"]}),
        elCreateEmpty('h3', {"class": ["popover-header"]}),
        elCreateEmpty('div', {"class": ["popover-body"]})
    ]);

    const popoverInit = new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false,
        title: document.createTextNode(tn('Columns')), template: template, content: document.createTextNode('dummy')});
    
    el.addEventListener('show.bs.popover', function() {
        const menu = elCreateEmpty('form', {});
        setColsChecklist(app.id, menu);
        menu.appendChild(elCreateText('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, tn('Apply')));
        menu.addEventListener('click', function(eventClick) {
            if (eventClick.target.nodeName === 'BUTTON' && eventClick.target.classList.contains('mi')) {
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
        elClear(popoverBody);
        popoverBody.appendChild(menu);
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

    const template = elCreateNodes('div', {"class": ["popover"]}, [
        elCreateEmpty('div', {"class": ["popover-arrow"]}),
        elCreateEmpty('h3', {"class": ["popover-header"]}),
        elCreateEmpty('div', {"class": ["popover-body"]})
    ]);

    const popoverInit = new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false,
        title: document.createTextNode(tn('Disc') + ' ' + disc), template: template, content: document.createTextNode('dummy')});
    
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = popoverInit.popover.getElementsByClassName('popover-body')[0];
        popoverBody.classList.add('px-0');
        elClear(popoverBody);
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album, disc]}, 'Append to queue');
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album, disc]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album, disc]}, 'Add to playlist');
        }
        popoverBody.addEventListener('click', function(eventClick) {
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
    }, false);
    return popoverInit;
}

function createPopoverTd(el) {
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

    const popoverInit = new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false,
        content: document.createTextNode('dummy'), template: template});
    
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
        const created = createMenuTd(el, tabHeader[i], tabPanes[i], i);
        if (created === true) {
            tabPanes[i].addEventListener('click', function(eventClick) {
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

function createMenuTd(el, tabHeader, tabContent, tabNr) {
    if (app.current.card === 'Home' && tabNr === 0) {
        return createMenuHome(el, tabHeader, tabContent);
    }
    
    if (tabNr === 0) {
        return createMenuGeneric(el, tabHeader, tabContent);
    }
    
    return createMenuSecondary(el, tabHeader, tabContent);
}

function addMenuItemsAlbumActions(tabContent, albumArtist, album) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["insertQueue", albumArtist, album]}, 'Insert after current playing song');
        }
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album]}, 'Replace queue');
    }
    if (features.featPlaylists === true) {
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album]}, 'Add to playlist');
    }
    tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
    addMenuItem(tabContent, {"cmd": "gotoAlbum", "options": [albumArtist, album]}, 'Album details');
    addMenuItem(tabContent, {"cmd": "gotoAlbumList", "options": [tagAlbumArtist, albumArtist]}, 'Show all albums from artist');
}

function addMenuItemsSongActions(tabContent, uri) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["song", uri]}, 'Append to queue');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "insertQueue", "options": ["song", uri, 0, 1]}, 'Insert after current playing song');
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
}

function addMenuItemsDirectoryActions(tabContent, baseuri) {
    //songs must be arragend in one album per folder
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["dir", baseuri]}, 'Append to queue');
    if (features.featWhence === true) {
        addMenuItem(div, {"cmd": "insertQueue", "options": ["dir", baseuri, 0, 1]}, 'Insert after current playing song');
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
}

function addMenuItemsPlaylistActions(tabContent, type, uri, name) {
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri]}, 'Append to queue');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertQueue", "options": [type, uri, 0, 1]}, 'Add after current playing song');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri]}, 'Replace queue');
    tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
    if (features.featHome === true) {
        addMenuItem(tabContent, {"cmd": "addPlistToHome", "options": [uri, name]}, 'Add to homescreen');
    }
}

function createMenuGeneric(el, tabHeader, tabContent) {
    let type = getData(el, 'data-type');
    let uri = getData(el, 'data-uri');
    let name = getData(el, 'data-name');
    let dataNode = el;
    let depth = 0;
    while (type === null || type === undefined || uri === null || uri === undefined) {
        dataNode = dataNode.parentNode;
        type = getData(dataNode, 'data-type');
        uri = getData(dataNode, 'data-uri');
        name = getData(dataNode, 'data-name');
        if (depth < 2) { depth++; } else { break; }
    }

    let pType = type;
    if (type === 'song') {
        pType = isStreamUri(uri) === false ? 'Song' : 'Stream';
    }
    else if (type === 'dir') { pType = 'Directory'; }
    else if (type === 'album') { pType = 'Album'; }
    else if (type === 'smartpls') { pType = 'Smart playlist'; }
    else if (type === 'plist') { pType = 'Playlist'; }

    tabHeader.textContent = tn(pType);

    if (app.id === 'BrowseFilesystem' || 
        app.id === 'Search' ||
        app.id === 'BrowseDatabaseDetail')
    {
        if (type === 'song') {
            addMenuItemsSongActions(tabContent, uri, name);
        }
        else if (type === 'dir') {
            addMenuItemsDirectoryActions(tabContent, uri, name);
        }
        else if (type === 'plist' || type === 'smartpls') {
            addMenuItemsPlaylistActions(tabContent, type, uri, name);
        }
        else {
            return false;
        }
        return true;
    }
    
    if (app.id === 'BrowseDatabaseList') {
        const albumArtist = getData(dataNode, 'data-albumartist');
        const album = getData(dataNode, 'data-album');
        addMenuItemsAlbumActions(tabContent, albumArtist, album);
        return true;
    }
    
    if (app.id === 'BrowsePlaylistsList') {
        const smartplsOnly = getData(dataNode, 'data-smartpls-only');
        if (smartplsOnly === false || type !== 'smartpls') {
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
        if (settings.smartpls === true && type === 'smartpls') {
            addMenuItem(tabContent, {"cmd": "showSmartPlaylist", "options": [uri]}, 'Edit smart playlist');
            addMenuItem(tabContent, {"cmd": "updateSmartPlaylist", "options": [uri]}, 'Update smart playlist');
        }
        addMenuItem(tabContent, {"cmd": "showDelPlaylist", "options": [uri, smartplsOnly]}, 'Delete playlist');
        return true;
    }
    
    if (app.current.card === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        const x = document.getElementById('BrowsePlaylistsDetailList');
        const songpos = getData(dataNode, 'data-songpos');
        addMenuItemsSongActions(tabContent, uri, name);
        tabContent.appendChild(elCreateEmpty('div', {"class": ["dropdown-divider"]}));
        if (getData(x, 'data-ro') === 'false') {
            const plist = getData(x, 'data-uri');
            addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["single", plist, songpos]}, 'Remove');
            if (features.featPlaylistRmRange === true) {
                addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["range", plist, 0, songpos]}, 'Remove all upwards');
                addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": ["range", plist, songpos + 1, -1]}, 'Remove all downwards');
            }
        }
        return true;
    }
    
    if (app.current.card === 'Queue' && app.current.tab === 'Current') {
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
        addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", 0, songpos]}, 'Remove all upwards');
        addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", songpos + 1, -1]}, 'Remove all downwards');
        return true;
    }
    
    if (app.current.card === 'Queue' && app.current.tab === 'LastPlayed') {
        addMenuItemsSongActions(tabContent, uri, name);
        return true;
    }
    
    if (app.current.card === 'Queue' && app.current.tab === 'Jukebox') {
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
    return false;
}

function createMenuSecondary(el, tabHeader, tabContent) {
    if (app.current.card === 'Search' ||
        app.id === 'QueueCurrent' ||
        app.id === 'QueueLastPlayed' ||
        (app.id === 'QueueJukebox' && settings.jukeboxMode === 1) ||
        app.id === 'BrowseFileSystem' ||
        app.id === 'BrowsePlaylistsDetail')
    {
        let type = getData(el, 'data-type');
        let uri = getData(el, 'data-uri');
        let name = getData(el, 'data-name');
        let dataNode = el;
        let depth = 0;
        while (type === null || type === undefined || uri === null || uri === undefined) {
            dataNode = dataNode.parentNode;
            type = getData(dataNode, 'data-type');
            uri = getData(dataNode, 'data-uri');
            name = getData(dataNode, 'data-name');
            if (depth < 2) { depth++; } else { break; }
        }

        if (isStreamUri(uri) === true) {
            return false;
        }
        const album = getData(dataNode, 'data-album');
        const albumArtist = getData(dataNode, 'data-albumartist');
        if (album !== null && albumArtist !== null &&
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
    return false;
}

function createMenuHome(el, tabHeader, tabContent) {
    const pos = getData(el.parentNode, 'data-pos');
    const href = getData(el.parentNode, 'data-href');
    if (href === null || href === undefined) {
        return;
    }
    let type = '';
    let actionDesc = '';
    let name = '';
    if (href.cmd === 'replaceQueue' && href.options[0] === 'plist') {
        type = 'plist';
        actionDesc = 'Replace queue with playlist';
        name = 'Playlist';
    }
    else if (href.cmd === 'appGoto') {
        type = 'view';
        actionDesc = 'Goto view';
        name = 'View';
    }
    else if (href.cmd === 'execScriptFromOptions') {
        type = 'script';
        actionDesc = 'Execute script';
        name = 'Script';
    }
    tabHeader.textContent = tn(name);
    addMenuItem(tabContent, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
    if (type === 'plist' && isMPDplaylist(href.options[1]) === true) {
        addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [href.options[1]]}, 'View playlist');
    }
    addMenuItem(tabContent, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
    addMenuItem(tabContent, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
    addMenuItem(tabContent, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
    return true;
}
