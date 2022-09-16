"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSettings() {
    document.getElementById('selectMusicDirectory').addEventListener('change', function () {
        const musicDirMode = getSelectValue(this);
        if (musicDirMode === 'auto') {
            document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else if (musicDirMode === 'none') {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
        }
        else {
            document.getElementById('inputMusicDirectory').value = '';
            document.getElementById('inputMusicDirectory').removeAttribute('readonly');
        }
    }, false);

    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettings');
        getSettings();
    });

    document.getElementById('modalMaintenance').addEventListener('shown.bs.modal', function () {
        document.getElementById('selectSetLoglevel').value = settings.loglevel;
        cleanupModalId('modalMaintenance');
    });

    document.getElementById('modalQueueSettings').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalQueueSettings');
        getSettings();
    });

    setDataId('selectJukeboxPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectJukeboxPlaylist', 'cb-filter-options', [0, 'selectJukeboxPlaylist']);

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        getSettings();
        cleanupModalId('modalConnection');
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            const value = getBtnGroupValueId('btnJukeboxModeGroup');
            if (value === 'off') {
                elDisableId('inputJukeboxQueueLength');
                elDisableId('selectJukeboxPlaylist');
            }
            else if (value === 'album') {
                elDisableId('inputJukeboxQueueLength');
                elDisableId('selectJukeboxPlaylist');
                elDisable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
                document.getElementById('selectJukeboxPlaylist').value = 'Database';
                setDataId('selectJukeboxPlaylist', 'value', 'Database');
            }
            else if (value === 'song') {
                elEnableId('inputJukeboxQueueLength');
                elEnableId('selectJukeboxPlaylist');
                elEnable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
            }
            if (value !== 'off') {
                toggleBtnChkId('btnConsume', true);
            }
            checkConsume();
        }, 100);
    });

    document.getElementById('btnConsume').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    initElements(document.getElementById('modalConnection'));
    initElements(document.getElementById('modalQueueSettings'));
    createSettingsFrm();
}

//eslint-disable-next-line no-unused-vars
function togglePlaymode(option) {
    let value;
    let title;
    switch(option) {
        case 'consume':
        case 'random':
        case 'repeat':
            if (settings.partition[option] === true) {
                value = false;
                title = 'Disable ' + option;
            }
            else {
                value = true;
                title = 'Enable ' + option;
            }
            break;
        case 'single':
            if (settings.partition.single === '0') {
                value = 'oneshot';
                title = 'Enable single oneshot';
            }
            else if (settings.partition.single === 'oneshot') {
                value = '1';
                title = 'Enable single mode';
            }
            else if (settings.partition.single === '1') {
                value = '0';
                title = 'Disable single mode';
            }
            break;
    }
    const params = {};
    params[option] = value;
    sendAPI('MYMPD_API_PLAYER_OPTIONS_SET', params);
    showNotification(tn(title), '', 'queue', 'info');
}

//eslint-disable-next-line no-unused-vars
function eventChangeLocale(event) {
    const value = getSelectValue(event.target);
    warnLocale(value);
}

//eslint-disable-next-line no-unused-vars
function eventChangeTheme(event) {
    const value = getSelectValue(event.target);
    const bgImageEl = document.getElementById('inputWebUIsettinguiBgImage');
    const bgImageValue = getData(bgImageEl, 'value');
    if (value === 'theme-light') {
        document.getElementById('inputWebUIsettinguiBgColor').value = '#ffffff';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = getBgImageText('/assets/mympd-background-light.svg');
            setData(bgImageEl, 'value', '/assets/mympd-background-light.svg');
        }
    }
    else {
        //theme-dark is the default
        document.getElementById('inputWebUIsettinguiBgColor').value = '#060708';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = getBgImageText('/assets/mympd-background-dark.svg');
            setData(bgImageEl, 'value', '/assets/mympd-background-dark.svg');
        }
    }
    toggleThemeInputs(value);
}

function toggleThemeInputs(theme) {
    if (theme === 'theme-autodetect') {
        document.getElementById('inputWebUIsettinguiBgColor').parentNode.parentNode.classList.add('d-none');
        document.getElementById('inputWebUIsettinguiBgImage').parentNode.parentNode.classList.add('d-none');
    }
    else {
        document.getElementById('inputWebUIsettinguiBgColor').parentNode.parentNode.classList.remove('d-none');
        document.getElementById('inputWebUIsettinguiBgImage').parentNode.parentNode.classList.remove('d-none');
    }
}

//eslint-disable-next-line no-unused-vars
function saveConnection() {
    cleanupModalId('modalConnection');
    let formOK = true;
    const mpdHostEl = document.getElementById('inputMpdHost');
    const mpdPortEl = document.getElementById('inputMpdPort');
    const mpdPassEl = document.getElementById('inputMpdPass');
    const playlistDirectoryEl = document.getElementById('inputPlaylistDirectory');
    const mpdBinarylimitEl = document.getElementById('inputMpdBinarylimit');
    const mpdTimeoutEl = document.getElementById('inputMpdTimeout');
    const musicDirectoryEl = document.getElementById('selectMusicDirectory');
    let musicDirectory = getSelectValue(musicDirectoryEl);

    if (musicDirectory === 'auto' && mpdHostEl.value.indexOf('/') !== 0) {
        formOK = false;
        setIsInvalid(musicDirectoryEl);
    }

    if (musicDirectory === 'custom') {
        const musicDirectoryValueEl = document.getElementById('inputMusicDirectory');
        if (validatePath(musicDirectoryValueEl) === false) {
            formOK = false;
        }
        musicDirectory = musicDirectoryValueEl.value;
    }
    if (mpdPortEl.value === '') {
        mpdPortEl.value = '6600';
    }
    if (validateIntRange(mpdPortEl, 1024, 65535) === false) {
        formOK = false;
    }
    if (mpdHostEl.value.indexOf('/') !== 0) {
        if (validateInt(mpdPortEl) === false) {
            formOK = false;
        }
        if (validateHost(mpdHostEl) === false) {
            formOK = false;
        }
    }
    if (validatePath(playlistDirectoryEl) === false) {
        formOK = false;
    }
    if (validateIntRange(mpdBinarylimitEl, 4, 256) === false) {
        formOK = false;
    }
    if (validateIntRange(mpdTimeoutEl, 1, 1000) === false) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPIpartition('default', 'MYMPD_API_CONNECTION_SAVE', {
            "mpdHost": mpdHostEl.value,
            "mpdPort": Number(mpdPortEl.value),
            "mpdPass": mpdPassEl.value,
            "musicDirectory": musicDirectory,
            "playlistDirectory": playlistDirectoryEl.value,
            "mpdBinarylimit": Number(mpdBinarylimitEl.value) * 1024,
            "mpdTimeout": Number(mpdTimeoutEl.value) * 1000,
            "mpdKeepalive": (document.getElementById('btnMpdKeepalive').classList.contains('active') ? true : false)
        }, saveConnectionClose, true);
    }
}

function saveConnectionClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings(false);
        uiElements.modalConnection.hide();
    }
}

function getSettings(onerror) {
    settingsParsed = 'no';
    sendAPI('MYMPD_API_SETTINGS_GET', {}, parseSettings, onerror);
}

function checkConsume() {
    const stateConsume = document.getElementById('btnConsume').classList.contains('active') ? true : false;
    const stateJukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    if (stateJukeboxMode !== 'off' && stateConsume === false) {
        elShowId('warnConsume');
    }
    else {
        elHideId('warnConsume');
    }
}

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
    for (const key in webuiSettingsDefault) {
        if (settings.webuiSettings[key] === undefined) {
            settings.webuiSettings[key] = webuiSettingsDefault[key].defaultValue;
        }
    }

    //set session state
    setSessionState();

    //set features object from settings
    setFeatures();

    //execute only if settings modal is displayed
    if (document.getElementById('modalSettings').classList.contains('show')) {
        populateSettingsFrm();
    }
    //execute only if connection modal is displayed
    if (document.getElementById('modalConnection').classList.contains('show')) {
        populateConnectionFrm();
    }
    //execute only if queue settings modal is displayed
    if (document.getElementById('modalQueueSettings').classList.contains('show')) {
        populateQueueSettingsFrm();
    }

    //locales
    setLocale(settings.webuiSettings.uiLocale);

    //theme
    let setTheme = settings.webuiSettings.uiTheme;
    if (setTheme === 'theme-default') {
        setTheme = 'theme-dark';
    }
    else if (setTheme === 'theme-autodetect') {
        setTheme = window.matchMedia &&
            window.matchMedia('(prefers-color-scheme: light)').matches ? 'theme-light' : 'theme-dark';
    }

    for (const theme in webuiSettingsDefault.uiTheme.validValues) {
        if (theme === setTheme) {
            domCache.body.classList.add(theme);
            setCssVars(theme);
        }
        else {
            domCache.body.classList.remove(theme);
        }
    }

    //background
    if (settings.webuiSettings.uiTheme === 'theme-autodetect') {
        //in auto mode we set default background
        domCache.body.style.backgroundImage = '';
        document.documentElement.style.setProperty('--mympd-backgroundcolor',
            (setTheme === 'theme-dark' ? '#060708' : '#ffffff')
        );
    }
    else {
        if (settings.webuiSettings.uiBgImage.indexOf('/assets/') === 0) {
            domCache.body.style.backgroundImage = 'url("' + subdir + myEncodeURI(settings.webuiSettings.uiBgImage) + '")';
        }
        else if (settings.webuiSettings.uiBgImage !== '') {
            domCache.body.style.backgroundImage = 'url("' + subdir + '/browse/pics/backgrounds/' + myEncodeURI(settings.webuiSettings.uiBgImage) + '")';
        }
        else {
            domCache.body.style.backgroundImage = '';
        }
        document.documentElement.style.setProperty('--mympd-backgroundcolor', settings.webuiSettings.uiBgColor);
    }

    const albumartbg = document.querySelectorAll('body > div.albumartbg');
    for (let i = 0, j = albumartbg.length; i < j; i++) {
        albumartbg[i].style.filter = settings.webuiSettings.uiBgCssFilter;
    }

    //Navigation and footer
    setNavbarIcons();

    //Mobile view
    setMobileView();

    if (settings.webuiSettings.uiFooterQueueSettings === true) {
        elShowId('footerQueueSettings');
    }
    else {
        elHideId('footerQueueSettings');
    }

    if (settings.webuiSettings.uiFooterPlaybackControls === 'both') {
        elShowId('btnStop');
    }
    else {
        elHideId('btnStop');
    }
    
    if (settings.partition.jukeboxMode !== 'off') {
        document.getElementById('btnNext').removeAttribute('disabled');
    }

    //parse mpd settings if connected
    if (settings.partition.mpdConnected === true) {
        parseMPDSettings();
    }

    //Info in about modal
    document.getElementById('mpdInfoHost').textContent = settings.mpdHost.indexOf('/') !== 0 ?
        settings.mpdHost + ':' + settings.mpdPort : settings.mpdHost;

    document.documentElement.style.setProperty('--mympd-thumbnail-size', settings.webuiSettings.uiThumbnailSize + "px");
    document.documentElement.style.setProperty('--mympd-highlightcolor', settings.partition.highlightColor);

    //default limit for all cards
    let limit = settings.webuiSettings.uiMaxElementsPerPage;
    if (limit === 0) {
        limit = 500;
    }
    app.cards.Home.limit = limit;
    app.cards.Playback.limit = limit;
    app.cards.Queue.tabs.Current.limit = limit;
    app.cards.Queue.tabs.LastPlayed.limit = limit;
    app.cards.Queue.tabs.Jukebox.limit = limit;
    app.cards.Browse.tabs.Filesystem.limit = limit;
    app.cards.Browse.tabs.Playlists.views.List.limit = limit;
    app.cards.Browse.tabs.Playlists.views.Detail.limit = limit;
    app.cards.Browse.tabs.Database.views.List.limit = limit;
    app.cards.Browse.tabs.Database.views.Detail.limit = limit;
    app.cards.Search.limit = limit;

    //scripts
    if (scriptsInited === false) {
        const selectTimerAction = document.getElementById('selectTimerAction');
        elClearId('selectTimerAction');
        selectTimerAction.appendChild(
            elCreateNodes('optgroup', {"data-value": "player", "label": tn('Playback')}, [
                elCreateText('option', {"value": "startplay"}, tn('Start playback')),
                elCreateText('option', {"value": "stopplay"}, tn('Stop playback'))
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

    //update columns and handle quick playback/remove buttons
    pEl.actionTdMenu.firstChild.title = tn('Actions');

    pEl.actionTdMenuPlay.firstChild.title = tn(webuiSettingsDefault.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay]);
    pEl.actionTdMenuPlay.lastChild.title = tn('Actions');

    pEl.actionTdMenuRemove.firstChild.title = tn('Remove');
    pEl.actionTdMenuRemove.lastChild.title = tn('Actions');

    pEl.actionTd = settings.webuiSettings.uiQuickPlayButton === false ? pEl.actionTdMenu : pEl.actionTdMenuPlay;
    pEl.actionQueueTd = settings.webuiSettings.uiQuickRemoveButton === false ? pEl.actionTdMenu : pEl.actionTdMenuRemove;
    pEl.coverPlayBtn.title = tn(webuiSettingsDefault.clickQuickPlay.validValues[settings.webuiSettings.clickQuickPlay]);

    appRoute();

    //mediaSession support
    if (checkMediaSessionSupport() === true) {
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
    btnWaiting(document.getElementById('btnApplySettings'), false);
    applyFeatures();
    settingsParsed = 'parsed';
}

function setCssVars(theme) {
    switch(theme) {
        case 'theme-light':
            document.documentElement.style.setProperty('--bs-body-color', '#343a40');
            document.documentElement.style.setProperty('--bs-body-color-rgb', '52, 58, 64');
            document.documentElement.style.setProperty('--bs-body-bg', 'white');
            document.documentElement.style.setProperty('--bs-dark-rgb', '206, 212, 218');
            document.documentElement.style.setProperty('--mympd-black-light', '#f8f9fa');
            break;
        default:
            document.documentElement.style.setProperty('--bs-body-color', '#f8f9fa');
            document.documentElement.style.setProperty('--bs-body-color-rgb', '248, 249, 250');
            document.documentElement.style.setProperty('--bs-body-bg', 'black');
            document.documentElement.style.setProperty('--bs-dark-rgb', '52, 58, 64');
            document.documentElement.style.setProperty('--mympd-black-light', '#1d2124');
    }
}

function setLocale(newLocale) {
    if (newLocale === 'default') {
        //auto detection
        locale = navigator.language || navigator.userLanguage;
        if (locale.length === 2) {
            locale += '-';
        }
    }
    else {
        locale = newLocale;
    }
    //check if locale is available
    let localeFound = false;
    for (const l in i18n) {
        if (l === 'default') {
            continue;
        }
        if (l.indexOf(locale) === 0) {
            locale = l;
            localeFound = true;
            break;
        }
    }
    //fallback to default locale
    if (localeFound === false) {
        logError('Locale "' + locale + '" not defined');
        locale = 'en-US';
    }
    //get phrases and translate dom
    httpGet(subdir + '/assets/i18n/' + locale + '.json', function(obj) {
        phrases = obj;
        i18nHtml(domCache.body);
    }, true);
}

function populateQueueSettingsFrm() {
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.partition.jukeboxMode);
    addTagListSelect('selectJukeboxUniqueTag', 'tagListBrowse');

    document.getElementById('selectJukeboxUniqueTag').value = settings.partition.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.partition.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.partition.jukeboxLastPlayed;
    if (settings.partition.jukeboxMode === 'off') {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
    }
    else if (settings.partition.jukeboxMode === 'album') {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
        elDisable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
    }
    else if (settings.partition.jukeboxMode === 'song') {
        elEnableId('inputJukeboxQueueLength');
        elEnableId('selectJukeboxPlaylist');
        elEnable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
    }

    document.getElementById('selectJukeboxPlaylist').filterInput.value = '';

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'selectJukeboxPlaylist', '', settings.partition.jukeboxPlaylist);
            setDataId('selectJukeboxPlaylist', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            document.getElementById('selectJukeboxPlaylist').value = tn('Database');
            setDataId('selectJukeboxPlaylist', 'value', 'Database');
        }
        toggleBtnChkId('btnRandom', settings.partition.random);
        toggleBtnChkId('btnConsume', settings.partition.consume);
        toggleBtnChkId('btnRepeat', settings.partition.repeat);
        toggleBtnChkId('btnAutoPlay', settings.partition.autoPlay);
        toggleBtnGroupValue(document.getElementById('btnSingleGroup'), settings.partition.single);
        toggleBtnGroupValue(document.getElementById('btnReplaygainGroup'), settings.partition.replaygain);
        document.getElementById('inputCrossfade').value = settings.partition.crossfade;
        document.getElementById('inputMixrampDb').value = settings.partition.mixrampDb;
        document.getElementById('inputMixrampDelay').value = settings.partition.mixrampDelay;
        if (features.featStickers === false) {
            elShowId('warnPlaybackStatistics');
            elDisableId('inputJukeboxLastPlayed');
        }
        else {
            elHideId('warnPlaybackStatistics');
            elEnableId('inputJukeboxLastPlayed');
        }
    }
    checkConsume();
}

