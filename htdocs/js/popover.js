"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module popover_js */

/**
 * Hides all popovers
 * @param {EventTarget} [thisEl] triggering element
 */
function hidePopover(thisEl) {
    const popoverEls = document.querySelectorAll('[aria-describedby]');
    for (const el of popoverEls) {
        if (thisEl === el) {
            //do not hide popover that should be opened
            continue;
        }
        BSN.Popover.getInstance(el).hide();
    }
    if (popoverEls.length === 0) {
        //handle popover dom nodes without a trigger element
        const popover = document.querySelector('.popover');
        if (popover !== null) {
            //simply remove the popover dom node
            popover.remove();
        }
    }
}

/**
 * Shows a popover menu
 * @param {Event} event triggering event
 * @returns {void}
 */
function showPopover(event) {
    event.preventDefault();
    event.stopPropagation();
    //get the dom node to attach the popover object
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
    hidePopover(target);
    //popover is shown
    if (target.getAttribute('aria-describedby') !== null ||
        target.classList.contains('not-clickable'))
    {
        return;
    }
    //check for existing popover instance
    let popoverInit = BSN.Popover.getInstance(target);
    //create it if no popover instance is found
    if (popoverInit === null) {
        const popoverType = target.getAttribute('data-popover');
        logDebug('Create new popover of type ' + popoverType);
        switch (popoverType) {
            case 'columns':
                //column select in table header
                popoverInit = createPopoverColumns(target);
                break;
            case 'disc':
                //disc actions in album details view
                popoverInit = createPopoverSimple(target, 'Disc', addMenuItemsDiscActions);
                break;
            case 'NavbarPlayback':
            case 'NavbarQueue':
            case 'NavbarBrowse':
                //navbar icons
                popoverInit = createPopoverSimple(target, target.getAttribute('title'), addMenuItemsNavbarActions);
                break;
            case 'home':
                //home card actions
                popoverInit = createPopoverTabs(target, createMenuHome, createMenuHomeSecondary);
                break;
            case 'webradio':
                //webradio favorite actions
                popoverInit = createPopoverSimple(target, 'Webradio', addMenuItemsWebradioFavoritesActions);
                break;
            case 'album':
                //album action in album list
                popoverInit = createPopoverSimple(target, 'Album', addMenuItemsAlbumActions);
                break;
            default:
                popoverInit = createPopoverTabs(target, createMenuLists, createMenuListsSecondary);
        }
    }
    //show the popover
    popoverInit.show();
}

/**
 * Creates the popover body
 * @param {string} template tabs = create a popover body with two tabes, else create an empty body
 * @returns {HTMLElement} the popover body
 */
function createPopoverBody(template) {
    if (template === 'tabs') {
        return elCreateNodes('div', {"class": ["popover-tabs", "py-2"]}, [
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
               ]);
    }
    return elCreateEmpty('div', {"class": ["popover-body"]})
}

/**
 * Creates a new BSN popover
 * @param {EventTarget} el triggering element
 * @param {string} title popover title 
 * @param {string} [bodyTemplate] the popover body
 * @returns {object} BSN popover object
 */
function createPopoverInit(el, title, bodyTemplate) {
    const template = elCreateNodes('div', {"class": ["popover"]}, [
                   elCreateEmpty('div', {"class": ["popover-arrow"]}),
                   elCreateEmpty('h3', {"class": ["popover-header"]}),
                   createPopoverBody(bodyTemplate)
               ]);
    const options = {
        trigger: 'manual',
        delay: 0,
        dismissible: false,
        title: (title !== '' ? elCreateText('span', {}, title) : ''),
        template: template, content: document.createTextNode('dummy')
    };

    let popoverType = el.getAttribute('data-popover');
    if (popoverType === null) {
        popoverType = el.getAttribute('data-col');
    }
    if (popoverType === null) {
        popoverType = el.parentNode.getAttribute('data-col');
    }
    switch(popoverType) {
        case 'columns':
        case 'Action':
            options.placement = 'left';
            break;
        case 'NavbarPlayback':
        case 'NavbarQueue':
        case 'NavbarBrowse':
            // @ts-ignore
            options.placement = getXpos(el) < 100 ? 'right' : 'bottom';
            break;
    }

    return new BSN.Popover(el, options);
}

