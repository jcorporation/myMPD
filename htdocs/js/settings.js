"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
        getSettings();
        hideModalAlert();
        removeIsInvalid(document.getElementById('modalSettings'));
        removeEnterPinFooter();
    });
    
    document.getElementById('modalQueueSettings').addEventListener('shown.bs.modal', function () {
        getSettings();
        hideModalAlert();
        removeIsInvalid(document.getElementById('modalQueueSettings'));
    });

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        getSettings();
        hideModalAlert();
        removeIsInvalid(document.getElementById('modalConnection'));
        removeEnterPinFooter();
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            const value = getCustomDomProperty(document.getElementById('btnJukeboxModeGroup').getElementsByClassName('active')[0], 'data-value');
            if (value === '0') {
                elDisableId('inputJukeboxQueueLength');
                elDisableId('selectJukeboxPlaylist');
            }
            else if (value === '2') {
                elDisableId('inputJukeboxQueueLength');
                elDisableId('selectJukeboxPlaylist');
                document.getElementById('selectJukeboxPlaylist').value = 'Database';
            }
            else if (value === '1') {
                elEnableId('inputJukeboxQueueLength');
                elEnableId('selectJukeboxPlaylist');
            }
            if (value !== '0') {
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
    const bgImageValue = getSelectValue(bgImageEl);
    if (value === 'theme-light') {
        document.getElementById('inputWebUIsettinguiBgColor').value = '#ffffff';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = '/assets/mympd-background-light.svg';
        }
    }
    else {
        //theme-dark is the default
        document.getElementById('inputWebUIsettinguiBgColor').value = '#060708';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = '/assets/mympd-background-dark.svg';
        }
    }
}

//eslint-disable-next-line no-unused-vars
function saveConnection() {
    let formOK = true;
    const mpdHostEl = document.getElementById('inputMpdHost');
    const mpdPortEl = document.getElementById('inputMpdPort');
    const mpdPassEl = document.getElementById('inputMpdPass');
    const playlistDirectoryEl = document.getElementById('inputPlaylistDirectory');
    const mpdStreamPortEl = document.getElementById('inputMpdStreamPort');
    const mpdBinarylimitEl = document.getElementById('inputMpdBinarylimit');
    const mpdTimeoutEl = document.getElementById('inputMpdTimeout');
    const musicDirectoryEl = document.getElementById('selectMusicDirectory');
    let musicDirectory = getSelectValue(musicDirectoryEl);
    
    if (musicDirectory === 'auto' && mpdHostEl.value.indexOf('/') !== 0) {
        formOK = false;
        setIsInvalid(musicDirectoryEl);
    }
    
    if (musicDirectory === 'custom') {
        const musicDirectoryValueEl  = document.getElementById('inputMusicDirectory');
        if (!validatePath(musicDirectoryValueEl)) {
            formOK = false;        
        }
        musicDirectory = musicDirectoryValueEl.value;
    }    
    if (mpdPortEl.value === '') {
        mpdPortEl.value = '6600';
    }
    if (mpdHostEl.value.indexOf('/') !== 0) {
        if (!validateInt(mpdPortEl)) {
            formOK = false;        
        }
        if (!validateHost(mpdHostEl)) {
            formOK = false;        
        }
    }
    if (!validatePath(playlistDirectoryEl)) {
        formOK = false;
    }
    if (!validateIntRange(mpdStreamPortEl, 1024, 65535)) {
        formOK = false;
    }
    if (!validateIntRange(mpdBinarylimitEl, 4, 256)) {
        formOK = false;
    }
    if (!validateIntRange(mpdTimeoutEl, 1, 100)) {
        formOK = false;
    }
    if (formOK === true) {
        sendAPI("MYMPD_API_CONNECTION_SAVE", {
            "mpdHost": mpdHostEl.value,
            "mpdPort": Number(mpdPortEl.value),
            "mpdPass": mpdPassEl.value,
            "musicDirectory": musicDirectory,
            "playlistDirectory": playlistDirectoryEl.value,
            "mpdStreamPort": Number(mpdStreamPortEl.value),
            "mpdBinarylimit": Number(mpdBinarylimitEl.value) * 1024,
            "mpdTimeout": Number(mpdTimeoutEl.value) * 1000,
            "mpdKeepalive": (document.getElementById('btnMpdKeepalive').classList.contains('active') ? true : false)
        }, saveConnectionClose);
    }
}

function saveConnectionClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        getSettings(false);
        uiElements.modalConnection.hide();
    }
}

function getSettings(onerror) {
    settingsParsed = 'no';
    sendAPI("MYMPD_API_SETTINGS_GET", {}, parseSettings, onerror);
}

function checkConsume() {
    const stateConsume = document.getElementById('btnConsume').classList.contains('active') ? true : false;
    const stateJukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    if (stateJukeboxMode > 0 && stateConsume === false) {
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
            showAppInitAlert(obj === '' ? t('Can not parse settings') : t(obj.error.message));
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
        setTheme = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'theme-dark' : 'theme-light';
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
    if (settings.webuiSettings.uiBgImage.indexOf('/assets/') === 0) {
        domCache.body.style.backgroundImage = 'url("' + subdir + myEncodeURI(settings.webuiSettings.uiBgImage) + '")';
    }
    else if (settings.webuiSettings.uiBgImage !== '') {
        domCache.body.style.backgroundImage = 'url("' + subdir + '/pics/' + myEncodeURI(settings.webuiSettings.uiBgImage) + '")';
    }
    else {
        domCache.body.style.backgroundImage = '';
    }
    document.documentElement.style.setProperty('--mympd-backgroundcolor', settings.webuiSettings.uiBgColor);

    const albumartbg = document.querySelectorAll('.albumartbg');
    for (let i = 0, j = albumartbg.length; i < j; i++) {
        albumartbg[i].style.filter = settings.webuiSettings.uiBgCssFilter;
    }

    //Navigation and footer
    setNavbarIcons();

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

    //set local playback url    
    if (settings.webuiSettings.enableLocalPlayback === true) {
        setLocalPlayerUrl();
    }
    
    //parse mpd settings if connected
    if (settings.mpdConnected === true) {
        parseMPDSettings();
    }
    
    //Info in about modal
    if (settings.mpdHost.indexOf('/') !== 0) {
        document.getElementById('mpdInfo_host').textContent = settings.mpdHost + ':' + settings.mpdPort;
    }
    else {
        document.getElementById('mpdInfo_host').textContent = settings.mpdHost;
    }

    document.documentElement.style.setProperty('--mympd-coverimagesize', settings.webuiSettings.uiCoverimageSize + "px");
    document.documentElement.style.setProperty('--mympd-coverimagesizesmall', settings.webuiSettings.uiCoverimageSizeSmall + "px");
    document.documentElement.style.setProperty('--mympd-highlightcolor', settings.webuiSettings.uiHighlightColor);

    //default limit for all apps
    //convert from string to int
    let limit = settings.webuiSettings.uiMaxElementsPerPage;
    if (limit === 0) {
        limit = 500;
    }
    app.apps.Home.limit = limit;
    app.apps.Playback.limit = limit;
    app.apps.Queue.tabs.Current.limit = limit;
    app.apps.Queue.tabs.LastPlayed.limit = limit;
    app.apps.Queue.tabs.Jukebox.limit = limit;
    app.apps.Browse.tabs.Filesystem.limit = limit;
    app.apps.Browse.tabs.Playlists.views.List.limit = limit;
    app.apps.Browse.tabs.Playlists.views.Detail.limit = limit;
    app.apps.Browse.tabs.Database.views.List.limit = limit;
    app.apps.Browse.tabs.Database.views.Detail.limit = limit;
    app.apps.Search.limit = limit;

    //scripts
    if (scriptsInited === false) {
        document.getElementById('selectTimerAction').innerHTML = '<optgroup data-value="player" label="' + t('Playback') + '">' +
            '<option value="startplay">' + t('Start playback') + '</option>' +
            '<option value="stopplay">' + t('Stop playback') + '</option>' +
            '</optgroup>';

        if (features.featScripting === true) {
            getScriptList(true);
        }
        else {
            elClear(document.getElementById('scripts'));
            //reinit mainmenu -> change of script list
            uiElements.dropdownMainMenu.dispose();
            uiElements.dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
        }
        scriptsInited = true;
    }

    //volumebar
    document.getElementById('volumeBar').setAttribute('min', settings.volumeMin);
    document.getElementById('volumeBar').setAttribute('max', settings.volumeMax);

    //update columns
    if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
        appRoute();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'Jukebox') {
        appRoute();
    }
    else if (app.current.app === 'Search') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Filesystem') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Playlists' && app.current.view === 'Detail') {
        appRoute();
    }
    else if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.search !== '') {
        appRoute();
    }

    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        navigator.mediaSession.setActionHandler('play', clickPlay);
        navigator.mediaSession.setActionHandler('pause', clickPlay);
        navigator.mediaSession.setActionHandler('stop', clickStop);
        navigator.mediaSession.setActionHandler('seekbackward', seekRelativeBackward);
        navigator.mediaSession.setActionHandler('seekforward', seekRelativeForward);
        navigator.mediaSession.setActionHandler('previoustrack', clickPrev);
        navigator.mediaSession.setActionHandler('nexttrack', clickNext);
        
        if (!navigator.mediaSession.setPositionState) {
            logDebug('mediaSession.setPositionState not supported by browser');
        }
    }
    else {
        logDebug('mediaSession not supported by browser');
    }

    //finished parse setting, set ui state
    toggleUI();
    btnWaiting(document.getElementById('btnApplySettings'), false);
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
        locale = navigator.language || navigator.userLanguage;
    }
    else {
        locale = newLocale;
    }
    let localeFound = false;
    for (const l of locales) {
        if (l.code === locale) {
            localeFound = true;
            break;
        }
    }
    if (localeFound === false) {
        logError('Locale ' + locale + 'not defined');
        locale = 'en-US';
    }

    i18nHtml(domCache.body);
}