function populateConnectionFrm() {
    document.getElementById('inputMpdHost').value = settings.mpdHost;
    document.getElementById('inputMpdPort').value = settings.mpdPort;
    document.getElementById('inputMpdPass').value = settings.mpdPass;
    document.getElementById('inputPlaylistDirectory').value = settings.playlistDirectory;
    document.getElementById('inputMpdBinarylimit').value = settings.mpdBinarylimit / 1024;
    document.getElementById('inputMpdTimeout').value = settings.mpdTimeout / 1000;

    toggleBtnChkId('btnMpdKeepalive', settings.mpdKeepalive);

    if (settings.musicDirectory === 'auto') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue !== undefined ? settings.musicDirectoryValue : '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else if (settings.musicDirectory === 'none') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = '';
        document.getElementById('inputMusicDirectory').setAttribute('readonly', 'readonly');
    }
    else {
        document.getElementById('selectMusicDirectory').value = 'custom';
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
        document.getElementById('inputMusicDirectory').removeAttribute('readonly');
    }

    if (settings.musicDirectoryValue === '' && settings.musicDirectory !== 'none') {
        elShowId('warnMusicDirectory');
    }
    else {
        elHideId('warnMusicDirectory');
    }
}

function getBgImageText(value) {
    if (value === '') {
        return 'None';
    }
    for (const key of bgImageValues) {
        if (key.value === value) {
            return key.text;
        }
    }
    return value;
}