/**
 * Creates the click handler for the popover menu
 * @param {HTMLElement} el container of the menu items
 */
function createPopoverClickHandler(el) {
    el.addEventListener('click', function(eventClick) {
        if (eventClick.target.nodeName === 'A') {
            const cmd = getData(eventClick.target, 'href');
            if (cmd) {
                parseCmd(eventClick, cmd);
                hidePopover();
            }
        }
        eventClick.preventDefault();
        eventClick.stopPropagation();
    }, false);
}

/**
 * Creates a popover for the column select for tables
 * @param {EventTarget} el triggering element
 * @returns {object} BSN popover object
 */
function createPopoverColumns(el) {
    const popoverInit = createPopoverInit(el, tn('Columns'));
    //update content on each show event
    el.addEventListener('show.bs.popover', function() {
        const menu = elCreateEmpty('form', {});
        setColsChecklist(app.id, menu);
        menu.addEventListener('click', function(eventClick) {
            if (eventClick.target.nodeName === 'BUTTON') {
                toggleBtnChk(eventClick.target, undefined);
                eventClick.preventDefault();
                eventClick.stopPropagation();
            }
        }, false);
        const popoverBody = popoverInit.tooltip.querySelector('.popover-body');
        elReplaceChild(popoverBody, menu);
        const applyEl = elCreateTextTn('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, 'Apply');
        popoverBody.appendChild(applyEl);
        applyEl.addEventListener('click', function(eventClick) {
            eventClick.preventDefault();
            saveCols(app.id);
        }, false);
        popoverBody.setAttribute('id', app.id + 'ColsDropdown');
    }, false);

    return popoverInit;
}

/**
 * Creates a simple popover
 * @param {EventTarget} el triggering element
 * @param {string} title popover title
 * @param {Function} contentCallback callback to create the popover content
 * @returns {object} BSN popover object
 */
function createPopoverSimple(el, title, contentCallback) {
    const popoverInit = createPopoverInit(el, tn(title));
    //update content on each show event
    el.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.tooltip.querySelector('.popover-body').replaceWith(popoverBody);
        contentCallback(popoverBody, el);
        createPopoverClickHandler(popoverBody);
    }, false);

    return popoverInit;
}

/**
 * Creates a popover with two tabs
 * @param {EventTarget} el triggering element
 * @param {Function} tab1Callback callback to create the popover content for the first tab
 * @param {Function} tab2Callback callback to create the popover content for the second tab
 * @returns {object} BSN popover object
 */
function createPopoverTabs(el, tab1Callback, tab2Callback) {
    const popoverInit = createPopoverInit(el, '', 'tabs');
    //update content on each show event
    el.addEventListener('show.bs.popover', function() {
        popoverInit.tooltip.querySelector('.popover-tabs').replaceWith(createPopoverBody('tabs'));
        const tabHeader = popoverInit.tooltip.querySelectorAll('.nav-link');
        const tabPanes = popoverInit.tooltip.querySelectorAll('.tab-pane');
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
                tab1Callback(el, tabHeader[0], tabPanes[0]) :
                tab2Callback(el, tabHeader[1], tabPanes[1]);

            if (created === true) {
                createPopoverClickHandler(tabPanes[i]);
            }
            else {
                popoverInit.tooltip.querySelector('.popover-header').textContent = tabHeader[0].textContent;
                tabHeader[0].parentNode.parentNode.remove();
            }
        }
    }, false);

    return popoverInit;
}

/**
 * Adds a divider to the popover menu
 * @param {HTMLElement} tabContent element to append the divider
 */
function addDivider(tabContent) {
    if (tabContent.lastChild &&
        tabContent.lastChild.nodeName !== 'div')
    {
        tabContent.appendChild(
            elCreateEmpty('div', {"class": ["dropdown-divider"]})
        );
    }
}

/**
 * Adds a menu item to the popover menu
 * @param {HTMLElement} tabContent element to append the menu item
 * @param {*} cmd the command
 * @param {*} text menu text, will be translated
 */
function addMenuItem(tabContent, cmd, text) {
    const a = elCreateTextTn('a', {"class": ["dropdown-item"], "href": "#"}, text);
    setData(a, 'href', cmd);
    tabContent.appendChild(a);
}

