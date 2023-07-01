"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module contextMenu_js */

/**
 * Shows a context menu as popover or offcanvas
 * @param {Event} event triggering event
 * @returns {void}
 */
function showContextMenu(event) {
    event.preventDefault();
    event.stopPropagation();
    //get the correct dom node for the triggering event
    let target = event.target.nodeName === 'SPAN'
               ? event.target.parentNode : event.target;
    if (target.nodeName === 'SMALL') {
        target = target.parentNode;
    }
    if (target.nodeName === 'TD') {
        //try to attach popover instance to action link in tables
        const actionLink = target.parentNode.lastElementChild.firstElementChild;
        if (actionLink !== null &&
            actionLink.nodeName === 'A')
        {
            target = actionLink;
        }
    }
    else if (target.parentNode.classList.contains('card')) {
        //attach popover instance to card
        target = target.parentNode;
    }
    
    const contextMenuType = target.getAttribute('data-contextmenu');
    logDebug('Create new context menu of type ' + contextMenuType);
    switch (contextMenuType) {
        case 'NavbarPlayback':
        case 'NavbarQueue':
        case 'NavbarBrowse':
            //use popover style context menu
            showPopover(target, contextMenuType);
            break;
        default:
            //all other context menus are displayed in an offcanvas
            showContextMenuOffcanvas(target, contextMenuType);
    }
}

/**
 * Functions to add the menu items
 */

/**
 * Creates the column check list for tables
 * @param {EventTarget} target event target
 * @param {HTMLElement} contextMenuTitle title element
 * @param {HTMLElement} contextMenuBody element to append the menu item
 * @returns {void}
 */
function createMenuColumns(target, contextMenuTitle, contextMenuBody) {
    const menu = elCreateEmpty('form', {});
    setColsChecklist(app.id, menu);
    menu.addEventListener('click', function(eventClick) {
        if (eventClick.target.nodeName === 'BUTTON') {
            toggleBtnChk(eventClick.target, undefined);
            eventClick.preventDefault();
            eventClick.stopPropagation();
        }
    }, false);
    contextMenuBody.classList.add('px-3');
    contextMenuBody.appendChild(menu);
    const applyEl = elCreateTextTn('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, 'Apply');
    contextMenuBody.appendChild(applyEl);
    applyEl.addEventListener('click', function(eventClick) {
        eventClick.preventDefault();
        saveCols(app.id);
    }, false);
    contextMenuBody.setAttribute('id', app.id + 'ColsDropdown');
}

/**
 * Adds a divider to the context menu
 * @param {HTMLElement} contextMenuBody element to append the divider
 * @returns {void}
 */
function addDivider(contextMenuBody) {
    if (contextMenuBody.lastChild &&
        contextMenuBody.lastChild.nodeName === 'A')
    {
        contextMenuBody.appendChild(
            elCreateEmpty('div', {"class": ["dropdown-divider"]})
        );
    }
}

/**
 * Adds a menu item to the context menu
 * @param {HTMLElement} contextMenuBody element to append the menu item
 * @param {object} cmd the command
 * @param {string} text menu text, will be translated
 * @returns {void}
 */
function addMenuItem(contextMenuBody, cmd, text) {
    const a = elCreateTextTn('a', {"class": ["dropdown-item"], "href": "#"}, text);
    setData(a, 'href', cmd);
    contextMenuBody.appendChild(a);
}

/**
 * Callback function to create the navbar context menu body
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} popoverBody element to append the menu items
 * @returns {void}
 */
function addMenuItemsNavbarActions(target, popoverBody) {
    const type = target.getAttribute('data-contextmenu');
    switch(type) {
        case 'NavbarPlayback':
            addMenuItem(popoverBody, {"cmd": "openModal", "options": ["modalQueueSettings"]}, 'Playback settings');
            addDivider(popoverBody);
            addMenuItemsSingleActions(popoverBody);
            addMenuItemsConsumeActions(popoverBody);
            addDivider(popoverBody);
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Playback", undefined, undefined]}, 'Show playback');
            break;
        case 'NavbarQueue':
            addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_CLEAR"}]}, 'Clear');
            addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_CROP"}]}, 'Crop');
            addMenuItem(popoverBody, {"cmd": "sendAPI", "options": [{"cmd": "MYMPD_API_QUEUE_SHUFFLE"}]}, 'Shuffle');
            addDivider(popoverBody);
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "Current", undefined]}, 'Show queue');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "LastPlayed", undefined]}, 'Show last played');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Queue", "Jukebox", undefined]}, 'Show jukebox queue');
            break;
        case 'NavbarBrowse':
            addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", false]}, 'Update database');
            addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", true]}, 'Rescan database');
            addDivider(popoverBody);
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Database", undefined]}, 'Show browse database');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Playlist", undefined]}, 'Show browse playlists');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Filesystem", undefined]}, 'Show browse filesystem');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Radio", undefined]}, 'Show browse webradio');
            break;
    }
}