function populateSettingsFrm() {
    getBgImageList(settings.webuiSettings.uiBgImage);
    const bgImageInput = document.getElementById('inputWebUIsettinguiBgImage');
    setData(bgImageInput, 'value', settings.webuiSettings.uiBgImage);
    bgImageInput.value = getBgImageText(settings.webuiSettings.uiBgImage);

    toggleThemeInputs(settings.webuiSettings.uiTheme);

    //partition specific settings
    document.getElementById('inputHighlightColor').value = settings.partition.highlightColor;
    document.getElementById('inputMpdStreamPort').value = settings.partition.mpdStreamPort;
    document.getElementById('inputStreamUri').value = settings.partition.streamUri;

    //locales
    const localeList = document.getElementById('inputWebUIsettinguiLocale');
    elClear(localeList);
    for (const l in i18n) {
        localeList.appendChild(
            elCreateText('option', {"value": l}, i18n[l].desc + ' (' + l + ')')
        );
        if (l === settings.webuiSettings.uiLocale) {
            localeList.lastChild.setAttribute('selected', 'selected');
        }
    }
    warnLocale(settings.webuiSettings.uiLocale);

    //web notifications - check permission
    const btnNotifyWeb = document.getElementById('inputWebUIsettingnotifyWeb');
    elHideId('warnNotifyWeb');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.webuiSettings.notifyWeb === true) {
                elShowId('warnNotifyWeb');
            }
            settings.webuiSettings.notifyWeb = false;
        }
        if (Notification.permission === 'denied') {
            elShowId('warnNotifyWeb');
        }
        toggleBtnChk(btnNotifyWeb, settings.webuiSettings.notifyWeb);
        elEnable(btnNotifyWeb);
    }
    else {
        elDisable(btnNotifyWeb);
        toggleBtnChk(btnNotifyWeb, false);
    }

    document.getElementById('inputScaleRatio').value = localSettings.scaleRatio;
    toggleBtnChkId('btnEnforceMobile', localSettings.enforceMobile);

    //media session support
    const btnMediaSession = document.getElementById('inputWebUIsettingmediaSession');
    if (checkMediaSessionSupport() === false) {
        elShowId('warninputWebUIsettingmediaSession');
        elDisable(btnMediaSession);
        toggleBtnChk(btnMediaSession, false);
    }
    else {
        elHideId('warninputWebUIsettingmediaSession');
        elEnable(btnMediaSession);
    }

    document.getElementById('inputBookletName').value = settings.bookletName;
    document.getElementById('inputCoverimageNames').value = settings.coverimageNames;
    document.getElementById('inputThumbnailNames').value = settings.thumbnailNames;
    document.getElementById('inputListenbrainzToken').value = settings.listenbrainzToken;

    //smart playlists
    if (settings.features.featPlaylists === true) {
        elEnableId('btnSmartpls');
        toggleBtnChkCollapseId('btnSmartpls', 'collapseSmartpls', settings.smartpls);
        elHideId('warnSmartpls');
    }
    else {
        elDisableId('btnSmartpls');
        toggleBtnChkCollapseId('btnSmartpls', 'collapseSmartpls', false);
        elShowId('warnSmartpls');
    }
    document.getElementById('inputSmartplsPrefix').value = settings.smartplsPrefix;
    document.getElementById('inputSmartplsInterval').value = settings.smartplsInterval / 60 / 60;
    addTagListSelect('selectSmartplsSort', 'tagList');
    document.getElementById('selectSmartplsSort').value = settings.smartplsSort;
    //lyrics
    if (features.featLibrary === false) {
        //lyrics need access to library
        settings.webuiSettings.enableLyrics = false;
    }
    toggleBtnChkCollapseId('btnEnableLyrics', 'collapseEnableLyrics', settings.webuiSettings.enableLyrics);

    //local playback
    toggleBtnChkCollapseId('btnEnableLocalPlayback', 'collapseEnableLocalPlayback', settings.webuiSettings.enableLocalPlayback);
    toggleBtnChkId('btnEnableLocalPlaybackAutoplay', localSettings.localPlaybackAutoplay);

    //albumart background css filter
    toggleBtnChkId('inputWebUIsettinguiBgCover', settings.webuiSettings.uiBgCover);

    //tag multiselects
    initTagMultiSelect('inputEnabledTags', 'listEnabledTags', settings.tagListMpd, settings.tagList);
    initTagMultiSelect('inputSearchTags', 'listSearchTags', settings.tagList, settings.tagListSearch);
    initTagMultiSelect('inputBrowseTags', 'listBrowseTags', settings.tagList, settings.tagListBrowse);
    initTagMultiSelect('inputGeneratePlsTags', 'listGeneratePlsTags', settings.tagListBrowse, settings.smartplsGenerateTagList);

    //features - show or hide warnings - use settings object
    setFeatureBtnId('btnEnableLyrics', settings.features.featLibrary);
    setFeatureBtnId('inputWebUIsettingenableScripting', settings.features.featScripting);
    setFeatureBtnId('inputWebUIsettingenableMounts', settings.features.featMounts);
    setFeatureBtnId('inputWebUIsettingenablePartitions', settings.features.featPartitions);
}

function setFeatureBtnId(id, value) {
    if (value === true) {
        elEnableId(id);
        elHideId('warn' + id);
    }
    else {
        elDisableId(id);
        toggleBtnChkId(id, false);
        elShowId('warn' + id);
    }
}

function createSettingsFrm() {
    _createSettingsFrm(settings.webuiSettings, webuiSettingsDefault, 'inputWebUIsetting');
    _createSettingsFrm(settings, settingFields, 'inputSetting');
    initElements(document.getElementById('modalSettings'));
}