/**
 * Callback function to create the navbar popover menu body
 * @param {HTMLElement} popoverBody element to append the menu items
 * @param {EventTarget} el triggering element
 */
function addMenuItemsNavbarActions(popoverBody, el) {
    const type = el.getAttribute('data-popover');
    switch(type) {
        case 'NavbarPlayback':
            addMenuItem(popoverBody, {"cmd": "openModal", "options": ["modalQueueSettings"]}, 'Playback settings');
            addMenuItemsSingleActions(popoverBody);
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
            addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", true, false, false]}, 'Update database');
            addMenuItem(popoverBody, {"cmd": "updateDB", "options": ["", true, false, true]}, 'Rescan database');
            addDivider(popoverBody);
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Database", undefined]}, 'Show browse database');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Playlists", undefined]}, 'Show browse playlists');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Filesystem", undefined]}, 'Show browse filesystem');
            addMenuItem(popoverBody, {"cmd": "appGoto", "options": ["Browse", "Radio", undefined]}, 'Show browse webradio');
            break;
    }
}

/**
 * Callback function to create the disc popover menu body
 * @param {HTMLElement} popoverBody element to append the menu items
 * @param {EventTarget} el triggering element
 */
function addMenuItemsDiscActions(popoverBody, el) {
    const disc = getData(el.parentNode.parentNode, 'Disc');
    const album = getData(el.parentNode.parentNode, 'Album');
    const albumArtist = getData(el.parentNode.parentNode, 'AlbumArtist');

    addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album, disc]}, 'Append to queue');
    addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["appendPlayQueue", albumArtist, album, disc]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["insertAfterCurrentQueue", albumArtist, album, disc]}, 'Insert after current playing song');
    }
    addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album, disc]}, 'Replace queue');
    addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["replacePlayQueue", albumArtist, album, disc]}, 'Replace queue and play');
    if (features.featPlaylists === true) {
        addMenuItem(popoverBody, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album, disc]}, 'Add to playlist');
    }
}

/**
 * Appends single actions for the queue actions popover
 * @param {HTMLElement} popoverBody element to append the menu items
 */
function addMenuItemsSingleActions(popoverBody) {
    if (settings.partition.single === '0') {
        if (settings.partition.repeat === true &&
            settings.partition.consume === '0')
        {
            //repeat one song can only work with consume disabled
            addMenuItem(popoverBody, {"cmd": "clickSingle", "options": ["oneshot"]}, 'Repeat current song once');
            addMenuItem(popoverBody, {"cmd": "clickSingle", "options": ["1"]}, 'Repeat current song');
        }
        else if (settings.partition.repeat === true &&
                 settings.partition.autoPlay === false)
        {
            //single one-shot works only with disabled auto play
            addMenuItem(popoverBody, {"cmd": "clickSingle", "options": ["oneshot"]}, 'Stop playback after current song');
        }
    }
    else {
        addMenuItem(popoverBody, {"cmd": "clickSingle", "options": ["0"]}, 'Disable single mode');
    }
}

/**
 * Appends album actions to the popover
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {HTMLElement} dataNode element with the album data
 * @param {object} [albumArtist] array of album artist names
 * @param {string} [album] album name
 */
function addMenuItemsAlbumActions(tabContent, dataNode, albumArtist, album) {
    if (dataNode !== null) {
        albumArtist = getData(dataNode, 'AlbumArtist');
        album = getData(dataNode, 'Album');
    }
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendQueue", albumArtist, album, undefined]}, 'Append to queue');
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["appendPlayQueue", albumArtist, album, undefined]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["insertAfterCurrentQueue", albumArtist, album, undefined]}, 'Insert after current playing song');
        }
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replaceQueue", albumArtist, album, undefined]}, 'Replace queue');
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["replacePlayQueue", albumArtist, album, undefined]}, 'Replace queue and play');
    }
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "_addAlbum", "options": ["addPlaylist", albumArtist, album, undefined]}, 'Add to playlist');
    }
    addDivider(tabContent);
    if (app.id !== 'BrowseDatabaseAlbumDetail') {
        addMenuItem(tabContent, {"cmd": "gotoAlbum", "options": [albumArtist, album]}, 'Album details');
    }
    for (const tag of settings.tagListBrowse) {
        if (tag === tagAlbumArtist) {
            addMenuItem(tabContent, {"cmd": "gotoAlbumList", "options": [tagAlbumArtist, albumArtist]}, 'Show all albums from artist');
        }
        else if (dataNode !== null && albumFilters.includes(tag)) {
            const value = getData(dataNode, tag);
            if (value !== undefined) {
                addMenuItem(tabContent, {"cmd": "gotoAlbumList", "options": [tag, value]}, 'Show all albums from ' + tag);
            }
        }
    }
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "addAlbumToHome", "options": [albumArtist, album]}, 'Add to homescreen');
    }
}