/**
 * Callback function to create the disc context menu body
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuTitle element to set the menu header
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @returns {void}
 */
function addMenuItemsDiscActions(target, contextMenuTitle, contextMenuBody) {
    const dataNode = target.parentNode.parentNode;
    const disc = getData(dataNode, 'Disc');
    const albumId = getData(dataNode, 'AlbumId');

    addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["appendQueue", albumId, disc]}, 'Append to queue');
    addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["appendPlayQueue", albumId, disc]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["insertAfterCurrentQueue", albumId, disc]}, 'Insert after current playing song');
    }
    addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["replaceQueue", albumId, disc]}, 'Replace queue');
    addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["replacePlayQueue", albumId, disc]}, 'Replace queue and play');
    if (features.featPlaylists === true) {
        addMenuItem(contextMenuBody, {"cmd": "addAlbumDisc", "options": ["addPlaylist", albumId, disc]}, 'Add to playlist');
    }
}

/**
 * Appends single actions for the queue actions context menu
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @returns {void}
 */
function addMenuItemsSingleActions(contextMenuBody) {
    if (settings.partition.single === '0') {
        if (settings.partition.repeat === true &&
            settings.partition.consume === '0')
        {
            //repeat one song can only work with consume disabled
            addMenuItem(contextMenuBody, {"cmd": "clickSingle", "options": ["oneshot"]}, 'Repeat current song once');
            addMenuItem(contextMenuBody, {"cmd": "clickSingle", "options": ["1"]}, 'Repeat current song');
        }
        else if (settings.partition.repeat === true &&
                 settings.partition.autoPlay === false)
        {
            //single one-shot works only with disabled auto play
            addMenuItem(contextMenuBody, {"cmd": "clickSingle", "options": ["oneshot"]}, 'Stop playback after current song');
        }
    }
    else {
        addMenuItem(contextMenuBody, {"cmd": "clickSingle", "options": ["0"]}, 'Disable single mode');
    }
}

/**
 * Appends consume actions for the queue actions context menu
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @returns {void}
 */
function addMenuItemsConsumeActions(contextMenuBody) {
    if (settings.partition.consume === '0') {
        if (features.featConsumeOneshot === true) {
            addMenuItem(contextMenuBody, {"cmd": "clickConsume", "options": ["oneshot"]}, 'Remove current song after playback');
        }
    }
    else {
        addMenuItem(contextMenuBody, {"cmd": "clickConsume", "options": ["0"]}, 'Disable consume mode');
    }
}

/**
 * Appends album actions to the context menu
 * @param {HTMLElement | EventTarget} dataNode element with the album data
 * @param {HTMLElement} contextMenuTitle element to set the menu header
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} [albumId] the album id
 * @returns {void}
 */
function addMenuItemsAlbumActions(dataNode, contextMenuTitle, contextMenuBody, albumId) {
    if (dataNode !== null) {
        albumId = getData(dataNode, 'AlbumId');
    }
    if (contextMenuTitle !== null) {
        contextMenuTitle.textContent = tn('Album');
    }
    if (app.id !== 'QueueCurrent') {
        addMenuItem(contextMenuBody, {"cmd": "appendQueue", "options": ["album", albumId]}, 'Append to queue');
        addMenuItem(contextMenuBody, {"cmd": "appendPlayQueue", "options": ["album", albumId]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addMenuItem(contextMenuBody, {"cmd": "insertAfterCurrentQueue", "options": ["album", albumId]}, 'Insert after current playing song');
        }
        addMenuItem(contextMenuBody, {"cmd": "replaceQueue", "options": ["album", albumId]}, 'Replace queue');
        addMenuItem(contextMenuBody, {"cmd": "replacePlayQueue", "options": ["album", albumId]}, 'Replace queue and play');
    }
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showAddToPlaylist", "options": [["addPlaylist", albumId], ""]}, 'Add to playlist');
    }
    addDivider(contextMenuBody);
    if (app.id !== 'BrowseDatabaseAlbumDetail') {
        addMenuItem(contextMenuBody, {"cmd": "gotoAlbum", "options": [albumId]}, 'Album details');
    }
    for (const tag of settings.tagListBrowse) {
        if (dataNode !== null &&
            albumFilters.includes(tag))
        {
            const value = getData(dataNode, tag);
            if (value !== undefined) {
                addMenuItem(contextMenuBody, {"cmd": "gotoAlbumList", "options": [tag, value]}, 'Show all albums from ' + tag);
            }
        }
    }
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "addAlbumToHome", "options": [albumId, ""]}, 'Add to homescreen');
    }
}

