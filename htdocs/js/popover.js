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
    const popoverInit = new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: false, title: tn('Columns'), content: 'content'});
    el.addEventListener('show.bs.popover', function(event) {
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
    const popoverInit = new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: false, content: 'content'});
    el.addEventListener('show.bs.popover', function(event) {
        const menu = elCreate('div', {}, '');
        createMenuTd(el, menu);
        menu.addEventListener('click', function(eventClick) {
            const target = eventClick.target.nodeName === 'SPAN' ? eventClick.target.parentNode : eventClick.target;
            if (target.getAttribute('data-bs-toggle') !== null) {
                toggleCollapseArrow(target);
                const collapse = document.querySelector(target.getAttribute('href'));
                if (collapse.classList.contains('show')) {
                    collapse.classList.remove('show');
                }
                else {
                    collapse.classList.add('show');
                }
            }
            else if (target.nodeName === 'A') {
                const cmd = getCustomDomProperty(eventClick.target, 'data-href');
                if (cmd) {
                    parseCmd(eventClick, cmd);
                    hidePopover();
                }
            }
            eventClick.preventDefault();
            eventClick.stopPropagation();
        }, false);
        const popoverBody = popoverInit.popover.getElementsByClassName('popover-body')[0];
        popoverBody.classList.add('px-0', 'py-1');
        elClear(popoverBody);
        popoverBody.appendChild(menu);
    }, false);
    return popoverInit;
}

function addMenuItem(menu, cmd, text) {
    const a = elCreate('a', {"class": ["dropdown-item"], "href": "#"}, tn(text));
    setCustomDomProperty(a, 'data-href', cmd);
    menu.appendChild(a);
}