/**
 * Appends actions for single songs or streams to the popover
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {HTMLElement} dataNode element with the album data
 * @param {string} uri song or stream uri
 * @param {string} type type of the element: song, stream, ...
 * @param {string} name name of the element
 */
function addMenuItemsSongActions(tabContent, dataNode, uri, type, name) {
    if (app.id !== 'QueueCurrent') {
        addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri]}, 'Append to queue');
        addMenuItem(tabContent, {"cmd": "appendPlayQueue", "options": [type, uri]}, 'Append to queue and play');
        if (features.featWhence === true) {
            addMenuItem(tabContent, {"cmd": "insertAfterCurrentQueue", "options": [type, uri, 0, 1, false]}, 'Insert after current playing song');
        }
        addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri]}, 'Replace queue');
        addMenuItem(tabContent, {"cmd": "replacePlayQueue", "options": [type, uri]}, 'Replace queue and play');
    }
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [uri, ""]}, 'Add to playlist');
    }
    if (type === 'song') {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "songDetails", "options": [uri]}, 'Song details');
    }
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        if (app.id === 'BrowseRadioWebradiodb') {
            const image = getData(dataNode, 'image');
            addMenuItem(tabContent, {"cmd": "addWebRadiodbToHome", "options": [uri, type, name, image]}, 'Add to homescreen');
        }
        else {
            addMenuItem(tabContent, {"cmd": "addSongToHome", "options": [uri, type, name]}, 'Add to homescreen');
        }
    }
    if (app.id === 'BrowseRadioRadiobrowser' &&
        dataNode !== null)
    {
        const uuid = getData(dataNode, 'RADIOBROWSERUUID');
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "showRadiobrowserDetails", "options": [uuid]}, 'Webradio details');
        addMenuItem(tabContent, {"cmd": "showEditRadioFavorite", "options": [{
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
    if (app.id === 'BrowseRadioWebradiodb' &&
        dataNode !== null)
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "showWebradiodbDetails", "options": [uri]}, 'Webradio details');
        addMenuItem(tabContent, {"cmd": "showEditRadioFavorite", "options": [{
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
    if (app.id === 'QueueCurrent' &&
        type === 'webradio')
    {
        addDivider(tabContent);
        const webradioUri = getData(dataNode, 'webradioUri');
        addMenuItem(tabContent, {"cmd": "editRadioFavorite", "options": [webradioUri]}, 'Edit webradio favorite');
    }
}

/**
 * Appends search actions to the popover
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {string} expression search expression
 */
function addMenuItemsSearchActions(tabContent, expression) {
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["search", expression]}, 'Append to queue');
    addMenuItem(tabContent, {"cmd": "appendPlayQueue", "options": ["search", expression]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertAfterCurrentQueue", "options": ["search", expression, 0, 1, false]}, 'Insert after current playing song');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": ["search", expression]}, 'Replace queue');
    addMenuItem(tabContent, {"cmd": "replacePlayQueue", "options": ["search", expression]}, 'Replace queue and play');
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": ["SEARCH", expression]}, 'Add to playlist');
    }
    addDivider(tabContent);
    addMenuItem(tabContent, {"cmd": "appGoto", "options": ["Search", undefined, undefined, 0, undefined, "any", "Title", "-", expression]}, 'Show search');
}

/**
 * Appends directory actions to the popover
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {string} baseuri directory
 */
function addMenuItemsDirectoryActions(tabContent, baseuri) {
    //songs must be arranged in one album per folder
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": ["dir", baseuri]}, 'Append to queue');
    addMenuItem(tabContent, {"cmd": "appendPlayQueue", "options": ["dir", baseuri]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertAfterCurrentQueue", "options": ["dir", baseuri, 0, 1, false]}, 'Insert after current playing song');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": ["dir", baseuri]}, 'Replace queue');
    addMenuItem(tabContent, {"cmd": "replacePlayQueue", "options": ["dir", baseuri]}, 'Replace queue and play');
    if (features.featPlaylists === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "showAddToPlaylist", "options": [baseuri, ""]}, 'Add to playlist');
    }
    if (app.id === 'BrowseFilesystem') {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "updateDB", "options": [baseuri, false, false, false]}, 'Update directory');
        addMenuItem(tabContent, {"cmd": "updateDB", "options": [baseuri, false, false, true]}, 'Rescan directory');
    }
    addDivider(tabContent);
    addMenuItem(tabContent, {"cmd": "gotoFilesystem", "options": [baseuri, "dir"]}, 'Open directory');
    if (features.featHome === true &&
        app.id !== 'Home')
    {
        addDivider(tabContent);
        addMenuItem(tabContent, {"cmd": "addDirToHome", "options": [baseuri, baseuri]}, 'Add to homescreen');
    }
}