/**
 * Appends actions for single songs or streams to the context menu
 * @param {HTMLElement} dataNode element with the album data
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} uri song or stream uri
 * @param {string} type type of the element: song, stream, ...
 * @param {string} name name of the element
 * @returns {void}
 */
function addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(contextMenuBody, {"cmd": "appendQueue", "options": [type, [uri]]}, 'Append to queue');
        addMenuItem(contextMenuBody, {"cmd": "appendPlayQueue", "options": [type, [uri]]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addMenuItem(contextMenuBody, {"cmd": "insertAfterCurrentQueue", "options": [type, [uri], 0, 1, false]}, 'Insert after current playing song');
        }
        addMenuItem(contextMenuBody, {"cmd": "replaceQueue", "options": [type, [uri]]}, 'Replace queue');
        addMenuItem(contextMenuBody, {"cmd": "replacePlayQueue", "options": [type, [uri]]}, 'Replace queue and play');
    }
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showAddToPlaylist", "options": [[uri], ""]}, 'Add to playlist');
        if (app.id === 'BrowsePlaylistDetail' &&
            getData(dataNode.parentNode.parentNode, 'type') === 'plist')
        {
            const plist = getData(dataNode.parentNode.parentNode, 'uri');
            const pos = getData(dataNode, 'pos');
            addMenuItem(contextMenuBody, {"cmd": "showMoveToPlaylist", "options": [plist, [pos]]}, 'Move to playlist');
        }
    }
    if (type === 'song') {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "songDetails", "options": [uri]}, 'Song details');
    }
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        if (app.id === 'BrowseRadioWebradiodb') {
            const image = getData(dataNode, 'image');
            addMenuItem(contextMenuBody, {"cmd": "addWebRadiodbToHome", "options": [uri, type, name, image]}, 'Add to homescreen');
        }
        else {
            addMenuItem(contextMenuBody, {"cmd": "addSongToHome", "options": [uri, type, name]}, 'Add to homescreen');
        }
    }
    if (app.id === 'BrowseRadioRadiobrowser') {
        const uuid = getData(dataNode, 'RADIOBROWSERUUID');
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showRadiobrowserDetails", "options": [uuid]}, 'Webradio details');
        addMenuItem(contextMenuBody, {"cmd": "showEditRadioFavorite", "options": [{
            "Name": name,
            "Genre": getData(dataNode, 'genre').replace(/,(\S)/g, ', $1'),
            "Image": getData(dataNode, 'image'),
            "StreamUri": uri,
            "Homepage": getData(dataNode, 'homepage'),
            "Country": getData(dataNode, 'country'),
            "Language": getData(dataNode, 'language'),
            "Codec": getData(dataNode, 'codec'),
            "Bitrate": getData(dataNode, 'bitrate'),
        }]}, 'Add to favorites');
    }
    else if (app.id === 'BrowseRadioWebradiodb') {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showWebradiodbDetails", "options": [uri]}, 'Webradio details');
        addMenuItem(contextMenuBody, {"cmd": "showEditRadioFavorite", "options": [{
            "Name": name,
            "Genre": getData(dataNode, 'genre'),
            "Image": getData(dataNode, 'image'),
            "StreamUri": uri,
            "Homepage": getData(dataNode, 'homepage'),
            "Country": getData(dataNode, 'country'),
            "Language": getData(dataNode, 'language'),
            "Codec": getData(dataNode, 'codec'),
            "Bitrate": getData(dataNode, 'bitrate'),
            "Description": getData(dataNode, 'description')
        }]}, 'Add to favorites');
    }
    else if (app.id === 'QueueCurrent' &&
        type === 'webradio')
    {
        addDivider(contextMenuBody);
        const webradioUri = getData(dataNode, 'webradioUri');
        addMenuItem(contextMenuBody, {"cmd": "editRadioFavorite", "options": [webradioUri]}, 'Edit webradio favorite');
    }
}