function _createSettingsFrm(fields, defaults, prefix) {
    //build form for web ui settings
    const advFrm = {};
    const advSettingsKeys = Object.keys(defaults);
    advSettingsKeys.sort();
    for (let i = 0, j = advSettingsKeys.length; i < j; i++) {
        const key = advSettingsKeys[i];
        if (defaults[key] === undefined || defaults[key].form === undefined) {
            continue;
        }
        const form = defaults[key].form;
        if (advFrm[form] === undefined) {
            advFrm[form] = document.getElementById(form);
            elClear(advFrm[form]);
        }

        if (defaults[key].inputType === 'section') {
            if (defaults[key].title !== undefined) {
                advFrm[form].appendChild(elCreateEmpty('hr', {}));
                advFrm[form].appendChild(elCreateText('h4', {}, tn(defaults[key].title)));
            }
            else if (defaults[key].subtitle !== undefined) {
                advFrm[form].appendChild(elCreateText('h4', {}, tn(defaults[key].subtitle)));
            }
            continue;
        }

        const col = elCreateEmpty('div', {"class": ["col-sm-8", "position-relative"]});
        if (defaults[key].inputType === 'select') {
            const select = elCreateEmpty('select', {"class": ["form-select"], "id": prefix + r(key)});
            for (let value in defaults[key].validValues) {
                if (defaults[key].contentType === 'integer') {
                    value = Number(value);
                }
                select.appendChild(elCreateText('option', {"value": value}, tn(defaults[key].validValues[value])));
                if (fields[key] === value) {
                    select.lastChild.setAttribute('selected', 'selected');
                }
            }
            col.appendChild(select);
        }
        else if (defaults[key].inputType === 'mympd-select-search') {
            const input = elCreateEmpty('input', {"class": ["form-select"], "id": prefix + r(key)});
            setData(input, 'cb-filter', defaults[key].cbCallback);
            setData(input, 'cb-filter-options', [input.getAttribute('id')]);
            input.setAttribute('data-is', 'mympd-select-search');
            col.classList.add('position-relative');
            const btnGrp = elCreateNode('div', {"class": ["btn-group", "d-flex"]}, input);
            col.appendChild(btnGrp);
        }
        else if (defaults[key].inputType === 'checkbox') {
            const btn = elCreateEmpty('button', {"type": "button", "id": prefix + r(key), "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]});
            if (fields[key] === true) {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            else {
                btn.textContent = 'radio_button_unchecked';
            }
            if (defaults[key].onClick !== undefined) {
                btn.addEventListener('click', function(event) {
                    window[defaults[key].onClick](event);
                }, false);
            }
            else {
                btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target);
                }, false);
            }
            col.appendChild(btn);
        }
        else {
            const it = defaults[key].inputType === 'color' ? 'color' : 'text';
            const input = elCreateEmpty('input', {"is": "mympd-input-reset", "id": prefix + r(key), "placeholder": defaults[key].defaultValue,
                "value": fields[key], "class": ["form-control"], "type": it});
            col.appendChild(input);
        }
        if (defaults[key].invalid !== undefined) {
            col.appendChild(elCreateText('div', {"class": ["invalid-feedback"], "data-phrase": defaults[key].invalid}, tn(defaults[key].invalid)));
        }
        if (defaults[key].warn !== undefined) {
            col.appendChild(elCreateText('div', {"id": "warn" + prefix + r(key), "class": ["mt-2", "mb-1", "alert", "alert-warning", "d-none"], "data-prase": defaults[key].warn}, tn(defaults[key].warn)));
        }

        advFrm[form].appendChild(
            elCreateNodes('div', {"class": ["mb-3", "row"]}, [
                elCreateText('label', {"class": ["col-sm-4", "col-form-label"], "for": prefix + r(key), "data-phrase": defaults[key].title}, defaults[key].title),
                col
            ])
        );
    }

    for (const key in defaults) {
        if (defaults[key].onChange !== undefined) {
            document.getElementById(prefix + r(key)).addEventListener('change', function(event) {
                window[defaults[key].onChange](event);
            }, false);
        }
    }

    //set featWhence feature detection for default actions
    for (const sel of ['inputWebUIsettingclickQuickPlay', 'inputWebUIsettingclickFilesystemPlaylist',
        'inputWebUIsettingclickPlaylist', 'inputWebUIsettingclickSong',
        'inputWebUIsettingclickRadioFavorites', 'inputWebUIsettingclickRadiobrowser'])
    {
        const options = document.getElementById(sel).getElementsByTagName('option');
        for (const opt of options) {
            if (opt.value === 'insert' || opt.value === 'play') {
                opt.classList.add('featWhence');
            }
        }
    }
}

function setFeatures() {
    //web ui features
    features.featCacert = settings.features.featCacert;
    features.featHome = settings.webuiSettings.enableHome;
    features.featLocalPlayback = settings.webuiSettings.enableLocalPlayback === true ?
        (settings.partition.mpdStreamPort > 0 || settings.partition.streamUri.length > 0 ? true : false) : false;
    features.featScripting = settings.webuiSettings.enableScripting === true ?
        settings.features.featScripting : false;
    features.featTimer = settings.webuiSettings.enableTimer;
    features.featTrigger = settings.webuiSettings.enableTrigger;

    //mpd features
    if (settings.partition.mpdConnected === true) {
        features.featLibrary = settings.features.featLibrary;
        features.featLyrics = settings.webuiSettings.enableLyrics === true ? settings.features.featLibrary : false;
        features.featMounts = settings.webuiSettings.enableMounts === true ? settings.features.featMounts : false;
        features.featNeighbors = settings.webuiSettings.enableMounts === true ? settings.features.featNeighbors : false;
        features.featPartitions = settings.webuiSettings.enablePartitions === true ? settings.features.featPartitions : false;
        features.featPlaylists = settings.features.featPlaylists;
        features.featSmartpls = settings.features.featPlaylists === true ? settings.smartpls : false;
        features.featStickers = settings.features.featStickers;
        features.featTags = settings.features.featTags;
        features.featBinarylimit = settings.features.featBinarylimit;
        features.featFingerprint = settings.features.featFingerprint;
        features.featPlaylistRmRange = settings.features.featPlaylistRmRange;
        features.featWhence = settings.features.featWhence;
        features.featAdvqueue = settings.features.featAdvqueue;
    }
}

function applyFeatures() {
    //show or hide elements
    for (const feature in features) {
        const els = document.getElementsByClassName(feature);
        const displayValue = features[feature] === true ? '' : 'none';
        for (const el of els) {
            el.style.display = displayValue;
        }
    }
}

