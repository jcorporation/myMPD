"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function b64EncodeUnicode(str) {
    return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g,
        function toSolidBytes(match, p1) {
            return String.fromCharCode('0x' + p1);
    }));
}

function b64DecodeUnicode(str) {
    return decodeURIComponent(atob(str).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
}

function addMenuItem(href, text) {
    return '<a class="dropdown-item" href="#" data-href=\'' + b64EncodeUnicode(JSON.stringify(href)) + '\'>' + text +'</a>';
}

function hideMenu() {
    const menuEl = document.querySelector('[data-popover]');
    if (menuEl) {
        const m = new BSN.Popover(menuEl, {});
        m.hide();
        menuEl.removeAttribute('data-popover');
        if (menuEl.parentNode.parentNode.classList.contains('selected')) {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
        else if (app.current.app === 'Browse' && app.current.tab === 'Database') {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
        else if (app.current.app === 'Home') {
            focusTable(undefined, menuEl.parentNode.parentNode.parentNode.parentNode);
        }
    }
}

function showMenu(el, event) {
    event.preventDefault();
    event.stopPropagation();
    hideMenu();
    if (el.parentNode.nodeName === 'TH') {
        showMenuTh(el);
    }
    else {
        showMenuTd(el);
    }
}

function showMenuTh(el) {
    const table = app.current.app + (app.current.tab !== undefined ? app.current.tab : '') + (app.current.view !== undefined ? app.current.view : '');
    let menu = '<form class="p-2" id="colChecklist' + table + '">';
    menu += setColsChecklist(table);
    menu += '<button class="btn btn-success btn-block btn-sm mt-2">' + t('Apply') + '</button>';
    menu += '</form>';
    const popoverInit = new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover">' +
        '<div class="popover-arrow"></div>' +
        '<div class="popover-content" id="' + table + 'ColsDropdown">' + menu + '</div>' +
        '</div>', content: 'content'});
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementById('colChecklist' + table).addEventListener('click', function(eventClick) {
                if (eventClick.target.nodeName === 'BUTTON' && eventClick.target.classList.contains('mi')) {
                    toggleBtnChk(eventClick.target);
                    eventClick.preventDefault();
                    eventClick.stopPropagation();
                }
                else if (eventClick.target.nodeName === 'BUTTON') {
                    eventClick.preventDefault();
                    saveCols(table);
                }
            }, false);
        }, false);
    }
    popoverInit.show();
}