/**
 * Appends search actions to the context menu
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} expression search expression
 * @returns {void}
 */
function addMenuItemsSearchActions(contextMenuBody, expression) {
    addMenuItem(contextMenuBody, {"cmd": "appendQueue", "options": ["search", expression]}, 'Append to queue');
    addMenuItem(contextMenuBody, {"cmd": "appendPlayQueue", "options": ["search", expression]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(contextMenuBody, {"cmd": "insertAfterCurrentQueue", "options": ["search", expression, 0, 1, false]}, 'Insert after current playing song');
    }
    addMenuItem(contextMenuBody, {"cmd": "replaceQueue", "options": ["search", expression]}, 'Replace queue');
    addMenuItem(contextMenuBody, {"cmd": "replacePlayQueue", "options": ["search", expression]}, 'Replace queue and play');
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showAddToPlaylist", "options": [["SEARCH"], expression]}, 'Add to playlist');
    }
    addDivider(contextMenuBody);
    addMenuItem(contextMenuBody, {"cmd": "appGoto", "options": ["Search", undefined, undefined, 0, undefined, "any", "Title", "-", expression]}, 'Show search');
}

/**
 * Appends directory actions to the context menu
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} baseuri directory
 * @returns {void}
 */
function addMenuItemsDirectoryActions(contextMenuBody, baseuri) {
    //songs must be arranged in one album per folder
    addMenuItem(contextMenuBody, {"cmd": "appendQueue", "options": ["dir", [baseuri]]}, 'Append to queue');
    addMenuItem(contextMenuBody, {"cmd": "appendPlayQueue", "options": ["dir", [baseuri]]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(contextMenuBody, {"cmd": "insertAfterCurrentQueue", "options": ["dir", [baseuri], 0, 1, false]}, 'Insert after current playing song');
    }
    addMenuItem(contextMenuBody, {"cmd": "replaceQueue", "options": ["dir", [baseuri]]}, 'Replace queue');
    addMenuItem(contextMenuBody, {"cmd": "replacePlayQueue", "options": ["dir", [baseuri]]}, 'Replace queue and play');
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "showAddToPlaylist", "options": [[baseuri], ""]}, 'Add to playlist');
    }
    if (app.id === 'BrowseFilesystem') {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "updateDB", "options": [baseuri, false]}, 'Update directory');
        addMenuItem(contextMenuBody, {"cmd": "updateDB", "options": [baseuri, true]}, 'Rescan directory');
    }
    addDivider(contextMenuBody);
    addMenuItem(contextMenuBody, {"cmd": "gotoFilesystem", "options": [baseuri, "dir"]}, 'Open directory');
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(contextMenuBody);
        addMenuItem(contextMenuBody, {"cmd": "addDirToHome", "options": [baseuri, baseuri]}, 'Add to homescreen');
    }
}

/**
 * Appends actions for webradio favorites to the context menu
 * @param {HTMLElement} target element with the data
 * @param {HTMLElement} contextMenuTitle element for the menu title
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @returns {void}
 */
function addMenuItemsWebradioFavoritesActions(target, contextMenuTitle, contextMenuBody) {
    const type = getData(target, 'type');
    const uri = getData(target, 'uri');
    const name = getData(target, 'name');
    addMenuItemsPlaylistActions(target, contextMenuBody, type, uri, name);
    addDivider(contextMenuBody);
    addMenuItem(contextMenuBody, {"cmd": "editRadioFavorite", "options": [uri]}, 'Edit webradio favorite');
    addMenuItem(contextMenuBody, {"cmd": "deleteRadioFavorites", "options": [[uri]]}, 'Delete webradio favorite');
}

/**
 * Appends actions for webradio favorites home icon to the context menu
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} uri webradio favorite uri
 * @returns {void}
 */