function populateQueueSettingsFrm() {
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.jukeboxMode);
    document.getElementById('selectJukeboxUniqueTag').value = settings.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.jukeboxLastPlayed;

    addTagListSelect('selectJukeboxUniqueTag', 'tagListBrowse');
    
    if (settings.jukeboxMode === 0) {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
    }
    else if (settings.jukeboxMode === 2) {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
    }
    else if (settings.jukeboxMode === 1) {
        elEnableId('inputJukeboxQueueLength');
        elEnableId('selectJukeboxPlaylist');
    }
    
    if (settings.mpdConnected === true) {
        if (features.featPlaylists === true) {
            sendAPI("MYMPD_API_PLAYLIST_LIST", {"searchstr": "", "offset": 0, "limit": 0}, function(obj) {
                getAllPlaylists(obj, 'selectJukeboxPlaylist', settings.jukeboxPlaylist);
            });
        }
        else {
            document.getElementById('selectJukeboxPlaylist').innerHTML = '<option value="Database">' + t('Database') + '</option>';
        }
        toggleBtnChkId('btnRandom', settings.random);
        toggleBtnChkId('btnConsume', settings.consume);
        toggleBtnChkId('btnRepeat', settings.repeat);
        toggleBtnChkId('btnAutoPlay', settings.autoPlay);
        toggleBtnGroupValue(document.getElementById('btnSingleGroup'), settings.single);
        toggleBtnGroupValue(document.getElementById('btnReplaygainGroup'), settings.replaygain);
        document.getElementById('inputCrossfade').value = settings.crossfade;    
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
    document.getElementById('inputMpdStreamPort').value = settings.mpdStreamPort;
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

function populateSettingsFrm() {
    createSettingsFrm();

    getBgImageList(settings.webuiSettings.uiBgImage);

    //locales
    let localeList = '';
    for (const l of locales) {
        localeList += '<option value="' + e(l.code) + '"' + 
            (l.code === settings.webuiSettings.uiLocale ? ' selected="selected"' : '') + '>' + 
            e(l.desc) + ' (' + e(l.code) + ')</option>';
    }
    document.getElementById('inputWebUIsettinguiLocale').innerHTML = localeList;
    warnLocale(settings.webuiSettings.uiLocale);

    //web notifications - check permission
    const btnNotifyWeb = document.getElementById('inputWebUIsettingnotifyWeb');
    elHideId('warnNotifyWeb');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.notificationWeb === true) {
                elShowId('warnNotifyWeb');
            }
            settings.notificationWeb = false;
        }
        if (Notification.permission === 'denied') {
            elShowId('warnNotifyWeb');
        }
        toggleBtnChk(btnNotifyWeb, settings.notificationWeb);
        elEnable(btnNotifyWeb);
    }
    else {
        elDisable(btnNotifyWeb);
        toggleBtnChk(btnNotifyWeb, false);
    }

    if (isMobile === true) {    
        document.getElementById('inputScaleRatio').value = scale;
    }
    
    document.getElementById('inputBookletName').value = settings.bookletName;
    document.getElementById('inputCoverimageNames').value = settings.coverimageNames;
    document.getElementById('inputCovercacheKeepDays').value = settings.covercacheKeepDays;
    
    //smart playlists
    if (settings.featSmartpls === true) {
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

    const inputWebUIsettinguiBgCover = document.getElementById('inputWebUIsettinguiBgCover');
    inputWebUIsettinguiBgCover.setAttribute('data-toggle', 'collapse');
    inputWebUIsettinguiBgCover.setAttribute('data-target', '#bgCssFilterFrm');
    if (uiElements.collapseuiBgCover !== undefined) {
        uiElements.collapseuiBgCover.dispose();
    }
    uiElements.collapseuiBgCover = new BSN.Collapse(inputWebUIsettinguiBgCover);
    toggleBtnChkCollapseId('inputWebUIsettinguiBgCover', 'bgCssFilterFrm', settings.webuiSettings.uiBgCover);

    //tag multiselects
    initTagMultiSelect('inputEnabledTags', 'listEnabledTags', settings.tagListMpd, settings.tagList);
    initTagMultiSelect('inputSearchTags', 'listSearchTags', settings.tagList, settings.tagListSearch);
    initTagMultiSelect('inputBrowseTags', 'listBrowseTags', settings.tagList, settings.tagListBrowse);
    initTagMultiSelect('inputGeneratePlsTags', 'listGeneratePlsTags', settings.tagListBrowse, settings.smartplsGenerateTagList);
    //features - show or hide warnings - use settings object
    setFeatureBtnId('btnEnableLyrics', settings.featLibrary);
    setFeatureBtnId('inputWebUIsettingenableScripting', settings.featScripting);
    setFeatureBtnId('inputWebUIsettingenableMounts', settings.featMounts);
    setFeatureBtnId('inputWebUIsettingenablePartitions', settings.featPartitions);
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
            advFrm[form] = '';
        }
        
        if (defaults[key].inputType === 'section') {
            if (defaults[key].title !== undefined) {
                advFrm[form] += '<hr/><h4>' + t(defaults[key].title) + '</h4>';
            }
            else if (defaults[key].subtitle !== undefined) {
                advFrm[form] += '<h5>' + t(defaults[key].subtitle) + '</h5>';
            }
            continue;
        }
        advFrm[form] += '<div class="mb-3 row">' +
                    '<label class="col-sm-4 col-form-label" for="' + prefix + r(key) + '" data-phrase="' + 
                    e(defaults[key].title) + '">' + t(defaults[key].title) + '</label>' +
                    '<div class="col-sm-8 position-relative">';
        if (defaults[key].inputType === 'select') {
            advFrm[form] += '<select id="' + prefix + r(key) + '" class="form-select">';
            for (let value in defaults[key].validValues) {
                if (defaults[key].contentType === 'integer') {
                    value = Number(value);
                }
                advFrm[form] += '<option value="' + e(value) + '"' +
                    (fields[key] === value ? ' selected="selected"' : '') +
                    '>' + t(defaults[key].validValues[value]) + '</option>';
            }
            advFrm[form] += '</select>';
        }
        else if (defaults[key].inputType === 'checkbox') {
            advFrm[form] += '<button type="button" class="btn btn-sm btn-secondary mi chkBtn ' + 
                (fields[key] === false ? '' : 'active') + ' clickable" id="' + prefix + r(key) + '">' +
                (fields[key] === false ? 'radio_button_unchecked' : 'check') + '</button>';
        }
        else {
            advFrm[form] += '<input is="mympd-input-reset" id="' + prefix + r(key) + '" placeholder="' + defaults[key].defaultValue + '" ' +
                'value="' + e(fields[key]) + '"  class="form-control" type="' + (defaults[key].inputType === 'color' ? 'color' : 'text') + '"/>';
        }
        if (defaults[key].invalid !== undefined) {
            advFrm[form] += '<div class="invalid-feedback" data-phrase="' + defaults[key].invalid + '"></div>';
        }
        advFrm[form] += '</div></div>';
        if (defaults[key].warn !== undefined) {
            advFrm[form] += '<div id="warn' + prefix + r(key) + '" class="alert alert-warning d-none">' + t(defaults[key].warn) + '</div>';
        }
    }
    for (const key in advFrm) {
        document.getElementById(key).innerHTML = advFrm[key];
        const advFrmBtns = document.getElementById(key).getElementsByClassName('chkBtn');
        for (const btn of advFrmBtns) {
            btn.addEventListener('click', function(event) {
                toggleBtnChk(event.target);
            }, false);
        }
    }
    
    for (const key in defaults) {
        if (defaults[key].onChange !== undefined) {
            document.getElementById(prefix + key).addEventListener('change', function(event) {
                window[defaults[key].onChange](event);
            }, false);
        }
    }
}

