"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module settings_js */

/**
 * Fetches all myMPD and MPD settings
 * @returns {void}
 */
function getSettings() {
    settingsParsed = 'no';
    sendAPI('MYMPD_API_SETTINGS_GET', {}, parseSettings, true);
}

/**
 * Parses the MYMPD_API_SETTINGS_GET jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseSettings(obj) {
    if (obj.error) {
        settingsParsed = 'error';
        if (appInited === false) {
            showAppInitAlert(obj.error.message === undefined ? tn('Can not parse settings') : tn(obj.error.message));
        }
        return;
    }
    settings = obj.result;

    //check for old cached javascript
    if ('serviceWorker' in navigator && settings.mympdVersion !== myMPDversion) {
        logWarn('Server version (' + settings.mympdVersion + ') not equal client version (' + myMPDversion + '), reloading');
        clearAndReload();
    }

    //set webuiSettings defaults
    for (const key in settingsWebuiFields) {
        if (settings.webuiSettings[key] === undefined &&
            settingsWebuiFields[key].defaultValue !== undefined)
        {
            settings.webuiSettings[key] = settingsWebuiFields[key].defaultValue;
        }
    }

    //set session state
    setSessionState();

    //set features object from settings
    setFeatures();

    //execute only if settings modal is displayed
    if (document.getElementById('modalSettings').classList.contains('show')) {
        createSettingsFrm();
        populateSettingsFrm();
    }
    //execute only if connection modal is displayed
    if (document.getElementById('modalSettingsConnection').classList.contains('show')) {
        populateConnectionFrm();
    }
    //execute only if queue settings modal is displayed
    if (document.getElementById('modalSettingsPlayback').classList.contains('show')) {
        populateSettingsPlaybackFrm();
    }

    //locales
    setLocale(settings.webuiSettings.locale);

    //theme
    let setTheme = settings.webuiSettings.theme;
    switch(setTheme) {
        case 'theme-autodetect': setTheme = 'auto'; break;
        case 'theme-light': setTheme = 'light'; break;
        case 'theme-dark': setTheme = 'dark'; break;
    }
    if (setTheme === 'auto') {
        setTheme = window.matchMedia &&
            window.matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark';
    }
    document.querySelector('html').setAttribute('data-bs-theme', setTheme);

    //compact grids
    if (settings.webuiSettings.compactGrids === true) {
        document.documentElement.style.setProperty('--mympd-card-footer-word-wrap', 'nowrap');
    }
    else {
        document.documentElement.style.setProperty('--mympd-card-footer-word-wrap', 'unset');
    }

    // toggle help
    if (settings.webuiSettings.showHelp === true) {
        document.documentElement.style.setProperty('--mympd-show-help', 'block');
    }
    else {
        document.documentElement.style.setProperty('--mympd-show-help', 'none');
    }

    //background
    if (settings.webuiSettings.theme === 'auto') {
        //in auto mode we set default background
        domCache.body.style.backgroundImage = '';
        document.documentElement.style.setProperty('--mympd-backgroundcolor',
            (setTheme === 'dark' ? '#060708' : '#ffffff')
        );
    }
    else {
        if (settings.webuiSettings.bgImage.indexOf('/assets/') === 0) {
            domCache.body.style.backgroundImage = 'url("' + subdir + myEncodeURI(settings.webuiSettings.bgImage) + '")';
        }
        else if (settings.webuiSettings.bgImage !== '') {
            domCache.body.style.backgroundImage = 'url("' + subdir + '/browse/pics/backgrounds/' + myEncodeURI(settings.webuiSettings.bgImage) + '")';
        }
        else {
            domCache.body.style.backgroundImage = '';
        }
        document.documentElement.style.setProperty('--mympd-backgroundcolor', settings.webuiSettings.bgColor);
    }

    const albumartbg = document.querySelectorAll('body > div.albumartbg');
    for (let i = 0, j = albumartbg.length; i < j; i++) {
        albumartbg[i].style.filter = settings.webuiSettings.bgCssFilter;
    }

    //Navigation and footer
    setNavbarIcons();

    //Mobile view
    setMobileView();
    setUserAgentData();
    setViewport();

    if (settings.webuiSettings.footerSettingsPlayback === true) {
        elShowId('footerSettingsPlayback');
    }
    else {
        elHideId('footerSettingsPlayback');
    }

    if (settings.webuiSettings.footerPlaybackControls === 'both') {
        elShowId('footerStopBtn');
    }
    else {
        elHideId('footerStopBtn');
    }
    
    if (settings.partition.jukeboxMode !== 'off') {
        document.getElementById('footerNextBtn').removeAttribute('disabled');
    }

    //presets
    populatePresetDropdowns();

    //parse mpd settings if connected
    if (settings.partition.mpdConnected === true) {
        parseMPDSettings();
    }

    //Info in about modal
    document.getElementById('mpdInfoHost').textContent = settings.mpdHost.indexOf('/') !== 0 ?
        settings.mpdHost + ':' + settings.mpdPort : settings.mpdHost;

    document.documentElement.style.setProperty('--mympd-thumbnail-size', settings.webuiSettings.thumbnailSize + "px");
    document.documentElement.style.setProperty('--mympd-highlightcolor', settings.partition.highlightColor);
    document.documentElement.style.setProperty('--mympd-highlightcolor-contrast', settings.partition.highlightColorContrast);

    //default limit for all cards
    let limit = settings.webuiSettings.maxElementsPerPage;
    if (limit === 0) {
        limit = 500;
    }
    app.cards.Home.limit = limit;
    app.cards.Playback.limit = limit;
    app.cards.Queue.tabs.Current.limit = limit;
    app.cards.Queue.tabs.LastPlayed.limit = limit;
    app.cards.Queue.tabs.Jukebox.limit = limit;
    app.cards.Browse.tabs.Filesystem.limit = limit;
    app.cards.Browse.tabs.Playlist.views.List.limit = limit;
    app.cards.Browse.tabs.Playlist.views.Detail.limit = limit;
    app.cards.Browse.tabs.Database.views.TagList.limit = limit;
    app.cards.Browse.tabs.Database.views.AlbumList.limit = limit;
    app.cards.Browse.tabs.Database.views.AlbumDetail.limit = limit;
    app.cards.Search.limit = limit;

    //scripts
    if (scriptsInited === false) {
        const selectTimerAction = document.getElementById('selectTimerAction');
        elClearId('selectTimerAction');
        selectTimerAction.appendChild(
            elCreateNodes('optgroup', {"data-value": "player", "data-label-phrase": "Playback", "label": tn('Playback')}, [
                elCreateTextTn('option', {"value": "startplay"}, 'Start playback'),
                elCreateTextTn('option', {"value": "stopplay"}, 'Stop playback')
            ])
        );

        if (features.featScripting === true) {
            getScriptList(true);
        }
        else {
            elClearId('scripts');
        }
        scriptsInited = true;
    }

    //volumebar
    document.getElementById('volumeBar').setAttribute('min', settings.volumeMin);
    document.getElementById('volumeBar').setAttribute('max', settings.volumeMax);

    //set translations for pregenerated elements
    pEl.actionTdMenu.firstChild.title = tn('Actions');

    pEl.actionTdMenuPlay.firstChild.title = tn(settingsWebuiFields.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay]);
    pEl.actionTdMenuPlay.lastChild.title = tn('Actions');

    pEl.actionTdMenuRemove.firstChild.title = tn('Remove');
    pEl.actionTdMenuRemove.lastChild.title = tn('Actions');

    pEl.actionTdMenuPlayRemove.childNodes[0].title = tn(settingsWebuiFields.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay]);
    pEl.actionTdMenuPlayRemove.childNodes[1].title = tn('Remove');
    pEl.actionTdMenuPlayRemove.childNodes[2].title = tn('Actions');

    //update actions for table rows
    if (settings.webuiSettings.quickPlayButton === true &&
        settings.webuiSettings.quickRemoveButton === true)
    {
        pEl.actionTd = pEl.actionTdMenuPlay;
        pEl.actionQueueTd = pEl.actionTdMenuRemove;
        pEl.actionJukeboxTd = pEl.actionTdMenuPlayRemove;
        pEl.actionPlaylistDetailTd = pEl.actionTdMenuPlayRemove;
        pEl.actionPlaylistTd = pEl.actionTdMenuPlayRemove;
    }
    else if (settings.webuiSettings.quickPlayButton === true) {
        pEl.actionTd = pEl.actionTdMenuPlay;
        pEl.actionQueueTd = pEl.actionTdMenu;
        pEl.actionJukeboxTd = pEl.actionTdMenuPlay;
        pEl.actionPlaylistDetailTd = pEl.actionTdMenuPlay;
        pEl.actionPlaylistTd = pEl.actionTdMenuPlay;
    }
    else if (settings.webuiSettings.quickRemoveButton === true) {
        pEl.actionTd = pEl.actionTdMenu;
        pEl.actionQueueTd = pEl.actionTdMenuRemove;
        pEl.actionJukeboxTd = pEl.actionTdMenuRemove;
        pEl.actionPlaylistDetailTd = pEl.actionTdMenuRemove;
        pEl.actionPlaylistTd = pEl.actionTdMenuRemove;
    }
    else {
        pEl.actionTd = pEl.actionTdMenu;
        pEl.actionQueueTd = pEl.actionTdMenu;
        pEl.actionJukeboxTd = pEl.actionTdMenu;
        pEl.actionPlaylistDetailTd = pEl.actionTdMenu;
        pEl.actionPlaylistTd = pEl.actionTdMenu;
    }

    pEl.coverPlayBtn.title = tn(settingsWebuiFields.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay]);

    //goto view
    if (app.id === 'QueueJukeboxSong' ||
        app.id === 'QueueJukeboxAlbum')
    {
        gotoJukebox();
    }
    else {
        appRoute();
    }

    //mediaSession support
    if (features.featMediaSession === true) {
        try {
            navigator.mediaSession.setActionHandler('play', clickPlay);
            navigator.mediaSession.setActionHandler('pause', clickPlay);
            navigator.mediaSession.setActionHandler('stop', clickStop);
            navigator.mediaSession.setActionHandler('seekbackward', seekRelativeBackward);
            navigator.mediaSession.setActionHandler('seekforward', seekRelativeForward);
            navigator.mediaSession.setActionHandler('previoustrack', clickPrev);
            navigator.mediaSession.setActionHandler('nexttrack', clickNext);
        }
        catch(err) {
            logWarn('mediaSession.setActionHandler not supported by browser: ' + err.message);
        }
        if (!navigator.mediaSession.setPositionState) {
            logDebug('mediaSession.setPositionState not supported by browser');
        }
    }
    else {
        logDebug('mediaSession not supported by browser or not enabled');
    }

    //finished parse setting, set ui state
    toggleUI();
    btnWaitingId('modalSettingsApplyBtn', false);
    applyFeatures();
    settingsParsed = 'parsed';
    myMPDready = true;
}

/**
 * Parses the MPD options
 * @returns {void}
 */