function addMenuItemsWebradioFavoritesHomeActions(contextMenuBody, uri) {
    addDivider(contextMenuBody);
    addMenuItem(contextMenuBody, {"cmd": "editRadioFavorite", "options": [uri]}, 'Edit webradio favorite');
}

/**
 * Appends actions for playlists to the context menu
 * @param {HTMLElement | EventTarget} dataNode element with the data
 * @param {HTMLElement} contextMenuBody element to append the menu items
 * @param {string} type playlist type: plist, smartpls
 * @param {string} uri playlist uri
 * @param {string} name playlist name
 * @returns {void}
 */
function addMenuItemsPlaylistActions(dataNode, contextMenuBody, type, uri, name) {
    addMenuItem(contextMenuBody, {"cmd": "appendQueue", "options": [type, [uri]]}, 'Append to queue');
    addMenuItem(contextMenuBody, {"cmd": "appendPlayQueue", "options": [type, [uri]]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(contextMenuBody, {"cmd": "insertAfterCurrentQueue", "options": [type, [uri], 0, 1, false]}, 'Add after current playing song');
    }
    addMenuItem(contextMenuBody, {"cmd": "replaceQueue", "options": [type, [uri]]}, 'Replace queue');
    addMenuItem(contextMenuBody, {"cmd": "replacePlayQueue", "options": [type, [uri]]}, 'Replace queue and play');
    if (features.featHome === true) {
        if (app.id !== 'Home') {
            addDivider(contextMenuBody);
            if (app.id === 'BrowseRadioFavorites') {
                let image = getData(dataNode, 'image');
                if (isHttpUri(image) === false) {
                    image = basename(image, false);
                }
                addMenuItem(contextMenuBody, {"cmd": "addRadioFavoriteToHome", "options": [uri, type, name, image]}, 'Add to homescreen');
            }
            else {
                addMenuItem(contextMenuBody, {"cmd": "addPlistToHome", "options": [uri, type, name]}, 'Add to homescreen');
            }
        }
    }
    if (app.id !== 'BrowsePlaylistList') {
        if (type === 'plist' ||
            type === 'smartpls')
        {
            if (isMPDplaylist(uri) === true) {
                addDivider(contextMenuBody);
                addMenuItem(contextMenuBody, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
            }
            else {
                addMenuItem(contextMenuBody, {"cmd": "gotoFilesystem", "options": [uri, "plist"]}, 'View playlist');
            }
        }
    }
}

/**
 * Creates the first context menu actions for list context menus
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuTitle title element
 * @param {HTMLElement} contextMenuBody content element
 * @returns {boolean} true on success, else false
 */
function createMenuLists(target, contextMenuTitle, contextMenuBody) {
    const dataNode = target.parentNode.parentNode;
    const type = getData(dataNode, 'type');
    const uri = getData(dataNode, 'uri');
    const name = getData(dataNode, 'name');

    contextMenuTitle.textContent = tn(typeFriendly[type]);

    switch(app.id) {
        case 'BrowseFilesystem':
        case 'Search':
        case 'BrowseRadioRadiobrowser':
        case 'BrowseRadioWebradiodb':
        case 'BrowseDatabaseAlbumDetail': {
            switch(type) {
                case 'song':
                case 'stream':
                    addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name);
                    break;
                case 'dir':
                    addMenuItemsDirectoryActions(contextMenuBody, uri);
                    break;
                case 'plist':
                case 'smartpls':
                    addMenuItemsPlaylistActions(dataNode, contextMenuBody, type, uri, name);
                    break;
                default:
                    return false;
            }
            return true;
        }
        case 'BrowsePlaylistList': {
            const smartplsOnly = getData(dataNode, 'smartpls-only');
            if (smartplsOnly === false ||
                type !== 'smartpls')
            {
                addMenuItemsPlaylistActions(dataNode, contextMenuBody, type, uri, name);
                addDivider(contextMenuBody);
                if (type === 'smartpls') {
                    addMenuItem(contextMenuBody, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
                }
                else {
                    addMenuItem(contextMenuBody, {"cmd": "playlistDetails", "options": [uri]}, 'Edit playlist');
                }
                addMenuItem(contextMenuBody, {"cmd": "showRenamePlaylist", "options": [uri]}, 'Rename playlist');
                addMenuItem(contextMenuBody, {"cmd": "showCopyPlaylist", "options": [uri]}, 'Copy playlist');
                if (type === 'plist') {
                    addMenuItem(contextMenuBody, {"cmd": "playlistValidateDedup", "options": [uri, true]}, 'Validate and deduplicate playlist');
                }
            }
            addMenuItem(contextMenuBody, {"cmd": "showDelPlaylist", "options": [[uri]]}, 'Delete playlist');
            if (settings.smartpls === true &&
                type === 'smartpls')
            {
                addDivider(contextMenuBody);
                addMenuItem(contextMenuBody, {"cmd": "showSmartPlaylist", "options": [uri]}, 'Edit smart playlist');
                addMenuItem(contextMenuBody, {"cmd": "updateSmartPlaylist", "options": [uri]}, 'Update smart playlist');
            }
            return true;
        }
        case 'BrowsePlaylistDetail': {
            const table = document.getElementById('BrowsePlaylistDetailList');
            addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name);
            if (getData(table, 'ro') === false) {
                addDivider(contextMenuBody);
                const plist = getData(table, 'uri');
                const songpos = getData(dataNode, 'songpos');
                const playlistLength = getData(table, 'playlistlength');
                addMenuItem(contextMenuBody, {"cmd": "showSetSongPos", "options": [plist, songpos, -1]}, 'Move song');
                addMenuItem(contextMenuBody, {"cmd": "removeFromPlaylistPositions", "options": [plist, [songpos]]}, 'Remove');
                if (features.featPlaylistRmRange === true) {
                    if (songpos > 0) {
                        addMenuItem(contextMenuBody, {"cmd": "removeFromPlaylistRange", "options": [plist, 0, songpos]}, 'Remove all upwards');
                    }
                    if (songpos + 1 < playlistLength) {
                        addMenuItem(contextMenuBody, {"cmd": "removeFromPlaylistRange", "options": [plist, songpos + 1, -1]}, 'Remove all downwards');
                    }
                }
            }
            return true;
        }
        case 'QueueCurrent': {
            const songid = getData(dataNode, 'songid');
            const songpos = getData(dataNode, 'songpos');
            addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name);
            addDivider(contextMenuBody);
            if (currentState.currentSongId !== -1 &&
                songid !== currentState.currentSongId &&
                features.featWhence === true)
            {
                addMenuItem(contextMenuBody, {"cmd": "playAfterCurrent", "options": [[songid]]}, 'Play after current playing song');
            }
            if (settings.partition.random === true) {
                addMenuItem(contextMenuBody, {"cmd": "showSetSongPriority", "options": [songid]}, 'Set priority');
            }
            else {
                addMenuItem(contextMenuBody, {"cmd": "showSetSongPos", "options": ["queue", songpos, songid]}, 'Move song');
            }
            if (songid === currentState.currentSongId) {
                addMenuItemsSingleActions(contextMenuBody);
                addMenuItemsConsumeActions(contextMenuBody);
            }
            addDivider(contextMenuBody);
            addMenuItem(contextMenuBody, {"cmd": "removeFromQueueIDs", "options": [[songid]]}, 'Remove');
            if (songpos > 0) {
                addMenuItem(contextMenuBody, {"cmd": "removeFromQueueRange", "options": [0, songpos]}, 'Remove all upwards');
            }
            if (songpos + 1 < currentState.queueLength) {
                addMenuItem(contextMenuBody, {"cmd": "removeFromQueueRange", "options": [songpos + 1, -1]}, 'Remove all downwards');
            }
            return true;
        }
        case 'QueueLastPlayed': {
            addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name);
            return true;
        }
        case 'QueueJukebox': {
            const pos = Number(getData(dataNode, 'pos'));
            if (settings.partition.jukeboxMode === 'song') {
                addMenuItemsSongActions(dataNode, contextMenuBody, uri, type, name);
            }
            else if (settings.partition.jukeboxMode === 'album') {
                addMenuItemsAlbumActions(dataNode, null, contextMenuBody);
            }
            addDivider(contextMenuBody);
            addMenuItem(contextMenuBody, {"cmd": "delQueueJukeboxEntries", "options": [[pos]]}, 'Remove');
            return true;
        }
    }
    return false;
}