/**
 * Appends actions for webradio favorites
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {HTMLElement} dataNode element with the data
 */
function addMenuItemsWebradioFavoritesActions(tabContent, dataNode) {
    const type = getData(dataNode, 'type');
    const uri = getData(dataNode, 'uri');
    const plistUri = getRadioFavoriteUri(uri);
    const name = getData(dataNode, 'name');
    addMenuItemsPlaylistActions(tabContent, dataNode, type, plistUri, name);
    addDivider(tabContent);
    addMenuItem(tabContent, {"cmd": "editRadioFavorite", "options": [uri]}, 'Edit webradio favorite');
    addMenuItem(tabContent, {"cmd": "deleteRadioFavorite", "options": [uri]}, 'Delete webradio favorite');
}

/**
 * Appends actions for webradio favorites home icon
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {string} uri webradio favorite uri
 */
function addMenuItemsWebradioFavoritesHomeActions(tabContent, uri) {
    addDivider(tabContent);
    addMenuItem(tabContent, {"cmd": "editRadioFavorite", "options": [uri]}, 'Edit webradio favorite');
}

/**
 * Appends actions for playlists to the popover
 * @param {HTMLElement} tabContent element to append the menu items
 * @param {HTMLElement | EventTarget} dataNode element with the data
 * @param {string} type playlist type: plist, smartpls
 * @param {string} uri playlist uri
 * @param {string} name playlist name
 */
function addMenuItemsPlaylistActions(tabContent, dataNode, type, uri, name) {
    addMenuItem(tabContent, {"cmd": "appendQueue", "options": [type, uri]}, 'Append to queue');
    addMenuItem(tabContent, {"cmd": "appendPlayQueue", "options": [type, uri]}, 'Append to queue and play');
    if (features.featWhence === true) {
        addMenuItem(tabContent, {"cmd": "insertAfterCurrentQueue", "options": [type, uri, 0, 1, false]}, 'Add after current playing song');
    }
    addMenuItem(tabContent, {"cmd": "replaceQueue", "options": [type, uri]}, 'Replace queue');
    addMenuItem(tabContent, {"cmd": "replacePlayQueue", "options": [type, uri]}, 'Replace queue and play');
    if (features.featHome === true) {
        if (app.id !== 'Home') {
            addDivider(tabContent);
            if (app.id === 'BrowseRadioFavorites') {
                let image = getData(dataNode, 'image');
                if (isHttpUri(image) === false)
                {
                    image = basename(image, false);
                }
                addMenuItem(tabContent, {"cmd": "addRadioFavoriteToHome", "options": [uri, type, name, image]}, 'Add to homescreen');
            }
            else {
                addMenuItem(tabContent, {"cmd": "addPlistToHome", "options": [uri, type, name]}, 'Add to homescreen');
            }
        }
    }
    if (app.id !== 'BrowsePlaylistsList') {
        if (type === 'plist' ||
            type === 'smartpls')
        {
            if (isMPDplaylist(uri) === true) {
                addDivider(tabContent);
                addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
            }
            else {
                addMenuItem(tabContent, {"cmd": "gotoFilesystem", "options": [uri, "plist"]}, 'View playlist');
            }
        }
    }
}