function parseMPDSettings() {
    document.getElementById('partitionName').textContent = localSettings.partition;

    if (settings.webuiSettings.BgCover === true) {
        setBackgroundImage(domCache.body, currentSongObj.uri);
    }
    else {
        clearBackgroundImage(domCache.body);
    }

    const triggerEventList = document.getElementById('selectTriggerEvent');
    elClear(triggerEventList);
    for (const event in settings.triggerEvents) {
        triggerEventList.appendChild(
            elCreateTextTn('option', {"value": settings.triggerEvents[event]}, event)
        );
    }

    settings.tagList.sort();
    settings.tagListSearch.sort();
    settings.tagListBrowse.sort();

    filterCols('Playback');

    for (const table of ['Search', 'QueueCurrent', 'QueueLastPlayed',
            'QueueJukeboxSong', 'QueueJukeboxAlbum',
            'BrowsePlaylistDetail', 'BrowseFilesystem', 'BrowseDatabaseAlbumDetail'])
    {
        filterCols(table);
        setCols(table);
        //add all browse tags (advanced action in popover menu)
        const col = 'cols' + table + 'Fetch';
        settings[col] = settings['cols' + table].slice();
        for (const tag of settings.tagListBrowse) {
            if (settings[col].includes(tag) === false) {
                settings[col].push(tag);
            }
        }
    }
    setCols('BrowseRadioWebradiodb');
    setCols('BrowseRadioRadiobrowser');

    //tagselect dropdowns
    for (const table of ['BrowseDatabaseAlbumList', 'BrowseDatabaseAlbumDetailInfo']) {
        filterCols(table);
        const menu = document.querySelector('#' + table + 'ColsDropdown > form');
        elClear(menu);
        setColsChecklist(table, menu);
    }

    //enforce album and albumartist for album list view
    settings['colsBrowseDatabaseAlbumListFetch'] = settings['colsBrowseDatabaseAlbumList'].slice();
    if (settings.colsBrowseDatabaseAlbumListFetch.includes('Album') === false) {
        settings.colsBrowseDatabaseAlbumListFetch.push('Album');
    }
    if (settings.colsBrowseDatabaseAlbumListFetch.includes(tagAlbumArtist) === false) {
        settings.colsBrowseDatabaseAlbumListFetch.push(tagAlbumArtist);
    }

    //enforce disc for album detail list view
    if (settings.colsBrowseDatabaseAlbumDetailFetch.includes('Disc') === false &&
        settings.tagList.includes('Disc'))
    {
        settings.colsBrowseDatabaseAlbumDetailFetch.push('Disc');
    }

    if (features.featTags === false) {
        app.cards.Browse.active = 'Filesystem';
        app.cards.Search.sort.tag = 'filename';
        app.cards.Search.filter = 'filename';
        app.cards.Queue.tabs.Current.filter = 'filename';
        settings.colsQueueCurrent = ["Pos", "Title", "Duration"];
        settings.colsQueueLastPlayed = ["Pos", "Title", "LastPlayed"];
        settings.colsQueueJukeboxSong = ["Pos", "Title"];
        settings.colsQueueJukeboxAlbum = ["Pos", "Title"];
        settings.colsSearch = ["Title", "Duration"];
        settings.colsBrowseFilesystem = ["Type", "Title", "Duration"];
        settings.colsPlayback = [];
    }
    else {
        //construct playback view
        const pbtl = document.getElementById('PlaybackListTags');
        elClear(pbtl);
        for (let i = 0, j = settings.colsPlayback.length; i < j; i++) {
            let colWidth;
            switch(settings.colsPlayback[i]) {
                case 'Lyrics':
                    colWidth = "col-xl-12";
                    break;
                default:
                    colWidth = "col-xl-6";
            }
            const div = elCreateNodes('div', {"id": "current" + settings.colsPlayback[i],"class": [colWidth]}, [
                elCreateTextTn('small', {}, settings.colsPlayback[i]),
                elCreateEmpty('p', {})
            ]);
            setData(div, 'tag', settings.colsPlayback[i]);
            pbtl.appendChild(div);
        }
        //musicbrainz
        pbtl.appendChild(
            elCreateEmpty('div', {"id": "currentMusicbrainz", "class": ["col-xl-6"]})
        );
        //fill blank card with currentSongObj
        if (currentSongObj !== null) {
            setPlaybackCardTags(currentSongObj);
        }
        //tagselect dropdown
        const menu = document.querySelector('#PlaybackColsDropdown > form');
        elClear(menu);
        setColsChecklist('Playback', menu);
    }

    if (settings.tagList.includes('Title')) {
        app.cards.Search.sort.tag = 'Title';
    }

    //fallback from AlbumArtist to Artist
    if (settings.tagList.includes('AlbumArtist')) {
        tagAlbumArtist = 'AlbumArtist';
    }
    else if (settings.tagList.includes('Artist')) {
        tagAlbumArtist = 'Artist';
        if (app.cards.Browse.tabs.Database.views.AlbumList.filter === 'AlbumArtist') {
            app.cards.Browse.tabs.Database.views.AlbumList.filter = tagAlbumArtist;
        }
        if (app.cards.Browse.tabs.Database.views.AlbumList.sort.tag === 'AlbumArtist') {
            app.cards.Browse.tabs.Database.views.AlbumList.sort.tag = tagAlbumArtist;
        }
    }

    addTagList('BrowseDatabaseAlbumListTagDropdown', 'tagListBrowse');
    addTagList('BrowseDatabaseTagListTagDropdown', 'tagListBrowse');
    addTagList('BrowsePlaylistListNavDropdown', 'tagListBrowse');
    addTagList('BrowseFilesystemNavDropdown', 'tagListBrowse');
    addTagList('BrowseRadioFavoritesNavDropdown', 'tagListBrowse');
    addTagList('BrowseRadioWebradiodbNavDropdown', 'tagListBrowse');
    addTagList('BrowseRadioRadiobrowserNavDropdown', 'tagListBrowse');

    addTagList('QueueCurrentSearchTags', 'tagListSearch');
    addTagList('QueueLastPlayedSearchTags', 'tagListSearch');
    addTagList('QueueJukeboxSongSearchTags', 'tagListSearch');
    addTagList('QueueJukeboxAlbumSearchTags', 'tagListSearch');
    addTagList('BrowsePlaylistDetailSearchTags', 'tagListSearch');
    addTagList('SearchSearchTags', 'tagListSearch');
    addTagList('BrowseDatabaseAlbumListSearchTags', 'tagListBrowse');
    addTagList('BrowseDatabaseAlbumListSortTagsList', 'tagListBrowse');
    addTagList('BrowsePlaylistDetailSortTagsDropdown', 'tagList');

    addTagListSelect('saveSmartPlaylistSort', 'tagList');
}