/**
 * Creates the secondary context menu actions for list context menus
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuTitle title element
 * @param {HTMLElement} contextMenuBody content element
 * @returns {boolean} true on success, else false
 */
function createMenuListsSecondary(target, contextMenuTitle, contextMenuBody) {
    switch(app.id) {
        case 'Search':
        case 'QueueCurrent':
        case 'QueueLastPlayed':
        case 'QueueJukebox':
        case 'BrowseFilesystem':
        case 'BrowseDatabaseAlbumDetail':
        case 'BrowsePlaylistDetail': {
            const dataNode = target.parentNode.parentNode;
            const type = getData(dataNode, 'type');
            const uri = getData(dataNode, 'uri');

            if (isStreamUri(uri) === true ||
                (app.id === 'BrowseFilesystem' && type === 'dir') ||
                (app.id === 'BrowseFilesystem' && type === 'plist') ||
                (app.id === 'BrowseFilesystem' && type === 'smartpls') ||
                (app.id === 'QueueJukebox' && settings.partition.jukeboxMode === 'album'))
            {
                return false;
            }
            const albumid = getData(dataNode, 'AlbumId');
            if (albumid !== undefined) {
                contextMenuTitle.textContent = tn('Album');
                addMenuItemsAlbumActions(dataNode, null, contextMenuBody);
            }
            else {
                contextMenuTitle.textContent = tn('Directory');
                const baseuri = dirname(uri);
                addMenuItemsDirectoryActions(contextMenuBody, baseuri);
            }
            return true;
        }
    }
    return false;
}