/**
 * Creates the first tab content for list popovers
 * @param {EventTarget} el triggering element
 * @param {HTMLElement} tabHeader tab header element
 * @param {HTMLElement} tabContent tab content element
 * @returns {boolean} true on success, else false
 */
function createMenuLists(el, tabHeader, tabContent) {
    const dataNode = el.parentNode.parentNode;
    const type = getData(dataNode, 'type');
    const uri = getData(dataNode, 'uri');
    const name = getData(dataNode, 'name');

    tabHeader.textContent = tn(typeFriendly[type]);

    switch(app.id) {
        case 'BrowseFilesystem':
        case 'Search':
        case 'BrowseRadioRadiobrowser':
        case 'BrowseRadioWebradiodb':
        case 'BrowseDatabaseDetail': {
            switch(type) {
                case 'song':
                case 'stream':
                    addMenuItemsSongActions(tabContent, dataNode, uri, type, name);
                    break;
                case 'dir':
                    addMenuItemsDirectoryActions(tabContent, uri);
                    break;
                case 'plist':
                case 'smartpls':
                    addMenuItemsPlaylistActions(tabContent, dataNode, type, uri, name);
                    break;
                default:
                    return false;
            }
            return true;
        }
        case 'BrowsePlaylistsList': {
            const smartplsOnly = getData(dataNode, 'smartpls-only');
            if (smartplsOnly === false ||
                type !== 'smartpls')
            {
                addMenuItemsPlaylistActions(tabContent, dataNode, type, uri, name);
                addDivider(tabContent);
                if (settings.smartpls === true && type === 'smartpls') {
                    addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'View playlist');
                }
                else {
                    addMenuItem(tabContent, {"cmd": "playlistDetails", "options": [uri]}, 'Edit playlist');
                }
                addMenuItem(tabContent, {"cmd": "showRenamePlaylist", "options": [uri]}, 'Rename playlist');
            }
            addMenuItem(tabContent, {"cmd": "showDelPlaylist", "options": [uri, smartplsOnly]}, 'Delete playlist');
            if (settings.smartpls === true &&
                type === 'smartpls')
            {
                addDivider(tabContent);
                addMenuItem(tabContent, {"cmd": "showSmartPlaylist", "options": [uri]}, 'Edit smart playlist');
                addMenuItem(tabContent, {"cmd": "updateSmartPlaylist", "options": [uri]}, 'Update smart playlist');
            }
            return true;
        }
        case 'BrowsePlaylistsDetail': {
            const table = document.getElementById('BrowsePlaylistsDetailList');
            addMenuItemsSongActions(tabContent, dataNode, uri, type, name);
            if (getData(table, 'ro') === 'false') {
                addDivider(tabContent);
                const plist = getData(table, 'uri');
                const songpos = getData(dataNode, 'songpos');
                const playlistLength = getData(table, 'playlistlength');
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
            const songid = getData(dataNode, 'songid');
            const songpos = getData(dataNode, 'songpos');
            addMenuItemsSongActions(tabContent, dataNode, uri, type, name);
            addDivider(tabContent);
            if (currentState.currentSongId !== -1 &&
                songid !== currentState.currentSongId)
            {
                addMenuItem(tabContent, {"cmd": "playAfterCurrent", "options": [songid, songpos]}, 'Play after current playing song');
            }
            addMenuItem(tabContent, {"cmd": "showSetSongPriority", "options": [songid]}, 'Set priority');
            if (songid === currentState.currentSongId) {
                addMenuItemsSingleActions(tabContent);
            }
            addDivider(tabContent);
            addMenuItem(tabContent, {"cmd": "removeFromQueue", "options": ["single", songid]}, 'Remove');
            if (songpos > 0) {
                addMenuItem(tabContent, {"cmd": "removeFromQueue", "options": ["range", 0, songpos]}, 'Remove all upwards');
            }
            if (songpos + 1 < currentState.queueLength) {
                addMenuItem(tabContent, {"cmd": "removeFromQueue", "options": ["range", songpos + 1, -1]}, 'Remove all downwards');
            }
            return true;
        }
        case 'QueueLastPlayed': {
            addMenuItemsSongActions(tabContent, dataNode, uri, type, name);
            return true;
        }
        case 'QueueJukebox': {
            const pos = Number(getData(dataNode, 'pos'));
            if (settings.partition.jukeboxMode === 'song') {
                addMenuItemsSongActions(tabContent, dataNode, uri, type, name);
            }
            else if (settings.partition.jukeboxMode === 'album') {
                addMenuItemsAlbumActions(tabContent, dataNode)
            }
            addDivider(tabContent);
            addMenuItem(tabContent, {"cmd": "delQueueJukeboxSong", "options": [pos]}, 'Remove');
            return true;
        }
    }
    return false;
}