function showMenuTd(el) {
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

    let menu = '';
    if ((app.current.app === 'Browse' && app.current.tab === 'Filesystem') || app.current.app === 'Search' ||
        (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'Detail')) {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            (type === 'song' ? addMenuItem({"cmd": "appendAfterQueue", "options": [type, uri, nextSongPos, name]}, t('Add after current playing song')) : '') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (type !== 'plist' && type !== 'smartpls' && features.featPlaylists === true ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (type === 'song' ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '') +
            (type === 'plist' || type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : '') +
            ((type === 'plist' || type === 'smartpls') && features.featHome === true ? addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '');
            
        if (app.current.tab === 'Filesystem') {
            menu += (type === 'dir' ? addMenuItem({"cmd": "updateDB", "options": [dirname(uri), true]}, t('Update directory')) : '') +
                (type === 'dir' ? addMenuItem({"cmd": "rescanDB", "options": [dirname(uri), true]}, t('Rescan directory')) : '');
        }
        if (app.current.app === 'Search') {
            const vAlbum = getCustomDomProperty(dataNode, 'data-album');
            const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
            if (vAlbum !== undefined && vAlbumArtist !== undefined) {
                menu += '<div class="dropdown-divider"></div>' +
                    '<a class="dropdown-item" id="advancedMenuLink" data-toggle="collapse" href="#advancedMenu"><span class="mi mi-left">keyboard_arrow_right</span>Album actions</a>' +
                    '<div class="collapse" id="advancedMenu">' +
                        addMenuItem({"cmd": "_addAlbum", "options": ["appendQueue", vAlbumArtist, vAlbum]}, t('Append to queue')) +
                        addMenuItem({"cmd": "_addAlbum", "options": ["replaceQueue", vAlbumArtist, vAlbum]}, t('Replace queue')) +
                        (features.featPlaylists === true ? addMenuItem({"cmd": "_addAlbum", "options": ["addPlaylist", vAlbumArtist, vAlbum]}, t('Add to playlist')) : '') +
                    '</div>';
            }
            else {
                //songs must be arragend in one album per folder
                const baseuri = dirname(uri);
                menu += '<div class="dropdown-divider"></div>' +
                    '<a class="dropdown-item" id="advancedMenuLink" data-toggle="collapse" href="#advancedMenu"><span class="mi mi-left">keyboard_arrow_right</span>Folder actions</a>' +
                    '<div class="collapse" id="advancedMenu">' +
                        addMenuItem({"cmd": "appendQueue", "options": [type, baseuri, name]}, t('Append to queue')) +
                        addMenuItem({"cmd": "appendAfterQueue", "options": [type, baseuri, nextSongPos, name]}, t('Add after current playing song')) +
                        addMenuItem({"cmd": "replaceQueue", "options": [type, baseuri, name]}, t('Replace queue')) +
                        (features.featPlaylists === true ? addMenuItem({"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, t('Add to playlist')) : '') +
                    '</div>';
            }
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
        const albumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        const album = getCustomDomProperty(dataNode, 'data-album');
        menu += addMenuItem({"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, undefined, "Album", tagAlbumArtist, album, albumArtist]}, t('Show album')) +
            addMenuItem({"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album]}, t('Append to queue')) +
            addMenuItem({"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album]}, t('Replace queue')) +
            (features.featPlaylists === true ? addMenuItem({"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album]}, t('Add to playlist')) : '');
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'List') {
        const smartplsOnly = getCustomDomProperty(dataNode, 'data-smartpls-only');
        if (smartplsOnly === false || type !== 'smartpls') {
            menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
                addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
                (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('Edit playlist'))) +
                addMenuItem({"cmd": "showRenamePlaylist", "options": [uri]}, t('Rename playlist'));
        }
        menu += (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "showSmartPlaylist", "options": [uri]}, t('Edit smart playlist')) : '') +
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "updateSmartPlaylist", "options": [uri]}, t('Update smart playlist')) : '') +
            addMenuItem({"cmd": "showDelPlaylist", "options": [uri, smartplsOnly]}, t('Delete playlist')) +
            (features.featHome === true ? addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '');
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        const x = document.getElementById('BrowsePlaylistsDetailList');
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (getCustomDomProperty(x, 'data-ro') === 'false' ? addMenuItem({"cmd": "removeFromPlaylist", "options": [getCustomDomProperty(x, 'data-uri'), 
                    getCustomDomProperty(el.parentNode.parentNode, 'data-songpos')]}, t('Remove')) : '') +
            (features.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (isStreamUri(uri) === false ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        const trackid = getCustomDomProperty(dataNode, 'data-trackid');
        const songpos = getCustomDomProperty(dataNode, 'data-songpos');
        menu += ( trackid !== lastState.currentSongId ? addMenuItem({"cmd": "playAfterCurrent", "options": [trackid, songpos]}, t('Play after current playing song')) : '') +
            addMenuItem({"cmd": "delQueueSong", "options": ["single", trackid]}, t('Remove')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", 0, songpos]}, t('Remove all upwards')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", (songpos - 1), -1]}, t('Remove all downwards')) +
            (features.featPlaylists === true ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (isStreamUri(uri) === false ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (features.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        const pos = Number(getCustomDomProperty(dataNode, 'data-pos'));
        const vAlbum = getCustomDomProperty(dataNode, 'data-album');
        const vAlbumArtist = getCustomDomProperty(dataNode, 'data-albumartist');
        menu += (settings.jukeboxMode === 1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) :
            addMenuItem({"cmd": "appGoto", "options": ["Browse", "Database", "Detail", 0, 50, "Album", tagAlbumArtist, vAlbum, vAlbumArtist]}, t('Show album')))+
            addMenuItem({"cmd": "delQueueJukeboxSong", "options": [pos]}, t('Remove'));
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
            actionDesc = t('Add and play playlist');
            name = t('Playlist');
        }
        else if (href.cmd === 'appGoto') {
            type = 'view';
            actionDesc = t('Goto view');
            name = t('View');
        }
        else if (href.cmd === 'execScriptFromOptions') {
            type = 'script';
            actionDesc = t('Execute script');
            name = t('Script');
        }
        menu += '<h6 class="dropdown-header">' + name + '</h6>' +
                addMenuItem({"cmd": "executeHomeIcon", "options": [pos]}, actionDesc) +
                (type === 'plist' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : '') +
                addMenuItem({"cmd": "editHomeIcon", "options": [pos]}, t('Edit home icon')) +
                addMenuItem({"cmd": "duplicateHomeIcon", "options": [pos]}, t('Duplicate home icon')) +
                addMenuItem({"cmd": "deleteHomeIcon", "options": [pos]}, t('Delete home icon'));
    }

    const popoverInit = new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover">' +
        '<div class="popover-arrow"></div>' +
        '<div class="popover-content">' + menu + '</div>' +
        '</div>', content: 'content'});
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementsByClassName('popover-content')[0].addEventListener('click', function(eventClick) {
                eventClick.preventDefault();
                eventClick.stopPropagation();
                if (eventClick.target.nodeName === 'A') {
                    const dh = eventClick.target.getAttribute('data-href');
                    if (dh) {
                        const cmd = JSON.parse(b64DecodeUnicode(dh));
                        parseCmd(event, cmd);
                        hideMenu();
                    }
                }
            }, false);
            document.getElementsByClassName('popover-content')[0].addEventListener('keydown', function(eventKey) {
                eventKey.preventDefault();
                eventKey.stopPropagation();
                if (eventKey.key === 'ArrowDown' || eventKey.key === 'ArrowUp') {
                    const menuItemsHtml = this.getElementsByTagName('a');
                    const menuItems = Array.prototype.slice.call(menuItemsHtml);
                    let idx = menuItems.indexOf(document.activeElement);
                    do {
                        idx = eventKey.key === 'ArrowUp' ? (idx > 1 ? idx - 1 : 0)
                                                 : eventKey.key === 'ArrowDown' ? ( idx < menuItems.length - 1 ? idx + 1 : idx)
                                                                            : idx;
                        if ( idx === 0 || idx === menuItems.length -1 ) {
                            break;
                        }
                    } while (!menuItems[idx].offsetHeight);
                    if (menuItems[idx]) {
                        menuItems[idx].focus();
                    }
                }
                else if (eventKey.key === 'Enter') {
                    eventKey.target.click();
                }
                else if (eventKey.key === 'Escape') {
                    hideMenu();
                }
            }, false);
            const collapseLink = document.getElementById('advancedMenuLink');
            if (collapseLink) {
                collapseLink.addEventListener('click', function() {
                    const icon = this.getElementsByTagName('span')[0];
                    if (icon.textContent === 'keyboard_arrow_right') {
                        icon.textContent = 'keyboard_arrow_down';
                    }
                    else {
                        icon.textContent = 'keyboard_arrow_right';
                    }
                }, false);
                new BSN.Collapse(collapseLink);
            }
            document.getElementsByClassName('popover-content')[0].firstChild.focus();
        }, false);
    }
    popoverInit.show();
}