/**
 * Creates the content of the first home icon actions for the context menu
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuTitle title element
 * @param {HTMLElement} contextMenuBody content element
 * @returns {boolean} true on success, else false
 */
function createMenuHome(target, contextMenuTitle, contextMenuBody) {
    const pos = getData(target, 'pos');
    const href = getData(target, 'href');
    if (href === undefined) {
        return false;
    }
    let type = '';
    let actionDesc = '';
    switch(href.cmd) {
        case 'appGoto':
            type = 'view';
            actionDesc = friendlyActions[href.cmd];
            break;
        case 'execScriptFromOptions':
            type = 'script';
            actionDesc = friendlyActions[href.cmd];
            break;
        case 'openExternalLink':
            type = 'externalLink';
            actionDesc = friendlyActions[href.cmd];
            break;
        case 'openModal':
            type = 'modal';
            actionDesc = friendlyActions[href.cmd];
            break;
        default:
            type = href.options[0];
            actionDesc = friendlyActions[href.cmd];
    }
    contextMenuTitle.textContent = tn(typeFriendly[type]);
    switch(type) {
        case 'plist':
        case 'smartpls':
            addMenuItemsPlaylistActions(target, contextMenuBody, type, href.options[1], href.options[1]);
            break;
        case 'webradio':
            addMenuItemsPlaylistActions(target, contextMenuBody, type, href.options[1], href.options[1]);
            addMenuItemsWebradioFavoritesHomeActions(contextMenuBody, href.options[1].substr(17));
            break;
        case 'dir':
            addMenuItemsDirectoryActions(contextMenuBody, href.options[1]);
            break;
        case 'song':
        case 'stream':
            addMenuItemsSongActions(null, contextMenuBody, href.options[1], type, href.options[1]);
            break;
        case 'search':
            addMenuItemsSearchActions(contextMenuBody, href.options[1]);
            break;
        case 'album':
            addMenuItemsAlbumActions(null, null, contextMenuBody, href.options[1]);
            break;
        case 'view':
        case 'externalLink':
        case 'modal':
            addMenuItem(contextMenuBody, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
            break;
        case 'script':
            addMenuItem(contextMenuBody, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
            addMenuItem(contextMenuBody, {"cmd": "showEditScriptModal", "options": [href.options[0]]}, 'Edit script');
            break;
    }
    return true;
}

/**
 * Creates the contents of the secondary home icon actions for the context menu
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuTitle title element
 * @param {HTMLElement} contextMenuBody content element
 * @returns {boolean} true on success, else false
 */
function createMenuHomeSecondary(target, contextMenuTitle, contextMenuBody) {
    const pos = getData(target, 'pos');
    contextMenuTitle.textContent = tn('Homeicon');
    addMenuItem(contextMenuBody, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
    addMenuItem(contextMenuBody, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
    addMenuItem(contextMenuBody, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
    return true;
}
