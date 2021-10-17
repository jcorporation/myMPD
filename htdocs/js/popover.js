"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function hidePopover() {
    const menuEl = document.querySelector('[aria-describedby]');
    if (menuEl) {
        menuEl.Popover.hide();
    }
}

function showPopover(event) {
    event.preventDefault();
    event.stopPropagation();
    if (event.target.getAttribute('aria-describedby') !== null) {
        return;
    }
    hidePopover();
    let popoverInit = event.target.Popover;
    if (popoverInit === undefined) {
        if (event.target.parentNode.nodeName === 'TH') {
            popoverInit = createPopoverTh(event.target);
        }
        else {
            popoverInit = createPopoverTd(event.target);
        }
    }
    popoverInit.show();
}

function createPopoverTh(el) {
    const popoverInit = new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false, title: tn('Columns'), content: 'content'});
    el.addEventListener('show.bs.popover', function() {
        const menu = elCreate('form', {}, '');
        setColsChecklist(app.id, menu);
        menu.appendChild(elCreate('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, tn('Apply')));
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
    return popoverInit;
}

function createPopoverTd(el) {
    const popoverInit = new BSN.Popover(el, {trigger: 'click', delay: 0, dismissible: false, content: 'content',
        template: '<div class="popover" role="tooltip">' +
            '<div class="popover-arrow"></div>' +
            '<h3 class="popover-header"></h3>' +
            '<div class="popover-tabs py-2">' +
            '<ul class="nav nav-tabs px-2">' +
            '<li class="nav-item"><a class="nav-link active" href="#"></a></li>' +
            '<li class="nav-item"><a class="nav-link" href="#"></a></li>' +
            '</ul>' +
            '<div class="tab-content">' +
            '<div class="tab-pane fade active show" id="popoverTab0">...</div>' +
            '<div class="tab-pane fade" id="popoverTab1">...</div>' +
            '</div>' +
            '</div></div>'});
    
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
        createMenuTd(el, tabHeader[i], tabPanes[i], i);
        if (tabHeader.textContent !== '') {
            tabPanes[i].addEventListener('click', function(eventClick) {
                const target = eventClick.target.nodeName === 'SPAN' ? eventClick.target.parentNode : eventClick.target;
                if (target.nodeName === 'A') {
                    const cmd = getCustomDomProperty(eventClick.target, 'data-href');
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
            elHide(tabHeader[i].parentNode);
        }
    }
    return popoverInit;
}

function addMenuItem(tabContent, cmd, text) {
    const a = elCreate('a', {"class": ["dropdown-item"], "href": "#"}, tn(text));
    setCustomDomProperty(a, 'data-href', cmd);
    tabContent.appendChild(a);
}

function createMenuTd(el, tabHeader, tabContent, tabNr) {
    if (app.current.app === 'Home' && tabNr === 0) {
        createMenuHome(el, tabHeader, tabContent);
    }
    else if (tabNr === 0) {
        createMenuGeneric(el, tabHeader, tabContent);
    }
    else {
        createMenuSecondary(el, tabHeader, tabContent);
    }
}

function createMenuGeneric(el, tabHeader, tabContent) {
    let type = getCustomDomProperty(el, 'data-type');
    let uri = getCustomDomProperty(el, 'data-uri');
    let name = getCustomDomProperty(el, 'data-name');
    let nextSongPos = 0;
    let dataNode = el;
    let depth = 0;
    while (type === null || type === undefined || uri === null || uri === undefined) {
        dataNode = dataNode.parentNode;
        type = getCustomDomProperty(dataNode, 'data-type');
        uri = getCustomDomProperty(dataNode, 'data-uri');
        name = getCustomDomProperty(dataNode, 'data-name');
        if (depth < 2) { depth++; } else { break; }
    }
    
    if (lastState) {
        nextSongPos = lastState.nextSongPos;
    }

    let pType = type;
    if (type === 'song') { pType = 'Song'; }
    else if (type === 'dir') {
        if (app.current.tab === 'Database') { pType = 'Album'; }
        else { pType = 'Directory'; }
    }
    else if (type === 'smartpls') { pType = 'Smart playlist'; }
    else if (type === 'plist') { pType = 'Playlist'; }

    tabHeader.textContent = tn(pType);

    if ((app.current.app === 'Browse' && app.current.tab === 'Filesystem') || app.current.app === 'Search' ||
        (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail'))
    {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        if (type === 'song') {
            addMenuItem(tabContent, {"cmd": "appendAfterQueue", "options": [type, uri, nextSongPos, name]}, 'Add after current playing song');
        }    
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (type !== 'plist' && type !== 'smartpls' && features.featPlaylists === true) {
            addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (type === 'song') {
            addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
        if (type === 'plist' || type === 'smartpls') {
            addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
        }
        if ((type === 'plist' || type === 'smartpls') && features.featHome === true) {
            addMenuItem(tabContent, {"cmd": "addPlistToHome", "options": [uri, name]}, 'Add to homescreen');
        }
        if (app.current.tab === 'Filesystem' && type === 'dir') {
            addMenuItem(tabContent, {"cmd": "updateDB", "options": [dirname(uri), true]}, 'Update directory');
            addMenuItem(tabContent, {"cmd": "rescanDB", "options": [dirname(uri), true]}, 'Rescan directory');
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        const albumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        const album = getCustomDomProperty(dataNode, 'data-album');
        addMenuItem(tabContent, {"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, undefined, "Album", tagAlbumArtist, album, albumArtist]}, 'Show album');
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album]}, 'Append to queue');
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album]}, 'Add to playlist');
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
        const smartplsOnly = getCustomDomProperty(dataNode, 'data-smartpls-only');
        if (smartplsOnly === false || type !== 'smartpls') {
            addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
            addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
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
        if (features.featHome === true) {
            addMenuItem(tabContent, {"cmd": "addPlistToHome", "options": [uri, name]}, 'Add to homescreen');
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        const x = document.getElementById('BrowsePlaylistsDetailList');
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (getCustomDomProperty(x, 'data-ro') === 'false') {
            addMenuItem(tabContent, {"cmd": "removeFromPlaylist", "options": [getCustomDomProperty(x, 'data-uri'), 
                    getCustomDomProperty(el.parentNode.parentNode, 'data-songpos')]}, 'Remove');
        }
        if (features.featPlaylists === true) {
            addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (isStreamUri(uri) === false) {
            addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        const trackid = getCustomDomProperty(dataNode, 'data-trackid');
        const songpos = getCustomDomProperty(dataNode, 'data-songpos');
        if (isStreamUri(uri) === false) {
            addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
        if (trackid !== lastState.currentSongId) {
            addMenuItem(tabContent, {"cmd": "playAfterCurrent", "options": [trackid, songpos]}, 'Play after current playing song');
        }
        addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["single", trackid]}, 'Remove');
        addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", 0, songpos]}, 'Remove all upwards');
        addMenuItem(tabContent, {"cmd": "delQueueSong", "options": ["range", (songpos - 1), -1]}, 'Remove all downwards');
        if (features.featPlaylists === true) {
            addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        addMenuItem(tabContent, {"cmd": "showSetSongPriority", "options": [trackid]}, 'Set priority');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (isStreamUri(uri) === false) {
            addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        const pos = Number(getCustomDomProperty(dataNode, 'data-pos'));
        const vAlbum = getCustomDomProperty(dataNode, 'data-album');
        const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        if (settings.jukeboxMode === 1) {
            addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
        addMenuItem(tabContent, {"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, 50, "Album", tagAlbumArtist, vAlbum, vAlbumArtist]}, 'Show album');
        addMenuItem(tabContent, {"cmd": "delQueueJukeboxSong", "options": [pos]}, 'Remove');
    }
}

function createMenuSecondary(el, tabHeader, tabContent) {
    let type = getCustomDomProperty(el, 'data-type');
    let uri = getCustomDomProperty(el, 'data-uri');
    let name = getCustomDomProperty(el, 'data-name');
    let nextSongPos = 0;
    let dataNode = el;
    let depth = 0;
    while (type === null || type === undefined || uri === null || uri === undefined) {
        dataNode = dataNode.parentNode;
        type = getCustomDomProperty(dataNode, 'data-type');
        uri = getCustomDomProperty(dataNode, 'data-uri');
        name = getCustomDomProperty(dataNode, 'data-name');
        if (depth < 2) { depth++; } else { break; }
    }
    
    if (app.current.app === 'Search') {
        //album actions
        const vAlbum = getCustomDomProperty(dataNode, 'data-album');
        const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        if (vAlbum !== undefined && vAlbumArtist !== undefined) {
            tabHeader.textContent = tn('Album');
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendQueue", vAlbumArtist, vAlbum]}, 'Append to queue');
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replaceQueue", vAlbumArtist, vAlbum]}, 'Replace queue');
            if (features.featPlaylists === true) {
                addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["addPlaylist", vAlbumArtist, vAlbum]}, 'Add to playlist');
            }
        }
        else {
            tabHeader.textContent = tn('Directory');
            const baseuri = dirname(uri);
            //songs must be arragend in one album per folder
            addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, baseuri, name]}, 'Append to queue');
            //add after playing song works only for single songs
            //addMenuItem(div, {"cmd": "appendAfterQueue", "options": [type, baseuri, nextSongPos, name]}, 'Add after current playing song');
            addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, baseuri, name]}, 'Replace queue');
            if (features.featPlaylists === true) {
                addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, 'Add to playlist');
            }
        }
    }
}

function createMenuHome(el, tabHeader, tabContent) {
    const pos = getCustomDomProperty(el.parentNode, 'data-pos');
    const href = getCustomDomProperty(el.parentNode, 'data-href');
    if (href === null || href === undefined) {
        return;
    }
    let type = '';
    let actionDesc = '';
    let name = '';
    if (href.cmd === 'replaceQueue' && href.options[0] === 'plist') {
        type = 'plist';
        actionDesc = 'Add and play playlist';
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
    if (type === 'plist') {
        addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [href.options[1]]}, 'View playlist');
    }
    addMenuItem(tabContent, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
    addMenuItem(tabContent, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
    addMenuItem(tabContent, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
}