/**
 * Creates the secondary tab content for list popovers
 * @param {EventTarget} el triggering element
 * @param {HTMLElement} tabHeader tab header element
 * @param {HTMLElement} tabContent tab content element
 * @returns {boolean} true on success, else false
 */
function createMenuListsSecondary(el, tabHeader, tabContent) {
    switch(app.id) {
        case 'Search':
        case 'QueueCurrent':
        case 'QueueLastPlayed':
        case 'QueueJukebox':
        case 'BrowseFilesystem':
        case 'BrowseDatabaseAlbumDetail':
        case 'BrowsePlaylistsDetail': {
            const dataNode = el.parentNode.parentNode;
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
            const album = getData(dataNode, 'Album');
            const albumArtist = getData(dataNode, 'AlbumArtist');
            if (album !== undefined &&
                albumArtist !== undefined &&
                album !== '-' && 
                checkTagValue(albumArtist, '-') === false)
            {
                tabHeader.textContent = tn('Album');
                addMenuItemsAlbumActions(tabContent, dataNode);
            }
            else {
                tabHeader.textContent = tn('Directory');
                const baseuri = dirname(uri);
                addMenuItemsDirectoryActions(tabContent, baseuri);
            }
            return true;
        }
    }
    return false;
}

/**
 * Creates the content of the first home popover tab
 * @param {EventTarget} dataNode triggering element
 * @param {HTMLElement} tabHeader tab header element
 * @param {HTMLElement} tabContent tab content element
 * @returns {boolean} true on success, else false
 */
function createMenuHome(dataNode, tabHeader, tabContent) {
    const pos = getData(dataNode, 'pos');
    const href = getData(dataNode, 'href');
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
    tabHeader.textContent = tn(typeFriendly[type]);
    switch(type) {
        case 'plist':
        case 'smartpls':
            addMenuItemsPlaylistActions(tabContent, dataNode, type, href.options[1], href.options[1]);
            break;
        case 'webradio':
            addMenuItemsPlaylistActions(tabContent, dataNode, type, href.options[1], href.options[1]);
            addMenuItemsWebradioFavoritesHomeActions(tabContent, href.options[1].substr(17));
            break;
        case 'dir':
            addMenuItemsDirectoryActions(tabContent, href.options[1]);
            break;
        case 'song':
        case 'stream':
            addMenuItemsSongActions(tabContent, null, href.options[1], type, href.options[1]);
            break;
        case 'search':
            addMenuItemsSearchActions(tabContent, href.options[1]);
            break;
        case 'album':
            addMenuItemsAlbumActions(tabContent, null, href.options[1], href.options[2]);
            break;
        case 'view':
        case 'script':
        case 'externalLink':
        case 'modal':
            addMenuItem(tabContent, {"cmd": "executeHomeIcon", "options": [pos]}, actionDesc);
    }
    return true;
}

/**
 * Creates the content of the second home popover tab
 * @param {EventTarget} el triggering element
 * @param {HTMLElement} tabHeader tab header element
 * @param {HTMLElement} tabContent tab content element
 * @returns {boolean} true on success, else false
 */
function createMenuHomeSecondary(el, tabHeader, tabContent) {
    const pos = getData(el, 'pos');
    tabHeader.textContent = tn('Homeicon');
    addMenuItem(tabContent, {"cmd": "editHomeIcon", "options": [pos]}, 'Edit home icon');
    addMenuItem(tabContent, {"cmd": "duplicateHomeIcon", "options": [pos]}, 'Duplicate home icon');
    addMenuItem(tabContent, {"cmd": "deleteHomeIcon", "options": [pos]}, 'Delete home icon');
    return true;
}