function setFeatures() {
    //web ui features
    features.featCacert = settings.featCacert;
    features.featHome = settings.webuiSettings.enableHome;
    features.featLocalPlayback = settings.webuiSettings.enableLocalPlayback === true ?
        (settings.mpdStreamPort > 0 ? true : false) : false;
    features.featScripting = settings.webuiSettings.enableScripting === true ?
        (settings.featScripting === true ? true : false) : false;
    features.featTimer = settings.webuiSettings.enableTimer;
    features.featTrigger = settings.webuiSettings.enableTrigger;

    //mpd features
    if (settings.mpdConnected === true) {
        features.featAdvsearch = settings.featAdvsearch;
        features.featLibrary = settings.featLibrary;
        features.featLyrics = settings.webuiSettings.enableLyrics === true ?
            (settings.featLibrary === true ? true : false) : false;
        features.featMounts = settings.webuiSettings.enableMounts === true ?
            (settings.featMounts === true ? true : false) : false;
        features.featNeighbors = settings.webuiSettings.enableMounts === true ?
            (settings.featNeighbors === true ? true : false) : false;
        features.featPartitions = settings.webuiSettings.enablePartitions === true ?
            (settings.featPartitions === true ? true : false) : false;
        features.featPlaylists = settings.featPlaylists;
        features.featSingleOneShot = settings.featSingleOneShot;
        features.featSmartpls = settings.featSmartpls === true ?
            (settings.smartpls === true ? true : false) : false;
        features.featStickers = settings.featStickers;
        features.featTags = settings.featTags;
        features.featBinarylimit = settings.featBinarylimit;
        features.featFingerprint = settings.featFingerprint;
    }

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
    document.getElementById('partitionName').textContent = settings.partition;
    
    if (settings.webuiSettings.uiBgCover === true) {
        setBackgroundImage(lastSongObj.uri);
    }
    else {
        clearBackgroundImage();
    }

    let triggerEventList = '';
    for (const event in settings.triggerEvents) {
        triggerEventList += '<option value="' + e(settings.triggerEvents[event]) + '">' + t(event) + '</option>';
    }
    document.getElementById('selectTriggerEvent').innerHTML = triggerEventList;
    
    settings.tagList.sort();
    settings.tagListSearch.sort();
    settings.tagListBrowse.sort();

    filterCols('Playback');

    for (const table of ['Search', 'QueueCurrent', 'QueueLastPlayed', 'QueueJukebox', 
        'BrowsePlaylistsDetail', 'BrowseFilesystem', 'BrowseDatabaseDetail'])
    {
        filterCols(table);
        setCols(table);
        //enforce albumartist and album for albumactions
        const col = 'cols' + table + 'Fetch';
        settings[col] = settings['cols' + table].slice();
        if (settings[col].includes('Album') === false && settings.tagList.includes('Album')) {
            settings[col].push('Album');
        }
        if (settings[col].includes(tagAlbumArtist) === false && settings.tagList.includes(tagAlbumArtist)) {
            settings[col].push(tagAlbumArtist);
        }
    }
    //enforce disc for album details view
    if (settings.colsBrowseDatabaseDetailFetch.includes('Disc') === false && settings.tagList.includes('Disc')) {
        settings.colsBrowseDatabaseDetailFetch.push('Disc');
    }

    if (features.featTags === false) {
        app.apps.Browse.active = 'Filesystem';
        app.apps.Search.sort = 'filename';
        app.apps.Search.filter = 'filename';
        app.apps.Queue.tabs.Current.filter = 'filename';
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
        let pbtl = '';
        for (let i = 0, j = settings.colsPlayback.length; i < j; i++) {
            pbtl += '<div id="current' + settings.colsPlayback[i]  + '" data-tag="' + 
                settings.colsPlayback[i] + '">' +
                '<small>' + t(settings.colsPlayback[i]) + '</small>' +
                '<p></p></div>';
        }
        document.getElementById('cardPlaybackTags').innerHTML = pbtl;
        //fill blank card with lastSongObj
        if (lastSongObj !== null) {
            setPlaybackCardTags(lastSongObj);
        }
        //tagselect dropdown
        const menu = document.getElementById('PlaybackColsDropdown').getElementsByTagName('form')[0];
        elClear(menu);
        setColsChecklist('Playback', menu);
    }

    if (settings.tagList.includes('Title')) {
        app.apps.Search.sort = 'Title';
    }
    
    if (settings.tagList.includes('AlbumArtist')) {
        tagAlbumArtist = 'AlbumArtist';        
    }
    else if (settings.tagList.includes('Artist')) {
        tagAlbumArtist = 'Artist';        
    }
    
    if (!settings.tagList.includes('AlbumArtist') && app.apps.Browse.tabs.Database.views.List.filter === 'AlbumArtist') {
        app.apps.Browse.tabs.Database.views.List.sort = 'Artist';
    }

    if (features.featAdvsearch === false && app.apps.Browse.active === 'Database') {
        app.apps.Browse.active = 'Filesystem';
    }

    if (features.featAdvsearch === false) {
        const tagEls = document.getElementById('cardPlaybackTags').getElementsByTagName('p');
        for (let i = 0, j = tagEls.length; i < j; i++) {
            tagEls[i].classList.remove('clickable');
        }
    }

    addTagList('BrowseDatabaseByTagDropdown', 'tagListBrowse');
    addTagList('BrowseNavPlaylistsDropdown', 'tagListBrowse');
    addTagList('BrowseNavFilesystemDropdown', 'tagListBrowse');
    
    addTagList('searchqueuetags', 'tagListSearch');
    addTagList('searchtags', 'tagListSearch');
    addTagList('searchDatabaseTags', 'tagListBrowse');
    addTagList('databaseSortTagsList', 'tagListBrowse');
    addTagList('dropdownSortPlaylistTags', 'tagList');
    addTagList('saveSmartPlaylistSort', 'tagList');

    addTagListSelect('saveSmartPlaylistSort', 'tagList');
}