function parseMPDSettings() {
    document.getElementById('partitionName').textContent = localSettings.partition;

    if (settings.webuiSettings.uiBgCover === true) {
        setBackgroundImage(domCache.body, currentSongObj.uri);
    }
    else {
        clearBackgroundImage(domCache.body);
    }

    const triggerEventList = document.getElementById('selectTriggerEvent');
    elClear(triggerEventList);
    for (const event in settings.triggerEvents) {
        triggerEventList.appendChild(
            elCreateText('option', {"value": settings.triggerEvents[event]}, tn(event))
        );
    }

    settings.tagList.sort();
    settings.tagListSearch.sort();
    settings.tagListBrowse.sort();

    filterCols('Playback');

    for (const table of ['Search', 'QueueCurrent', 'QueueLastPlayed', 'QueueJukebox',
            'BrowsePlaylistsDetail', 'BrowseFilesystem', 'BrowseDatabaseDetail'])
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

    //enforce disc for album details view
    if (settings.colsBrowseDatabaseDetailFetch.includes('Disc') === false &&
        settings.tagList.includes('Disc'))
    {
        settings.colsBrowseDatabaseDetailFetch.push('Disc');
    }

    if (features.featTags === false) {
        app.cards.Browse.active = 'Filesystem';
        app.cards.Search.sort.tag = 'filename';
        app.cards.Search.filter = 'filename';
        app.cards.Queue.tabs.Current.filter = 'filename';
        settings.colsQueueCurrent = ["Pos", "Title", "Duration"];
        settings.colsQueueLastPlayed = ["Pos", "Title", "LastPlayed"];
        settings.colsQueueJukebox = ["Pos", "Title"];
        settings.colsSearch = ["Title", "Duration"];
        settings.colsBrowseFilesystem = ["Type", "Title", "Duration"];
        settings.colsBrowseDatabase = ["Track", "Title", "Duration"];
        settings.colsPlayback = [];
    }
    else {
        //construct playback view
        const pbtl = document.getElementById('cardPlaybackTags');
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
                elCreateText('small', {}, tn(settings.colsPlayback[i])),
                elCreateEmpty('p', {})
            ]);
            setData(div, 'tag', settings.colsPlayback[i]);
            pbtl.appendChild(div);
        }
        //fill blank card with currentSongObj
        if (currentSongObj !== null) {
            setPlaybackCardTags(currentSongObj);
        }
        //tagselect dropdown
        const menu = document.getElementById('PlaybackColsDropdown').getElementsByTagName('form')[0];
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
        if (app.cards.Browse.tabs.Database.views.List.filter === 'AlbumArtist') {
            app.cards.Browse.tabs.Database.views.List.filter = tagAlbumArtist;
        }
        if (app.cards.Browse.tabs.Database.views.List.sort.tag === 'AlbumArtist') {
            app.cards.Browse.tabs.Database.views.List.sort.tag = tagAlbumArtist;
        }
    }

    addTagList('BrowseDatabaseByTagDropdown', 'tagListBrowse');
    addTagList('BrowseNavPlaylistsDropdown', 'tagListBrowse');
    addTagList('BrowseNavFilesystemDropdown', 'tagListBrowse');
    addTagList('BrowseNavRadioFavoritesDropdown', 'tagListBrowse');
    addTagList('BrowseNavWebradiodbDropdown', 'tagListBrowse');
    addTagList('BrowseNavRadiobrowserDropdown', 'tagListBrowse');

    addTagList('searchQueueTags', 'tagListSearch');
    addTagList('searchTags', 'tagListSearch');
    addTagList('searchDatabaseTags', 'tagListBrowse');
    addTagList('databaseSortTagsList', 'tagListBrowse');
    addTagList('dropdownSortPlaylistTags', 'tagList');

    addTagListSelect('saveSmartPlaylistSort', 'tagList');
}

