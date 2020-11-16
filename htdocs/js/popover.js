"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
    let menuEl = document.querySelector('[data-popover]');
    if (menuEl) {
        let m = new BSN.Popover(menuEl, {});
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
    //if (el.getAttribute('data-init')) {
    //    return;
    //}
    if (el.parentNode.nodeName === 'TH') {
        showMenuTh(el);
    }
    else {
        showMenuTd(el);
    }
}

function showMenuTh(el) {
    let table = app.current.app + (app.current.tab !== undefined ? app.current.tab : '') + (app.current.view !== undefined ? app.current.view : '');
    let menu = '<form class="p-2" id="colChecklist' + table + '">';
    menu += setColsChecklist(table);
    menu += '<button class="btn btn-success btn-block btn-sm mt-2">' + t('Apply') + '</button>';
    menu += '</form>';
    new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover" role="tooltip">' +
        '<div class="arrow"></div>' +
        '<div class="popover-content" id="' + table + 'ColsDropdown' + '">' + menu + '</div>' +
        '</div>', content: ' '});
    let popoverInit = el.Popover;
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementById('colChecklist' + table).addEventListener('click', function(eventClick) {
                if (eventClick.target.nodeName === 'BUTTON' && eventClick.target.classList.contains('material-icons')) {
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
    let type = el.getAttribute('data-type');
    let uri = decodeURI(el.getAttribute('data-uri'));
    let name = decodeURI(el.getAttribute('data-name'));
    let nextsongpos = 0;
    if (type === null || uri === '') {
        type = el.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.getAttribute('data-uri'));
        name = el.parentNode.getAttribute('data-name');
    }
    if (type === null || uri === '') {
        type = el.parentNode.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.parentNode.getAttribute('data-uri'));
        name = el.parentNode.parentNode.getAttribute('data-name');
    }
    
    if (lastState) {
        nextsongpos = lastState.nextSongPos;
    }

    let menu = '';
    if ((app.current.app === 'Browse' && app.current.tab === 'Filesystem') || app.current.app === 'Search' ||
        (app.current.app === 'Browse' && app.current.tab === 'Database')) {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            (type === 'song' ? addMenuItem({"cmd": "appendAfterQueue", "options": [type, uri, nextsongpos, name]}, t('Add after current playing song')) : '') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (type !== 'plist' && type !== 'smartpls' && settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (type === 'song' ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '') +
            (type === 'plist' || type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : '') +
            ((type === 'plist' || type === 'smartpls') && settings.featHome === true ? addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '') +
            (type === 'dir' && settings.featBookmarks ? addMenuItem({"cmd": "showBookmarkSave", "options": [-1, name, uri, type]}, t('Add bookmark')) : '');
        if (app.current.tab === 'Filesystem') {
            menu += (type === 'dir' ? addMenuItem({"cmd": "updateDB", "options": [dirname(uri), true]}, t('Update directory')) : '') +
                (type === 'dir' ? addMenuItem({"cmd": "rescanDB", "options": [dirname(uri), true]}, t('Rescan directory')) : '');
        }
        if (app.current.app === 'Search') {
            //songs must be arragend in one album per folder
            let baseuri = dirname(uri);
            menu += '<div class="dropdown-divider"></div>' +
                '<a class="dropdown-item" id="advancedMenuLink" data-toggle="collapse" href="#advancedMenu"><span class="material-icons material-icons-small-left">keyboard_arrow_right</span>Album actions</a>' +
                '<div class="collapse" id="advancedMenu">' +
                    addMenuItem({"cmd": "appendQueue", "options": [type, baseuri, name]}, t('Append to queue')) +
                    addMenuItem({"cmd": "appendAfterQueue", "options": [type, baseuri, nextsongpos, name]}, t('Add after current playing song')) +
                    addMenuItem({"cmd": "replaceQueue", "options": [type, baseuri, name]}, t('Replace queue')) +
                    (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, t('Add to playlist')) : '') +
                '</div>';
        }
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'All') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('View playlist')) : addMenuItem({"cmd": "playlistDetails", "options": [uri]}, t('Edit playlist')))+
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "showSmartPlaylist", "options": [uri]}, t('Edit smart playlist')) : '') +
            (settings.smartpls === true && type === 'smartpls' ? addMenuItem({"cmd": "updateSmartPlaylist", "options": [uri]}, t('Update smart playlist')) : '') +
            addMenuItem({"cmd": "showRenamePlaylist", "options": [uri]}, t('Rename playlist')) + 
            addMenuItem({"cmd": "showDelPlaylist", "options": [uri]}, t('Delete playlist')) +
            (settings.featHome === true ?addMenuItem({"cmd": "addPlistToHome", "options": [uri, name]}, t('Add to homescreen')) : '');
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        let x = document.getElementById('BrowsePlaylistsDetailList');
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (x.getAttribute('data-ro') === 'false' ? addMenuItem({"cmd": "removeFromPlaylist", "options": [x.getAttribute('data-uri'), 
                    el.parentNode.parentNode.getAttribute('data-songpos')]}, t('Remove')) : '') +
            (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        menu += addMenuItem({"cmd": "delQueueSong", "options": ["single", el.parentNode.parentNode.getAttribute('data-trackid')]}, t('Remove')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", 0, el.parentNode.parentNode.getAttribute('data-songpos')]}, t('Remove all upwards')) +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", (parseInt(el.parentNode.parentNode.getAttribute('data-songpos'))-1), -1]}, t('Remove all downwards')) +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, t('Append to queue')) +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, t('Replace queue')) +
            (settings.featPlaylists ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri, ""]}, t('Add to playlist')) : '') +
            (uri.indexOf('http') === -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) : '');
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        menu += addMenuItem({"cmd": "songDetails", "options": [uri]}, t('Song details')) +
            addMenuItem({"cmd": "delQueueJukeboxSong", "options": [el.parentNode.parentNode.getAttribute('data-pos')]}, t('Remove'));
    }
    else if (app.current.app === 'Home') {
        const pos = parseInt(el.parentNode.getAttribute('data-pos'));
        const href = JSON.parse(el.parentNode.getAttribute('data-href'));
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

    new BSN.Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover" role="tooltip">' +
        '<div class="arrow"></div>' +
        '<div class="popover-content">' + menu + '</div>' +
        '</div>', content: ' '});
    let popoverInit = el.Popover;
    if (el.getAttribute('data-init') === null) {
        el.setAttribute('data-init', 'true');
        el.addEventListener('shown.bs.popover', function(event) {
            event.target.setAttribute('data-popover', 'true');
            document.getElementsByClassName('popover-content')[0].addEventListener('click', function(eventClick) {
                eventClick.preventDefault();
                eventClick.stopPropagation();
                if (eventClick.target.nodeName === 'A') {
                    let dh = eventClick.target.getAttribute('data-href');
                    if (dh) {
                        let cmd = JSON.parse(b64DecodeUnicode(dh));
                        parseCmd(event, cmd);
                        hideMenu();
                    }
                }
            }, false);
            document.getElementsByClassName('popover-content')[0].addEventListener('keydown', function(eventKey) {
                eventKey.preventDefault();
                eventKey.stopPropagation();
                if (eventKey.key === 'ArrowDown' || eventKey.key === 'ArrowUp') {
                    let menuItemsHtml = this.getElementsByTagName('a');
                    let menuItems = Array.prototype.slice.call(menuItemsHtml);
                    let idx = menuItems.indexOf(document.activeElement);
                    do {
                        idx = eventKey.key === 'ArrowUp' ? (idx > 1 ? idx - 1 : 0)
                                                 : eventKey.key === 'ArrowDown' ? ( idx < menuItems.length - 1 ? idx + 1 : idx)
                                                                            : idx;
                        if ( idx === 0 || idx === menuItems.length -1 ) {
                            break;
                        }
                    } while ( !menuItems[idx].offsetHeight )
                    menuItems[idx] && menuItems[idx].focus();
                }
                else if (eventKey.key === 'Enter') {
                    eventKey.target.click();
                }
                else if (eventKey.key === 'Escape') {
                    hideMenu();
                }
            }, false);
            let collapseLink = document.getElementById('advancedMenuLink');
            if (collapseLink) {
                collapseLink.addEventListener('click', function() {
                    let icon = this.getElementsByTagName('span')[0];
                    if (icon.innerText === 'keyboard_arrow_right') {
                        icon.innerText = 'keyboard_arrow_down';
                    }
                    else {
                        icon.innerText = 'keyboard_arrow_right';
                    }
                }, false);
                new BSN.Collapse(collapseLink);
            }
            document.getElementsByClassName('popover-content')[0].firstChild.focus();
        }, false);
    }
    popoverInit.show();
}