//eslint-disable-next-line no-unused-vars
function resetSettings() {
    sendAPI("MYMPD_API_SETTINGS_RESET", {}, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveSettings(closeModal) {
    let formOK = true;

    for (const inputId of ['inputWebUIsettinguiCoverimageSize', 'inputWebUIsettinguiCoverimageSizeSmall',
            'inputSettinglastPlayedCount', 'inputSmartplsInterval', 'inputSettingvolumeMax', 'inputSettingvolumeMin',
            'inputSettingvolumeStep', 'inputCovercacheKeepDays']) 
    {
        const inputEl = document.getElementById(inputId);
        if (!validateUint(inputEl)) {
            formOK = false;
        }
    }

    const inputCoverimageNames = document.getElementById('inputCoverimageNames');
    if (!validateFilenameList(inputCoverimageNames)) {
        formOK = false;
    }
    
    const inputBookletName = document.getElementById('inputBookletName');
    if (!validateFilename(inputBookletName)) {
        formOK = false;
    }
    
    if (isMobile === true) {
        const inputScaleRatio = document.getElementById('inputScaleRatio');
        if (!validateFloat(inputScaleRatio)) {
            formOK = false;
        }
        else {
            scale = parseFloat(inputScaleRatio.value);
            setViewport(true);
        }
    }

    //from hours to seconds
    const smartplsInterval = Number(document.getElementById('inputSmartplsInterval').value) * 60 * 60;

    const webuiSettings = {};
    for (const key in webuiSettingsDefault) {
        const el = document.getElementById('inputWebUIsetting' + r(key));
        if (el) {
            if (webuiSettingsDefault[key].inputType === 'select') {
                webuiSettings[key] =  webuiSettingsDefault[key].contentType === 'integer' ? Number(getSelectValue(el)) : getSelectValue(el);
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
    
    if (formOK === true) {
        const params = {
            "coverimageNames": inputCoverimageNames.value,
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
            "covercacheKeepDays": Number(document.getElementById('inputCovercacheKeepDays').value),
            "webuiSettings": webuiSettings
        };

        if (closeModal === true) {
            sendAPI("MYMPD_API_SETTINGS_SET", params, saveSettingsClose, true);
        }
        else {
            sendAPI("MYMPD_API_SETTINGS_SET", params, saveSettingsApply, true);
        }
    }
}

function saveSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        getSettings(true);
        uiElements.modalSettings.hide();
    }
}

function saveSettingsApply(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
        getSettings(true);
        btnWaiting(document.getElementById('btnApplySettings'), true);
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueueSettings() {
    let formOK = true;

    for (const inputId of ['inputCrossfade', 'inputJukeboxQueueLength', 'inputJukeboxLastPlayed']) {
        const inputEl = document.getElementById(inputId);
        if (!validateInt(inputEl)) {
            formOK = false;
        }
    }

    const singleState = getBtnGroupValueId('btnSingleGroup');
    const jukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    const replaygain = getBtnGroupValueId('btnReplaygainGroup');
    let jukeboxUniqueTag = getSelectValueId('selectJukeboxUniqueTag');
    const jukeboxPlaylist = getSelectValueId('selectJukeboxPlaylist');
    
    if (jukeboxMode === '2') {
        jukeboxUniqueTag = 'Album';
    }
    
    if (formOK === true) {
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random": (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single": Number(singleState),
            "repeat": (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": replaygain,
            "crossfade": Number(document.getElementById('inputCrossfade').value),
            "jukeboxMode": Number(jukeboxMode),
            "jukeboxPlaylist": jukeboxPlaylist,
            "jukeboxQueueLength": Number(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": Number(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTag,
            "autoPlay": (document.getElementById('btnAutoPlay').classList.contains('active') ? true : false)
        }, saveQueueSettingsClose, true);
    }
}

function saveQueueSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        hideModalAlert();
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
                values.push(t(chkBoxes[i].name));
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
    let list = '';
    for (let i = 0, j = allTags.length; i < j; i++) {
        if (enabledTags.includes(allTags[i])) {
            values.push(t(allTags[i]));
        }
        list += '<div class="form-check">' +
            '<button class="btn btn-secondary btn-xs clickable mi mi-small' + 
            (enabledTags.includes(allTags[i]) ? ' active' : '') + '" name="' + allTags[i] + '">' +
            (enabledTags.includes(allTags[i]) ? 'check' : 'radio_button_unchecked') + '</button>' +
            '<label class="form-check-label" for="' + allTags[i] + '">&nbsp;&nbsp;' + t(allTags[i]) + '</label>' +
            '</div>';
    }
    document.getElementById(listId).innerHTML = list;

    const inputEl = document.getElementById(inputId);
    inputEl.value = values.join(', ');
    if (getCustomDomProperty(inputEl, 'data-init') === 'true') {
        return;
    }
    setCustomDomProperty(inputEl, 'data-init', 'true');
    document.getElementById(listId).addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target);
            event.target.parentNode.parentNode.parentNode.previousElementSibling.value = getTagMultiSelectValues(event.target.parentNode.parentNode, true);
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
function toggleBtnNotifyWeb() {
    const btnNotifyWeb = document.getElementById('btnNotifyWeb');
    const notifyWebState = btnNotifyWeb.classList.contains('active') ? true : false;
    if (notificationsSupported()) {
        if (notifyWebState === false) {
            Notification.requestPermission(function (permission) {
                if (!('permission' in Notification)) {
                    Notification.permission = permission;
                }
                if (permission === 'granted') {
                    toggleBtnChkId('btnNotifyWeb', true);
                    settings.notificationWeb = true;
                    elHideId('warnNotifyWeb');
                } 
                else {
                    toggleBtnChkId('btnNotifyWeb', false);
                    settings.notificationWeb = false;
                    elShowId('warnNotifyWeb');
                }
            });
        }
        else {
            toggleBtnChkId('btnNotifyWeb', false);
            settings.notificationWeb = false;
            elHideId('warnNotifyWeb');
        }
    }
    else {
        toggleBtnChkId('btnNotifyWeb', false);
        settings.notificationWeb = false;
    }
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
        if (id === 'nav' + app.current.app) {
            btn.classList.add('active');
        }
        if (features.featHome === false && icon.options[0] === 'Home') {
            elHide(btn);
        }
        const a = elCreateEmpty('a', {"data-title-phrase": icon.title, "title": tn(icon.title), "href": "#", "class": ["nav-link"]});
        a.appendChild(elCreateText('span', {"class": ["mi"]}, icon.ligature));
        if (icon.options[0] === 'Queue' && icon.options.length === 1) {
            a.appendChild(elCreateText('span', {"id": "badgeQueueItems", "class": ["badge", "bg-secondary"]}, oldQueueLength));
        }
        btn.appendChild(a);
        container.appendChild(btn);
        setCustomDomProperty(a, 'data-href', JSON.stringify({"cmd": "appGoto", "options": icon.options}));
    }
    
    //cache elements, reused in appPrepare
    domCache.navbarBtns = container.getElementsByTagName('div');
    domCache.navbarBtnsLen = domCache.navbarBtns.length;
}

function getBgImageList(image) {
    getImageList('inputWebUIsettinguiBgImage', image, [
        {"value": "", "text": "None"},
        {"value": "/assets/mympd-background-default.svg", "text": "Default image"},
        {"value": "/assets/mympd-background-dark.svg", "text": "Default image dark"},
        {"value": "/assets/mympd-background-light.svg", "text": "Default image light"},
    ]);
}

function getImageList(selectEl, value, addOptions) {
    sendAPI("MYMPD_API_PICTURE_LIST", {}, function(obj) {
        let options = '';
        for (const option of addOptions) {
            options += '<option value="' + e(option.value) + '">' + t(option.text) + '</option>';
        }
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            options += '<option value="' + e(obj.result.data[i]) + '">' + e(obj.result.data[i])  + '</option>';
        }
        const sel = document.getElementById(selectEl);
        sel.innerHTML = options;
        sel.value = value;
    });
}

function warnLocale(value) {
    const warnEl = document.getElementById('warnMissingPhrases');
    if (missingPhrases[value] !== undefined) {
        warnEl.innerHTML = t('Missing translations', missingPhrases[value]) + '<br/>' +
            '<a class="alert-link" target="_blank" href="https://github.com/jcorporation/myMPD/discussions/167">' +
            '<span class="mi">open_in_browser</span>&nbsp;' + t('Help to improve myMPD') + '</a>';
        elShow(warnEl);
    }
    else {
        elHide(warnEl);
    }
}