/**
 * Populates the navbar with the icons
 * @returns {void}
 */
function setNavbarIcons() {
    const oldBadgeQueueItems = document.getElementById('badgeQueueItems');
    let oldQueueLength = 0;
    if (oldBadgeQueueItems) {
        oldQueueLength = Number(oldBadgeQueueItems.textContent);
    }

    const container = document.getElementById('navbar-main');
    elClear(container);

    if (settings.webuiSettings.showBackButton === true) {
        container.appendChild(
            elCreateNode('div', {"class": ["nav-item", "flex-fill", "text-center"]},
                elCreateNode('a', {"data-title-phrase": "History back", "title": tn("History back"), "href": "#", "class": ["nav-link"]},
                    elCreateText('span', {"class": ["mi"]}, "arrow_back_ios")
                )
            )
        );
        setData(container.firstElementChild.firstElementChild, 'href', {"cmd": "historyBack", "options": []});
    }

    for (const icon of settings.navbarIcons) {
        const id = "nav" + icon.options.join('');
        const btn = elCreateEmpty('div', {"id": id, "class": ["nav-item", "flex-fill", "text-center"]});
        if (id === 'nav' + app.current.card) {
            btn.classList.add('active');
        }
        if (features.featHome === false &&
            icon.options[0] === 'Home')
        {
            elHide(btn);
        }
        const a = elCreateNode('a', {"data-title-phrase": icon.title, "title": tn(icon.title), "href": "#", "class": ["nav-link"]},
            elCreateText('span', {"class": ["mi"]}, icon.ligature)
        );
        if (icon.options.length === 1 &&
            (icon.options[0] === 'Browse' ||
             icon.options[0] === 'Queue' ||
             icon.options[0] === 'Playback'))
        {
            a.setAttribute('data-contextmenu', 'Navbar' + icon.options.join(''));
        }
        if (icon.options[0] === 'Queue' &&
            icon.options.length === 1)
        {
            a.appendChild(
                elCreateText('span', {"id": "badgeQueueItems", "class": ["badge", "bg-secondary"]}, oldQueueLength.toString())
            );
        }
        btn.appendChild(a);
        container.appendChild(btn);
        setData(a, 'href', {"cmd": "appGoto", "options": icon.options});
    }

    //cache elements, reused in appPrepare
    domCache.navbarBtns = container.querySelectorAll('div');
    domCache.navbarBtnsLen = domCache.navbarBtns.length;
}

/**
 * Removes all settings from localStorage
 * @returns {void}
 */
function resetLocalSettings() {
    for (const key in localSettings) {
        localStorage.removeItem(key);
    }
}