function createMenuTd(el, menu) {
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

    if ((app.current.app === 'Browse' && app.current.tab === 'Filesystem') || app.current.app === 'Search' ||
        (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail'))
    {
        addMenuItem(menu, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        if (type === 'song') {
            addMenuItem(menu, {"cmd": "appendAfterQueue", "options": [type, uri, nextSongPos, name]}, 'Add after current playing song');
        }    
        addMenuItem(menu, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (type !== 'plist' && type !== 'smartpls' && features.featPlaylists === true) {
            addMenuItem(menu, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (type === 'song') {
            addMenuItem(menu, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
        if (type === 'plist' || type === 'smartpls') {
            addMenuItem(menu, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
        }
        if ((type === 'plist' || type === 'smartpls') && features.featHome === true) {
            addMenuItem(menu, {"cmd": "addPlistToHome", "options": [uri, name]}, 'Add to homescreen');
        }
        if (app.current.tab === 'Filesystem' && type === 'dir') {
            addMenuItem(menu, {"cmd": "updateDB", "options": [dirname(uri), true]}, 'Update directory');
            addMenuItem(menu, {"cmd": "rescanDB", "options": [dirname(uri), true]}, 'Rescan directory');
        }
        if (app.current.app === 'Search') {
            //album actions collapse
            const vAlbum = getCustomDomProperty(dataNode, 'data-album');
            const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
            menu.appendChild(elCreate('div', {"class": ["dropdown-divider"]}, ''));
            const a = (elCreate('a', {"class": ["dropdown-item"], "data-bs-toggle": "collapse", "href": "#popoverAlbumMenu"}, ''));
            a.appendChild(elCreate('span', {"class": ["mi", "mi-left"]}, 'keyboard_arrow_right'));
            a.appendChild(elCreate('span', {}, tn('Album actions')));
            menu.appendChild(a);
            const div = elCreate('div', {"class": ["collapse", "menu-indent"], "id": "popoverAlbumMenu"});
            if (vAlbum !== undefined && vAlbumArtist !== undefined) {
                addMenuItem(div, {"cmd": "_addAlbum", "options": ["appendQueue", vAlbumArtist, vAlbum]}, 'Append to queue');
                addMenuItem(div, {"cmd": "_addAlbum", "options": ["replaceQueue", vAlbumArtist, vAlbum]}, 'Replace queue');
                if (features.featPlaylists === true) {
                    addMenuItem(div, {"cmd": "_addAlbum", "options": ["addPlaylist", vAlbumArtist, vAlbum]}, 'Add to playlist');
                }
            }
            else {
                //songs must be arragend in one album per folder
                addMenuItem(div, {"cmd": "appendQueue", "options": [type, baseuri, name]}, 'Append to queue');
                //add after playing song works only for single songs
                //addMenuItem(div, {"cmd": "appendAfterQueue", "options": [type, baseuri, nextSongPos, name]}, 'Add after current playing song');
                addMenuItem(div, {"cmd": "replaceQueue", "options": [type, baseuri, name]}, 'Replace queue');
                if (features.featPlaylists === true) {
                    addMenuItem(div, {"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, 'Add to playlist');
                }
            }
            menu.appendChild(div);
            //new BSN.Collapse(a);
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        const albumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        const album = getCustomDomProperty(dataNode, 'data-album');
        addMenuItem(menu, {"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, undefined, "Album", tagAlbumArtist, album, albumArtist]}, 'Show album');
        addMenuItem(menu, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album]}, 'Append to queue');
        addMenuItem(menu, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(menu, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album]}, 'Add to playlist');
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
        const smartplsOnly = getCustomDomProperty(dataNode, 'data-smartpls-only');
        if (smartplsOnly === false || type !== 'smartpls') {
            addMenuItem(menu, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
            addMenuItem(menu, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
            if (settings.smartpls === true && type === 'smartpls') {
                addMenuItem(menu, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
            }
            else {
                addMenuItem(menu, {"cmd": "playlistDetails", "options": [uri]}, 'Edit playlist');
            }
            addMenuItem(menu, {"cmd": "showRenamePlaylist", "options": [uri]}, 'Rename playlist');
        }
        if (settings.smartpls === true && type === 'smartpls') {
            addMenuItem(menu, {"cmd": "showSmartPlaylist", "options": [uri]}, 'Edit smart playlist');
            addMenuItem(menu, {"cmd": "updateSmartPlaylist", "options": [uri]}, 'Update smart playlist');
        }
        addMenuItem(menu, {"cmd": "showDelPlaylist", "options": [uri, smartplsOnly]}, 'Delete playlist');
        if (features.featHome === true) {
            addMenuItem(menu, {"cmd": "addPlistToHome", "options": [uri, name]}, 'Add to homescreen');
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        const x = document.getElementById('BrowsePlaylistsDetailList');
        addMenuItem(menu, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        addMenuItem(menu, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (getCustomDomProperty(x, 'data-ro') === 'false') {
            addMenuItem(menu, {"cmd": "removeFromPlaylist", "options": [getCustomDomProperty(x, 'data-uri'), 
                    getCustomDomProperty(el.parentNode.parentNode, 'data-songpos')]}, 'Remove');
        }
        if (features.featPlaylists === true) {
            addMenuItem(menu, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (isStreamUri(uri) === false) {
            addMenuItem(menu, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        const trackid = getCustomDomProperty(dataNode, 'data-trackid');
        const songpos = getCustomDomProperty(dataNode, 'data-songpos');
        if (trackid !== lastState.currentSongId) {
            addMenuItem(menu, {"cmd": "playAfterCurrent", "options": [trackid, songpos]}, 'Play after current playing song');
        }
        addMenuItem(menu, {"cmd": "delQueueSong", "options": ["single", trackid]}, 'Remove');
        addMenuItem(menu, {"cmd": "delQueueSong", "options": ["range", 0, songpos]}, 'Remove all upwards');
        addMenuItem(menu, {"cmd": "delQueueSong", "options": ["range", (songpos - 1), -1]}, 'Remove all downwards');
        if (features.featPlaylists === true) {
            addMenuItem(menu, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (isStreamUri(uri) === false) {
            addMenuItem(menu, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        addMenuItem(menu, {"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue');
        addMenuItem(menu, {"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue');
        if (features.featPlaylists === true) {
            addMenuItem(menu, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
        }
        if (isStreamUri(uri) === false) {
            addMenuItem(menu, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        const pos = Number(getCustomDomProperty(dataNode, 'data-pos'));
        const vAlbum = getCustomDomProperty(dataNode, 'data-album');
        const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        if (settings.jukeboxMode === 1) {
            addMenuItem(menu, {"cmd": "songDetails", "options": [uri]}, 'Song details');
        }
        addMenuItem(menu, {"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, 50, "Album", tagAlbumArtist, vAlbum, vAlbumArtist]}, 'Show album');
        addMenuItem(menu, {"cmd": "delQueueJukeboxSong", "options": [pos]}, 'Remove');
    }
    else if (app.current.app === 'Home') {
        const pos = getCustomDomProperty(el.parentNode, 'data-pos');
        const href = getCustomDomProperty(el.parentNode, 'data-href');
        if (href === null || href === undefined) {
            return;
        }
        let actionDesc = '';
        if (href.cmd === 'replaceQueue' && href.options[0] === 'plist') {
            type = 'plist';
            uri = href.options[1];
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
        menu.appendChild(elCreate('h6', {"class": ["dropdown-header"]}, tn(name)));
        addMenuItem(menu, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
        if (type === 'plist') {
            addMenuItem(menu, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
        }
        addMenuItem(menu, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
        addMenuItem(menu, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
        addMenuItem(menu, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
    }
}