//eslint-disable-next-line no-unused-vars
function saveSettings(closeModal) {
    cleanupModalId('modalSettings');
    let formOK = true;

    for (const inputId of ['inputWebUIsettinguiThumbnailSize', 'inputSettinglastPlayedCount',
        'inputSmartplsInterval', 'inputSettingvolumeMax', 'inputSettingvolumeMin',
        'inputSettingvolumeStep'])
    {
        const inputEl = document.getElementById(inputId);
        if (validateUint(inputEl) === false) {
            formOK = false;
        }
    }

    const inputCoverimageNames = document.getElementById('inputCoverimageNames');
    if (validateFilenameList(inputCoverimageNames) === false) {
        formOK = false;
    }
    const inputThumbnailNames = document.getElementById('inputThumbnailNames');
    if (validateFilenameList(inputThumbnailNames) === false) {
        formOK = false;
    }

    const inputBookletName = document.getElementById('inputBookletName');
    if (validateFilename(inputBookletName) === false) {
        formOK = false;
    }

    const inputScaleRatio = document.getElementById('inputScaleRatio');
    //handle scaleRatio only for mobile browsers
    if (userAgentData.isMobile === true) {
        if (validateFloat(inputScaleRatio) === false) {
            formOK = false;
        }
    }

    if (formOK === true) {
        //browser specific settings
        localSettings.localPlaybackAutoplay = (document.getElementById('btnEnableLocalPlaybackAutoplay').classList.contains('active') ? true : false);
        localSettings.enforceMobile = (document.getElementById('btnEnforceMobile').classList.contains('active') ? true : false);
        setUserAgentData();

        //use scaleRatio only for mobile browsers
        localSettings.scaleRatio = userAgentData.isMobile === true ?
            parseFloat(inputScaleRatio.value) : 1.0;
        setViewport();

        //save localSettings in browsers localStorage
        try {
            for (const key in localSettings) {
                localStorage.setItem(key, localSettings[key]);
            }
        }
        catch(err) {
            logError('Can not save settings to localStorage: ' + err.message);
        }

        //from hours to seconds
        const smartplsInterval = Number(document.getElementById('inputSmartplsInterval').value) * 60 * 60;

        const webuiSettings = {};
        for (const key in webuiSettingsDefault) {
            const el = document.getElementById('inputWebUIsetting' + r(key));
            if (el) {
                if (webuiSettingsDefault[key].inputType === 'select') {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(getSelectValue(el)) : getSelectValue(el);
                }
                else if (webuiSettingsDefault[key].inputType === 'mympd-select-search') {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(getData(el, 'value')) : getData(el, 'value');
                }
                else if (webuiSettingsDefault[key].inputType === 'checkbox') {
                    webuiSettings[key] = el.classList.contains('active') ? true : false;
                }
                else {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(el.value) : el.value;
                }
            }
        }

        webuiSettings.enableLyrics = (document.getElementById('btnEnableLyrics').classList.contains('active') ? true : false);
        webuiSettings.enableLocalPlayback = (document.getElementById('btnEnableLocalPlayback').classList.contains('active') ? true : false);

        const params = {
            "coverimageNames": inputCoverimageNames.value,
            "thumbnailNames": inputThumbnailNames.value,
            "lastPlayedCount": Number(document.getElementById('inputSettinglastPlayedCount').value),
            "smartpls": (document.getElementById('btnSmartpls').classList.contains('active') ? true : false),
            "smartplsPrefix": document.getElementById('inputSmartplsPrefix').value,
            "smartplsInterval": smartplsInterval,
            "smartplsSort": document.getElementById('selectSmartplsSort').value,
            "smartplsGenerateTagList": getTagMultiSelectValues(document.getElementById('listGeneratePlsTags'), false),
            "tagList": getTagMultiSelectValues(document.getElementById('listEnabledTags'), false),
            "tagListSearch": getTagMultiSelectValues(document.getElementById('listSearchTags'), false),
            "tagListBrowse": getTagMultiSelectValues(document.getElementById('listBrowseTags'), false),
            "bookletName": inputBookletName.value,
            "volumeMin": Number(document.getElementById('inputSettingvolumeMin').value),
            "volumeMax": Number(document.getElementById('inputSettingvolumeMax').value),
            "volumeStep": Number(document.getElementById('inputSettingvolumeStep').value),
            "lyricsUsltExt": document.getElementById('inputSettinglyricsUsltExt').value,
            "lyricsSyltExt": document.getElementById('inputSettinglyricsSyltExt').value,
            "lyricsVorbisUslt": document.getElementById('inputSettinglyricsVorbisUslt').value,
            "lyricsVorbisSylt": document.getElementById('inputSettinglyricsVorbisSylt').value,
            "listenbrainzToken": document.getElementById('inputListenbrainzToken').value,
            "webuiSettings": webuiSettings
        };

        if (closeModal === true) {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', params, saveSettingsClose, true);
        }
        else {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', params, saveSettingsApply, true);
        }
    }
}

function saveSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        savePartitionSettings(true);
        
    }
}

function saveSettingsApply(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        savePartitionSettings(false);
    }
}

//eslint-disable-next-line no-unused-vars
function savePartitionSettings(closeModal) {
    let formOK = true;
    const mpdStreamPortEl = document.getElementById('inputMpdStreamPort');
    const streamUriEl = document.getElementById('inputStreamUri');
    if (validateIntRange(mpdStreamPortEl, 0, 65535) === false) {
        formOK = false;
    }
    if (streamUriEl.value.length > 0 &&
        validateStream(streamUriEl) === false)
    {
        formOK = false;
    }

    if (formOK === true) {
        const params = {
            "highlightColor": document.getElementById('inputHighlightColor').value,
            "mpdStreamPort": Number(mpdStreamPortEl.value),
            "streamUri": streamUriEl.value
        };
        if (closeModal === true) {
            sendAPI('MYMPD_API_PARTITION_SAVE', params, savePartitionSettingsClose, true);
        }
        else {
            sendAPI('MYMPD_API_PARTITION_SAVE', params, savePartitionSettingsApply, true);
        }
    }
}

function savePartitionSettingsApply(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        btnWaiting(document.getElementById('btnApplySettings'), true);
        getSettings(true);
    }
}

function savePartitionSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings(true);
        uiElements.modalSettings.hide();
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueueSettings() {
    cleanupModalId('modalQueueSettings');
    let formOK = true;

    for (const inputId of ['inputCrossfade', 'inputJukeboxQueueLength', 'inputJukeboxLastPlayed']) {
        const inputEl = document.getElementById(inputId);
        if (validateInt(inputEl) === false) {
            formOK = false;
        }
    }

    const mixrampDbEl = document.getElementById('inputMixrampDb');
    if (validateFloatRange(mixrampDbEl, -100, 0) === false) {
        formOK = false;
    }
    const mixrampDelayEl = document.getElementById('inputMixrampDelay');
    if (validateFloatRange(mixrampDelayEl, -1, 100) === false) {
        formOK = false;
    }

    const singleState = getBtnGroupValueId('btnSingleGroup');
    const jukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    const replaygain = getBtnGroupValueId('btnReplaygainGroup');
    let jukeboxUniqueTag = getSelectValueId('selectJukeboxUniqueTag');
    const jukeboxPlaylist = getDataId('selectJukeboxPlaylist', 'value');

    if (jukeboxMode === 'album') {
        jukeboxUniqueTag = 'Album';
    }

    if (formOK === true) {
        btnWaitingId('btnSaveQueueSettings', true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? true : false),
            "random": (document.getElementById('btnRandom').classList.contains('active') ? true : false),
            "single": singleState,
            "repeat": (document.getElementById('btnRepeat').classList.contains('active') ? true : false),
            "replaygain": replaygain,
            "crossfade": Number(document.getElementById('inputCrossfade').value),
            "mixrampDb": Number(mixrampDbEl.value),
            "mixrampDelay": Number(mixrampDelayEl.value),
            "jukeboxMode": jukeboxMode,
            "jukeboxPlaylist": jukeboxPlaylist,
            "jukeboxQueueLength": Number(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": Number(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTag,
            "autoPlay": (document.getElementById('btnAutoPlay').classList.contains('active') ? true : false)
        }, saveQueueSettingsClose, true);
    }
}

function saveQueueSettingsClose(obj) {
    btnWaitingId('btnSaveQueueSettings', false);
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings(false);
        uiElements.modalQueueSettings.hide();
    }
}

function getTagMultiSelectValues(taglist, translated) {
    const values = [];
    const chkBoxes = taglist.getElementsByTagName('button');
    for (let i = 0, j = chkBoxes.length; i < j; i++) {
        if (chkBoxes[i].classList.contains('active')) {
            if (translated === true) {
                values.push(tn(chkBoxes[i].name));
            }
            else {
                values.push(chkBoxes[i].name);
            }
        }
    }
    if (translated === true) {
        return values.join(', ');
    }
    return values.join(',');
}

function initTagMultiSelect(inputId, listId, allTags, enabledTags) {
    const values = [];
    const list = document.getElementById(listId);
    elClear(list);
    for (let i = 0, j = allTags.length; i < j; i++) {
        if (enabledTags.includes(allTags[i])) {
            values.push(tn(allTags[i]));
        }
        const btn = elCreateEmpty('button', {"class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-small", "me-2"], "name": allTags[i]});
        if (enabledTags.includes(allTags[i])) {
            btn.classList.add('active');
            btn.textContent = 'check';
        }
        else {
            btn.textContent = 'radio_button_unchecked';
        }
        list.appendChild(
            elCreateNodes('div', {"class": ["form-check"]}, [
                btn,
                elCreateText('label', {"class": ["form-check-label"], "for": allTags[i]}, allTags[i])
            ])
        );
    }

    const inputEl = document.getElementById(inputId);
    inputEl.value = values.join(', ');
    if (getData(inputEl, 'init') === true) {
        return;
    }
    setData(inputEl, 'init', true);
    document.getElementById(listId).addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target);
            event.target.parentNode.parentNode.parentNode.previousElementSibling.value =
                getTagMultiSelectValues(event.target.parentNode.parentNode, true);
        }
    });
}

function filterCols(x) {
    const tags = setColTags(x);
    const set = "cols" + x;
    const cols = [];
    for (let i = 0, j = settings[set].length; i < j; i++) {
        if (tags.includes(settings[set][i])) {
            cols.push(settings[set][i]);
        }
    }
    settings[set] = cols;
    logDebug('Columns for ' + set + ': ' + cols);
}

//eslint-disable-next-line no-unused-vars
function toggleBtnNotifyWeb(event) {
    const btnNotifyWeb = event.target;
    const notifyWebState = btnNotifyWeb.classList.contains('active') ? true : false;
    if (notificationsSupported() === false) {
        toggleBtnChk(btnNotifyWeb, false);
        settings.webuiSettings.notifyWeb = false;
        return;
    }
    if (notifyWebState === true) {
        toggleBtnChk(btnNotifyWeb, false);
        settings.webuiSettings.notifyWeb = false;
        elHideId('warnNotifyWeb');
        return;
    }
    Notification.requestPermission(function (permission) {
        if (!('permission' in Notification)) {
            Notification.permission = permission;
        }
        if (permission === 'granted') {
            toggleBtnChk(btnNotifyWeb, true);
            settings.webuiSettings.notifyWeb = true;
            elHideId('warnNotifyWeb');
        }
        else {
            toggleBtnChk(btnNotifyWeb, false);
            settings.webuiSettings.notifyWeb = false;
            elShowId('warnNotifyWeb');
        }
    });
}

function setNavbarIcons() {
    const oldBadgeQueueItems = document.getElementById('badgeQueueItems');
    let oldQueueLength = 0;
    if (oldBadgeQueueItems) {
        oldQueueLength = oldBadgeQueueItems.textContent;
    }

    const container = document.getElementById('navbar-main');
    elClear(container);
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
            a.setAttribute('data-popover', 'Navbar' + icon.options.join(''));
        }
        if (icon.options[0] === 'Queue' &&
            icon.options.length === 1)
        {
            a.appendChild(
                elCreateText('span', {"id": "badgeQueueItems", "class": ["badge", "bg-secondary"]}, oldQueueLength)
            );
        }
        btn.appendChild(a);
        container.appendChild(btn);
        setData(a, 'href', JSON.stringify({"cmd": "appGoto", "options": icon.options}));
    }

    //cache elements, reused in appPrepare
    domCache.navbarBtns = container.getElementsByTagName('div');
    domCache.navbarBtnsLen = domCache.navbarBtns.length;
}

function getBgImageList() {
    const list = document.getElementById('inputWebUIsettinguiBgImage');
    getImageList(list, bgImageValues, 'backgrounds');
}

//eslint-disable-next-line no-unused-vars
function getImageListId(selectId, addOptions, type) {
    getImageList(document.getElementById(selectId), addOptions, type)
}

function getImageList(sel, addOptions, type) {
    sendAPI("MYMPD_API_PICTURE_LIST", {
        "type": type
    }, function(obj) {
        elClear(sel.filterResult);
        for (const option of addOptions) {
            sel.addFilterResult(option.text, option.value);
        }
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            sel.addFilterResult(obj.result.data[i], obj.result.data[i]);
        }
    });
}

//eslint-disable-next-line no-unused-vars
function filterImageSelect(elId, searchstr) {
    const select = document.getElementById(elId).filterResult;
    searchstr = searchstr.toLowerCase();
    const items = select.getElementsByTagName('li');
    for (const item of items) {
        const value = item.textContent.toLowerCase();
        if (value.indexOf(searchstr) >= 0) {
            elShow(item);
        }
        else {
            elHide(item);
        }
    }
}

function warnLocale(value) {
    const warnEl = document.getElementById('warnMissingPhrases');
    elClear(warnEl);
    if (i18n[value].missingPhrases > 0) {
        warnEl.appendChild(
            elCreateText('p', {}, tn('Missing translations', i18n[value].missingPhrases))
        );
        warnEl.appendChild(
            elCreateText('a', {"class": ["alert-link", "external"], "target": "_blank",
                "href": "https://github.com/jcorporation/myMPD/discussions/167"}, tn('Help to improve myMPD')
            )
        );
        elShow(warnEl);
    }
    else {
        elHide(warnEl);
    }
}

//eslint-disable-next-line no-unused-vars
function setLoglevel() {
    const loglevel = Number(getSelectValueId('selectSetLoglevel'));
    sendAPI("MYMPD_API_LOGLEVEL", {
        "loglevel": loglevel
    }, function() {
        settings.loglevel = loglevel
    }, false);
}

function resetLocalSettings() {
    for (const key in localSettings) {
        localStorage.removeItem(key);
    }
}
