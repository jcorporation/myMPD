"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initSettings() {
    let selectThemeHtml = '';
    Object.keys(themes).forEach(function(key) {
        selectThemeHtml += '<option value="' + e(key) + '">' + t(themes[key]) + '</option>';
    });
    document.getElementById('selectTheme').innerHTML = selectThemeHtml;

    document.getElementById('selectTheme').addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        const bgImageEl = document.getElementById('selectBgImage');
        const bgImageValue = getSelectValue(bgImageEl);
        if (value === 'theme-default') { 
            document.getElementById('inputBgColor').value = '#aaaaaa';
            if (bgImageValue.indexOf('/assets/') === 0) {
                bgImageEl.value = '/assets/mympd-background-default.svg';
            }
        }
        else if (value === 'theme-light') {
            document.getElementById('inputBgColor').value = '#ffffff';
            if (bgImageValue.indexOf('/assets/') === 0) {
                bgImageEl.value = '/assets/mympd-background-light.svg';
            }
        }
        else if (value === 'theme-dark') {
            document.getElementById('inputBgColor').value = '#060708';
            if (bgImageValue.indexOf('/assets/') === 0) {
                bgImageEl.value = '/assets/mympd-background-dark.svg';
            }
        }
    }, false);
    
    document.getElementById('selectLocale').addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        warnLocale(value);
    }, false);

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
        removeIsInvalid(document.getElementById('modalSettings'));
    });
    
    document.getElementById('modalQueueSettings').addEventListener('shown.bs.modal', function () {
        getSettings();
        removeIsInvalid(document.getElementById('modalQueueSettings'));
    });

    document.getElementById('modalConnection').addEventListener('shown.bs.modal', function () {
        getSettings();
        removeIsInvalid(document.getElementById('modalConnection'));
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            const value = getAttDec(document.getElementById('btnJukeboxModeGroup').getElementsByClassName('active')[0], 'data-value');
            if (value === '0') {
                disableEl('inputJukeboxQueueLength');
                disableEl('selectJukeboxPlaylist');
            }
            else if (value === '2') {
                disableEl('inputJukeboxQueueLength');
                disableEl('selectJukeboxPlaylist');
                document.getElementById('selectJukeboxPlaylist').value = 'Database';
            }
            else if (value === '1') {
                enableEl('inputJukeboxQueueLength');
                enableEl('selectJukeboxPlaylist');
            }
            if (value !== '0') {
                toggleBtnChk('btnConsume', true);            
            }
            checkConsume();
        }, 100);
    });
    
    document.getElementById('btnConsume').addEventListener('mouseup', function() {
        setTimeout(function() { 
            checkConsume(); 
        }, 100);
    });
    
    document.getElementById('btnStickers').addEventListener('mouseup', function() {
        setTimeout(function() {
            if (document.getElementById('btnStickers').classList.contains('active')) {
                document.getElementById('warnPlaybackStatistics').classList.add('hide');
                enableEl('inputJukeboxLastPlayed');
            }
            else {
                document.getElementById('warnPlaybackStatistics').classList.remove('hide');
                disableEl('inputJukeboxLastPlayed');
            }
        }, 100);
    });
}

//eslint-disable-next-line no-unused-vars
function saveConnection() {
    let formOK = true;
    const mpdHostEl = document.getElementById('inputMpdHost');
    const mpdPortEl = document.getElementById('inputMpdPort');
    const mpdPassEl = document.getElementById('inputMpdPass');
    let musicDirectory = getSelectValue('selectMusicDirectory');
    
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
    if (formOK === true) {
        sendAPI("MYMPD_API_CONNECTION_SAVE", {
            "mpdHost": mpdHostEl.value,
            "mpdPort": mpdPortEl.value,
            "mpdPass": mpdPassEl.value,
            "musicDirectory": musicDirectory
        }, getSettings);
        uiElements.modalConnection.hide();    
    }
}

function getSettings(onerror) {
    if (settingsLock === false) {
        settingsLock = true;
        sendAPI("MYMPD_API_SETTINGS_GET", {}, getMpdSettings, onerror);
    }
}

function getMpdSettings(obj) {
    if (obj !== '' && obj.result) {
        settingsNew = obj.result;
        document.getElementById('splashScreenAlert').innerText = t('Fetch MPD settings');
        sendAPI("MPD_API_SETTINGS_GET", {}, joinSettings, true);
    }
    else {
        settingsParsed = 'error';
        if (appInited === false) {
            showAppInitAlert(obj === '' ? t('Can not parse settings') : t(obj.error.message));
        }
        return false;
    }
}

function joinSettings(obj) {
    if (obj !== '' && obj.result) {
        for (const key in obj.result) {
            settingsNew[key] = obj.result[key];
        }
    }
    else {
        settingsParsed = 'error';
        if (appInited === false) {
            showAppInitAlert(obj === '' ? t('Can not parse settings') : t(obj.error.message));
        }
        settingsNew.mpdConnected = false;
    }
    settings = Object.assign({}, settingsNew);
    settingsLock = false;
    parseSettings();
    toggleUI();
    btnWaiting(document.getElementById('btnApplySettings'), false);
}

function checkConsume() {
    const stateConsume = document.getElementById('btnConsume').classList.contains('active') ? true : false;
    const stateJukeboxMode = getBtnGroupValue('btnJukeboxModeGroup');
    if (stateJukeboxMode > 0 && stateConsume === false) {
        document.getElementById('warnConsume').classList.remove('hide');
    }
    else {
        document.getElementById('warnConsume').classList.add('hide');
    }
}

function parseSettings() {
    if ('serviceWorker' in navigator && settings.mympdVersion !== myMPDversion) {
        logWarn('Server version (' + settings.mympdVersion + ') not equal client version (' + myMPDversion + '), reloading');
        clearAndReload();
    }

    if (document.getElementById('modalSettings').classList.contains('show')) {
        //execute only if settings modal is displayed
        getBgImageList(settings.bgImage);
    }

    if (settings.bgImage.indexOf('/assets/') === 0) {
        domCache.body.style.backgroundImage = 'url("' + subdir + settings.bgImage + '")';
    }
    else if (settings.bgImage !== '') {
        domCache.body.style.backgroundImage = 'url("' + subdir + '/pics/' + settings.bgImage + '")';
    }
    else {
        domCache.body.style.backgroundImage = '';
    }

    if (settings.locale === 'default') {
        locale = navigator.language || navigator.userLanguage;
    }
    else {
        locale = settings.locale;
    }
    warnLocale(settings.locale);
    
    if (isMobile === true) {    
        document.getElementById('inputScaleRatio').value = scale;
    }

    let setTheme = settings.theme;
    if (settings.theme === 'theme-autodetect') {
        setTheme = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'theme-dark' : 'theme-default';
    }    

    Object.keys(themes).forEach(function(key) {
        if (key === setTheme) {
            domCache.body.classList.add(key);
        }
        else {
            domCache.body.classList.remove(key);
        }
    });

    setNavbarIcons();

    document.getElementById('selectTheme').value = settings.theme;

    //build form for advanced settings    
    for (const key in advancedSettingsDefault) {
        if (!settings.advanced[key]) {
            settings.advanced[key] = advancedSettingsDefault[key].defaultValue;
        }
    }

    const advFrm = {};
    
    const advSettingsKeys = Object.keys(settings.advanced);
    advSettingsKeys.sort();
    for (let i = 0; i < advSettingsKeys.length; i++) {
        const key = advSettingsKeys[i];
        if (advancedSettingsDefault[key] === undefined) {
            continue;
        }
        const form = advancedSettingsDefault[key].form;
        if (advFrm[form] === undefined) {
            advFrm[form] = '';
        }
        
        if (advancedSettingsDefault[key].inputType === 'section') {
            if (advancedSettingsDefault[key].title !== undefined) {
                advFrm[form] += '<hr/><h4>' + t(advancedSettingsDefault[key].title) + '</h4>';
            }
            else if (advancedSettingsDefault[key].subtitle !== undefined) {
                advFrm[form] += '<h5>' + t(advancedSettingsDefault[key].subtitle) + '</h5>';
            }
            continue;
        }
        advFrm[form] += '<div class="form-group row">' +
                    '<label class="col-sm-4 col-form-label" for="inputAdvSetting' + r(key) + '" data-phrase="' + 
                    e(advancedSettingsDefault[key].title) + '">' + t(advancedSettingsDefault[key].title) + '</label>' +
                    '<div class="col-sm-8 ">';
        if (advancedSettingsDefault[key].inputType === 'select') {
            advFrm[form] += '<select id="inputAdvSetting' + r(key) + '" data-key="' + 
                r(key) + '" class="form-control border-secondary custom-select">';
            for (const value in advancedSettingsDefault[key].validValues) {
                advFrm[form] += '<option value="' + e(value) + '"' +
                    (settings.advanced[key] === value ? ' selected' : '') +
                    '>' + t(advancedSettingsDefault[key].validValues[value]) + '</option>';
            }
            advFrm[form] += '</select>';
        }
        else if (advancedSettingsDefault[key].inputType === 'checkbox') {
            advFrm[form] += '<button type="button" class="btn btn-sm btn-secondary mi ' + 
                (settings.advanced[key] === false ? '' : 'active') + ' clickable" id="inputAdvSetting' + r(key) + '"'+
                'data-key="' + r(key) + '">' +
                (settings.advanced[key] === false ? 'radio_button_unchecked' : 'check') + '</button>';
        }
        else {
            advFrm[form] += '<input id="inputAdvSetting' + r(key) + '" data-key="' + 
                r(key) + '" type="text" class="form-control border-secondary" value="' + e(settings.advanced[key]) + '">';
        }
        advFrm[form] += '</div></div>';
    }
    for (const key in advFrm) {
        document.getElementById(key).innerHTML = advFrm[key];
        const advFrmBtns = document.getElementById(key).getElementsByTagName('button');
        for (const btn of advFrmBtns) {
            btn.addEventListener('click', function(event) {
                toggleBtnChk(event.target);
            }, false);
        }
    }

    if (settings.advanced.uiFooterQueueSettings === true) {
        document.getElementById('footerQueueSettings').classList.remove('hide');
    }
    else {
        document.getElementById('footerQueueSettings').classList.add('hide');
    }

    if (settings.advanced.uiFooterPlaybackControls === 'both') {
        document.getElementById('btnStop').classList.remove('hide');
    }
    else {
        document.getElementById('btnStop').classList.add('hide');
    }

    //set local playback url    
    if (settings.advanced.uiLocalPlayback === true) {
        setLocalPlayerUrl();
    }
    
    //parse mpd settings if connected
    if (settings.mpdConnected === true) {
        parseMPDSettings();
    }
    
    //Info in about modal
    if (settings.mpdHost.indexOf('/') !== 0) {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost + ':' + settings.mpdPort;
    }
    else {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost;
    }
    
    //connection modal
    document.getElementById('inputMpdHost').value = settings.mpdHost;
    document.getElementById('inputMpdPort').value = settings.mpdPort;
    document.getElementById('inputMpdPass').value = settings.mpdPass;

    //web notifications - check permission
    const btnNotifyWeb = document.getElementById('btnNotifyWeb');
    document.getElementById('warnNotifyWeb').classList.add('hide');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.notificationWeb === true) {
                document.getElementById('warnNotifyWeb').classList.remove('hide');
            }
            settings.notificationWeb = false;
        }
        if (Notification.permission === 'denied') {
            document.getElementById('warnNotifyWeb').classList.remove('hide');
        }
        toggleBtnChk('btnNotifyWeb', settings.notificationWeb);
        enableEl(btnNotifyWeb);
    }
    else {
        disableEl(btnNotifyWeb);
        toggleBtnChk('btnNotifyWeb', false);
    }
    
    toggleBtnChk('btnNotifyPage', settings.notificationPage);
    toggleBtnChk('btnMediaSession', settings.mediaSession);
    toggleBtnChk('btnFeatTimer', settings.featTimer);
    toggleBtnChk('btnFeatLyrics', settings.featLyrics);
    toggleBtnChk('btnFeatHome', settings.featHome);

    document.getElementById('inputBookletName').value = settings.bookletName;
    
    document.getElementById('selectLocale').value = settings.locale;
    document.getElementById('inputCoverimageName').value = settings.coverimageName;

    document.getElementById('inputCoverimageSize').value = settings.coverimageSize;
    document.getElementById('inputCoverimageSizeSmall').value = settings.coverimageSizeSmall;

    document.documentElement.style.setProperty('--mympd-coverimagesize', settings.coverimageSize + "px");
    document.documentElement.style.setProperty('--mympd-coverimagesizesmall', settings.coverimageSizeSmall + "px");
    document.documentElement.style.setProperty('--mympd-highlightcolor', settings.highlightColor);
    
    document.getElementById('inputHighlightColor').value = settings.highlightColor;
    document.getElementById('inputBgColor').value = settings.bgColor;
    domCache.body.style.backgroundColor = settings.bgColor;
    
    toggleBtnChkCollapse('btnBgCover', 'collapseBackground', settings.bgCover);
    document.getElementById('inputBgCssFilter').value = settings.bgCssFilter;    

    const albumartbg = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < albumartbg.length; i++) {
        albumartbg[i].style.filter = settings.bgCssFilter;
    }

    toggleBtnChkCollapse('btnLoveEnable', 'collapseLove', settings.love);
    document.getElementById('inputLoveChannel').value = settings.loveChannel;
    document.getElementById('inputLoveMessage').value = settings.loveMessage;
    
    //default limit for all apps
    //convert from string to int
    const limit = parseInt(settings.advanced.uiMaxElementsPerPage);
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
    
    toggleBtnChk('btnStickers', settings.stickers);
    document.getElementById('inputLastPlayedCount').value = settings.lastPlayedCount;
    
    toggleBtnChkCollapse('btnSmartpls', 'collapseSmartpls', settings.smartpls);

    if (settings.advanced.uiLocalPlayback === false) {
        settings.featLocalPlayback = false;    
    }
    const features = ["featLocalPlayback", "featCacert", "featRegex", "featTimer", "featLyrics", 
        "featScripting", "featScripteditor", "featHome"];
    for (let j = 0; j < features.length; j++) {
        const Els = document.getElementsByClassName(features[j]);
        const ElsLen = Els.length;
        const displayEl = settings[features[j]] === true ? '' : 'none';
        for (let i = 0; i < ElsLen; i++) {
            Els[i].style.display = displayEl;
        }
    }
    
    const readonlyEls = document.getElementsByClassName('warnReadonly');
    for (let i = 0; i < readonlyEls.length; i++) {
        if (settings.readonly === false) {
            readonlyEls[i].classList.add('hide');
        }
        else {
            readonlyEls[i].classList.remove('hide');
        }
    }
    if (settings.readonly === true) {
        document.getElementsByClassName('groupClearCovercache')[0].classList.add('hide');
    }
    else {
        document.getElementsByClassName('groupClearCovercache')[0].classList.remove('hide');
    }
    
    let timerActions = '<optgroup data-value="player" label="' + t('Playback') + '">' +
        '<option value="startplay">' + t('Start playback') + '</option>' +
        '<option value="stopplay">' + t('Stop playback') + '</option>' +
        '</optgroup>';

    if (settings.featScripting === true) {
        getScriptList(true);
    }
    else {
        document.getElementById('scripts').innerHTML = '';
        //reinit mainmenu -> change of script list
        uiElements.dropdownMainMenu.dispose();
        uiElements.dropdownMainMenu = new BSN.Dropdown(document.getElementById('mainMenu'));
    }

    document.getElementById('selectTimerAction').innerHTML = timerActions;
    
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.jukeboxMode);
    document.getElementById('selectJukeboxUniqueTag').value = settings.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.jukeboxLastPlayed;
    
    if (settings.jukeboxMode === 0) {
        disableEl('inputJukeboxQueueLength');
        disableEl('selectJukeboxPlaylist');
    }
    else if (settings.jukeboxMode === 2) {
        disableEl('inputJukeboxQueueLength');
        disableEl('selectJukeboxPlaylist');
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
    }
    else if (settings.jukeboxMode === 1) {
        enableEl('inputJukeboxQueueLength');
        enableEl('selectJukeboxPlaylist');
    }

    document.getElementById('inputSmartplsPrefix').value = settings.smartplsPrefix;
    document.getElementById('inputSmartplsInterval').value = settings.smartplsInterval / 60 / 60;
    document.getElementById('selectSmartplsSort').value = settings.smartplsSort;

    document.getElementById('volumeBar').setAttribute('min', settings.volumeMin);
    document.getElementById('volumeBar').setAttribute('max', settings.volumeMax);

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

    i18nHtml(domCache.body);

    checkConsume();

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

    settingsParsed = 'true';
}

function parseMPDSettings() {
    if (document.getElementById('modalQueueSettings').classList.contains('show')) {
        //execute only if queueSettings modal is shown
        if (settings.featPlaylists === true) {
            sendAPI("MPD_API_PLAYLIST_LIST", {"searchstr": "", "offset": 0, "limit": 0}, function(obj) {
                getAllPlaylists(obj, 'selectJukeboxPlaylist', settings.jukeboxPlaylist);
            });
        }
        else {
            document.getElementById('selectJukeboxPlaylist').innerHTML = '<option value="Database">' + t('Database') + '</option>';
        }
    }

    toggleBtnChk('btnRandom', settings.random);
    toggleBtnChk('btnConsume', settings.consume);
    toggleBtnChk('btnRepeat', settings.repeat);
    toggleBtnChk('btnAutoPlay', settings.autoPlay);

    toggleBtnGroupValue(document.getElementById('btnSingleGroup'), settings.single);
    toggleBtnGroupValue(document.getElementById('btnReplaygainGroup'), settings.replaygain);

    document.getElementById('partitionName').innerText = settings.partition;
    
    document.getElementById('inputCrossfade').value = settings.crossfade;
    
    if (settings.featLibrary === true && settings.publish === true) {
        settings['featBrowse'] = true;    
    }
    else {
        settings['featBrowse'] = false;
    }

    const features = ['featStickers', 'featSmartpls', 'featPlaylists', 'featTags', 'featAdvsearch',
        'featLove', 'featSingleOneshot', 'featBrowse', 'featMounts', 'featNeighbors',
        'featPartitions'];
    for (let j = 0; j < features.length; j++) {
        const Els = document.getElementsByClassName(features[j]);
        const ElsLen = Els.length;
        let displayEl = settings[features[j]] === true ? '' : 'none';
        for (let i = 0; i < ElsLen; i++) {
            Els[i].style.display = displayEl;
        }
    }
    
    if (settings.featPlaylists === false && settings.smartpls === true) {
        document.getElementById('warnSmartpls').classList.remove('hide');
    }
    else {
        document.getElementById('warnSmartpls').classList.add('hide');
    }
    
    if (settings.featPlaylists === true && settings.readonly === false) {
        enableEl('btnSmartpls');
    }
    else {
        disableEl('btnSmartpls');
    }

    if (settings.featStickers === false && settings.stickers === true) {
        document.getElementById('warnStickers').classList.remove('hide');
    }
    else {
        document.getElementById('warnStickers').classList.add('hide');
    }
    
    if (settings.featStickers === false || settings.stickers === false) {
        document.getElementById('warnPlaybackStatistics').classList.remove('hide');
        disableEl('inputJukeboxLastPlayed');
    }
    else {
        document.getElementById('warnPlaybackStatistics').classList.add('hide');
        enableEl('inputJukeboxLastPlayed');
    }
    
    if (settings.featLove === false && settings.love === true) {
        document.getElementById('warnScrobbler').classList.remove('hide');
    }
    else {
        document.getElementById('warnScrobbler').classList.add('hide');
    }
    
    if (settings.musicDirectoryValue === '' && settings.musicDirectory !== 'none') {
        document.getElementById('warnMusicDirectory').classList.remove('hide');
    }
    else {
        document.getElementById('warnMusicDirectory').classList.add('hide');
    }

    document.getElementById('warnJukeboxPlaylist').classList.add('hide');

    if (settings.bgCover === true) {
        setBackgroundImage(lastSongObj.uri);
    }
    else {
        clearBackgroundImage();
    }

    let triggerEventList = '';
    Object.keys(settings.triggers).forEach(function(key) {
        triggerEventList += '<option value="' + e(settings.triggers[key]) + '">' + t(key) + '</option>';
    });
    document.getElementById('selectTriggerEvent').innerHTML = triggerEventList;
    
    settings.tags.sort();
    settings.searchtags.sort();
    settings.browsetags.sort();
    filterCols('Search');
    filterCols('QueueCurrent');
    filterCols('QueueLastPlayed');
    filterCols('QueueJukebox');
    filterCols('BrowsePlaylistsDetail');
    filterCols('BrowseFilesystem');
    filterCols('BrowseDatabaseDetail');
    filterCols('Playback');
    
    if (settings.featTags === false) {
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
        for (let i = 0; i < settings.colsPlayback.length; i++) {
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
    }

    if (settings.tags.includes('Title')) {
        app.apps.Search.sort = 'Title';
    }
    
    if (settings.tags.includes('AlbumArtist')) {
        tagAlbumArtist = 'AlbumArtist';        
    }
    else if (settings.tags.includes('Artist')) {
        tagAlbumArtist = 'Artist';        
    }
    
    if (!settings.tags.includes('AlbumArtist') && app.apps.Browse.tabs.Database.views.List.filter === 'AlbumArtist') {
        app.apps.Browse.tabs.Database.views.List.sort = 'Artist';
    }

    if (settings.featAdvsearch === false && app.apps.Browse.active === 'Database') {
        app.apps.Browse.active = 'Filesystem';
    }

    if (settings.featAdvsearch === false) {
        const tagEls = document.getElementById('cardPlaybackTags').getElementsByTagName('p');
        for (let i = 0; i < tagEls.length; i++) {
            tagEls[i].classList.remove('clickable');
        }
    }

    setCols('QueueCurrent');
    setCols('Search');
    setCols('QueueLastPlayed');
    setCols('QueueJukebox');
    setCols('BrowseFilesystem');
    setCols('BrowsePlaylistsDetail');
    setCols('BrowseDatabaseDetail');
    setCols('Playback');

    addTagList('BrowseDatabaseByTagDropdown', 'browsetags');
    addTagList('BrowseNavPlaylistsDropdown', 'browsetags');
    addTagList('BrowseNavFilesystemDropdown', 'browsetags');
    
    addTagList('searchqueuetags', 'searchtags');
    addTagList('searchtags', 'searchtags');
    addTagList('searchDatabaseTags', 'browsetags');
    addTagList('databaseSortTagsList', 'browsetags');
    addTagList('dropdownSortPlaylistTags', 'tags');
    addTagList('saveSmartPlaylistSort', 'tags');
    
    addTagListSelect('selectSmartplsSort', 'tags');
    addTagListSelect('saveSmartPlaylistSort', 'tags');
    addTagListSelect('selectJukeboxUniqueTag', 'browsetags');
    
    initTagMultiSelect('inputEnabledTags', 'listEnabledTags', settings.allmpdtags, settings.tags);
    initTagMultiSelect('inputSearchTags', 'listSearchTags', settings.tags, settings.searchtags);
    initTagMultiSelect('inputBrowseTags', 'listBrowseTags', settings.tags, settings.browsetags);
    initTagMultiSelect('inputGeneratePlsTags', 'listGeneratePlsTags', settings.browsetags, settings.generatePlsTags);
}

//eslint-disable-next-line no-unused-vars
function resetSettings() {
    sendAPI("MYMPD_API_SETTINGS_RESET", {}, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveSettings(closeModal) {
    let formOK = true;

    const inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        if (!validateInt(inputCrossfade)) {
            formOK = false;
        }
    }

    const inputJukeboxQueueLength = document.getElementById('inputJukeboxQueueLength');
    if (!validateInt(inputJukeboxQueueLength)) {
        formOK = false;
    }

    const inputJukeboxLastPlayed = document.getElementById('inputJukeboxLastPlayed');
    if (!validateInt(inputJukeboxLastPlayed)) {
        formOK = false;
    }
    
    const inputCoverimageSizeSmall = document.getElementById('inputCoverimageSizeSmall');
    if (!validateInt(inputCoverimageSizeSmall)) {
        formOK = false;
    }

    const inputCoverimageSize = document.getElementById('inputCoverimageSize');
    if (!validateInt(inputCoverimageSize)) {
        formOK = false;
    }
    
    const inputCoverimageName = document.getElementById('inputCoverimageName');
    if (!validateFilenameList(inputCoverimageName)) {
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

    const inputLastPlayedCount = document.getElementById('inputLastPlayedCount');
    if (!validateInt(inputLastPlayedCount)) {
        formOK = false;
    }
    
    if (document.getElementById('btnLoveEnable').classList.contains('active')) {
        const inputLoveChannel = document.getElementById('inputLoveChannel');
        const inputLoveMessage = document.getElementById('inputLoveMessage');
        if (!validateNotBlank(inputLoveChannel) || !validateNotBlank(inputLoveMessage)) {
            formOK = false;
        }
    }

    const inputSmartplsInterval = document.getElementById('inputSmartplsInterval');
    if (!validateInt(inputSmartplsInterval)) {
        formOK = false;
    }
    const smartplsInterval = document.getElementById('inputSmartplsInterval').value * 60 * 60;

    const advSettings = {};
    for (const key in advancedSettingsDefault) {
        const el = document.getElementById('inputAdvSetting' + r(key));
        if (el) {
            if (advancedSettingsDefault[key].inputType === 'select') {
                advSettings[key] = getSelectValue(el);
            }
            else if (advancedSettingsDefault[key].inputType === 'checkbox') {
                advSettings[key] = el.classList.contains('active') ? true : false;
            }
            else {
                advSettings[key] = el.value;
            }
        }
    }
    
    if (formOK === true) {
        sendAPI("MYMPD_API_SETTINGS_SET", {
            "notificationWeb": (document.getElementById('btnNotifyWeb').classList.contains('active') ? true : false),
            "notificationPage": (document.getElementById('btnNotifyPage').classList.contains('active') ? true : false),
            "mediaSession": (document.getElementById('btnMediaSession').classList.contains('active') ? true : false),
            "bgCover": (document.getElementById('btnBgCover').classList.contains('active') ? true : false),
            "bgColor": document.getElementById('inputBgColor').value,
            "bgImage": getSelectValue('selectBgImage'),
            "bgCssFilter": document.getElementById('inputBgCssFilter').value,
            "coverimageName": document.getElementById('inputCoverimageName').value,
            "coverimageSize": document.getElementById('inputCoverimageSize').value,
            "coverimageSizeSmall": document.getElementById('inputCoverimageSizeSmall').value,
            "locale": getSelectValue('selectLocale'),
            "love": (document.getElementById('btnLoveEnable').classList.contains('active') ? true : false),
            "loveChannel": document.getElementById('inputLoveChannel').value,
            "loveMessage": document.getElementById('inputLoveMessage').value,
            "stickers": (document.getElementById('btnStickers').classList.contains('active') ? true : false),
            "lastPlayedCount": document.getElementById('inputLastPlayedCount').value,
            "smartpls": (document.getElementById('btnSmartpls').classList.contains('active') ? true : false),
            "smartplsPrefix": document.getElementById('inputSmartplsPrefix').value,
            "smartplsInterval": smartplsInterval,
            "smartplsSort": document.getElementById('selectSmartplsSort').value,
            "taglist": getTagMultiSelectValues(document.getElementById('listEnabledTags'), false),
            "searchtaglist": getTagMultiSelectValues(document.getElementById('listSearchTags'), false),
            "browsetaglist": getTagMultiSelectValues(document.getElementById('listBrowseTags'), false),
            "generatePlsTags": getTagMultiSelectValues(document.getElementById('listGeneratePlsTags'), false),
            "theme": getSelectValue('selectTheme'),
            "highlightColor": document.getElementById('inputHighlightColor').value,
            "timer": (document.getElementById('btnFeatTimer').classList.contains('active') ? true : false),
            "bookletName": document.getElementById('inputBookletName').value,
            "lyrics": (document.getElementById('btnFeatLyrics').classList.contains('active') ? true : false),
            "advanced": advSettings,
            "featHome": (document.getElementById('btnFeatHome').classList.contains('active') ? true : false)
        }, getSettings);
        if (closeModal === true) {
            uiElements.modalSettings.hide();
        }
        else {
            btnWaiting(document.getElementById('btnApplySettings'), true);
        }
    }
}

//eslint-disable-next-line no-unused-vars
function saveQueueSettings() {
    let formOK = true;

    const inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        if (!validateInt(inputCrossfade)) {
            formOK = false;
        }
    }

    const inputJukeboxQueueLength = document.getElementById('inputJukeboxQueueLength');
    if (!validateInt(inputJukeboxQueueLength)) {
        formOK = false;
    }

    const inputJukeboxLastPlayed = document.getElementById('inputJukeboxLastPlayed');
    if (!validateInt(inputJukeboxLastPlayed)) {
        formOK = false;
    }
    
    const singleState = getBtnGroupValue('btnSingleGroup');
    const jukeboxMode = getBtnGroupValue('btnJukeboxModeGroup');
    const replaygain = getBtnGroupValue('btnReplaygainGroup');
    let jukeboxUniqueTag = getSelectValue('selectJukeboxUniqueTag');
    const jukeboxPlaylist = getSelectValue('selectJukeboxPlaylist');
    
    if (jukeboxMode === '2') {
        jukeboxUniqueTag = 'Album';
    }
    
    if (formOK === true) {
        sendAPI("MYMPD_API_SETTINGS_SET", {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random": (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single": parseInt(singleState),
            "repeat": (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": replaygain,
            "crossfade": document.getElementById('inputCrossfade').value,
            "jukeboxMode": parseInt(jukeboxMode),
            "jukeboxPlaylist": jukeboxPlaylist,
            "jukeboxQueueLength": parseInt(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": parseInt(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTag,
            "autoPlay": (document.getElementById('btnAutoPlay').classList.contains('active') ? true : false)
        }, getSettings);
        uiElements.modalQueueSettings.hide();
    }
}

function getTagMultiSelectValues(taglist, translated) {
    const values = [];
    const chkBoxes = taglist.getElementsByTagName('button');
    for (let i = 0; i < chkBoxes.length; i++) {
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
    for (let i = 0; i < allTags.length; i++) {
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
    if (getAttDec(inputEl, 'data-init') === 'true') {
        return;
    }
    setAttEnc(inputEl, 'data-init', 'true');
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
    for (let i = 0; i < settings[set].length; i++) {
        if (tags.includes(settings[set][i])) {
            cols.push(settings[set][i]);
        }
    }
    if (x === 'Search') {
        //enforce albumartist and album for albumactions
        if (cols.includes('Album') === false && tags.includes('Album')) {
            cols.push('Album');
        }
        if (cols.includes(tagAlbumArtist) === false && tags.includes(tagAlbumArtist)) {
            cols.push(tagAlbumArtist);
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
                    toggleBtnChk('btnNotifyWeb', true);
                    settings.notificationWeb = true;
                    document.getElementById('warnNotifyWeb').classList.add('hide');
                } 
                else {
                    toggleBtnChk('btnNotifyWeb', false);
                    settings.notificationWeb = false;
                    document.getElementById('warnNotifyWeb').classList.remove('hide');
                }
            });
        }
        else {
            toggleBtnChk('btnNotifyWeb', false);
            settings.notificationWeb = false;
            document.getElementById('warnNotifyWeb').classList.add('hide');
        }
    }
    else {
        toggleBtnChk('btnNotifyWeb', false);
        settings.notificationWeb = false;
    }
}

function setNavbarIcons() {
    const oldBadgeQueueItems = document.getElementById('badgeQueueItems');
    let oldQueueLength = 0;
    if (oldBadgeQueueItems) {
        oldQueueLength = oldBadgeQueueItems.innerText;
    }
    
    let btns = '';
    for (const icon of settings.navbarIcons) {
        let hide = '';
        if (settings.featHome === false && icon.options[0] === 'Home') {
            hide = 'hide';
        }
        btns += '<div id="nav' + icon.options.join('') + '" class="nav-item flex-fill text-center ' + hide + '">' +
          '<a data-title-phrase="' + t(icon.title) + '" data-href="" class="nav-link text-light" href="#">' +
            '<span class="mi">' + icon.ligature + '</span>' + 
            '<span class="navText" data-phrase="' + t(icon.title) + '"></span>' +
            (icon.badge !== '' ? icon.badge : '') +
          '</a>' +
        '</div>';
    }
    const container = document.getElementById('navbar-main');
    container.innerHTML = btns;

    const badgeQueueItemsEl = document.getElementById('badgeQueueItems');
    if (badgeQueueItemsEl) {
        document.getElementById('badgeQueueItems').innerText = oldQueueLength;
    }

    if (document.getElementById('nav' + app.current.app)) {
        document.getElementById('nav' + app.current.app).classList.add('active');
    }

    //cache elements, reused in appPrepare
    domCache.navbarBtns = container.getElementsByTagName('div');
    domCache.navbarBtnsLen = domCache.navbarBtns.length;
    
    for (let i = 0; i < domCache.navbarBtnsLen; i++) {
        setAttEnc(domCache.navbarBtns[i].firstChild, 'data-href', JSON.stringify({"cmd": "appGoto", "options": settings.navbarIcons[i].options}));
    }
}

//eslint-disable-next-line no-unused-vars
function resetValue(elId) {
    const el = document.getElementById(elId);
    el.value = getAttDec(el, 'data-default') !== null ? getAttDec(el, 'data-default') : 
        (getAttDec(el, 'placeholder') !== null ? getAttDec(el, 'placeholder') : '');
}

function getBgImageList(image) {
    getImageList('selectBgImage', image, [
        {"value":"","text":"None"},
        {"value":"/assets/mympd-background-default.svg","text":"Default image"},
        {"value":"/assets/mympd-background-dark.svg","text":"Default image dark"},
        {"value":"/assets/mympd-background-light.svg","text":"Default image light"},
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
            '<a class="alert-link" target="_blank" href="https://github.com/jcorporation/myMPD/discussions/167"><span class="mi">open_in_browser</span>&nbsp;' + t('Help to improve myMPD') + '</a>';
        warnEl.classList.remove('hide');
    }
    else {
        warnEl.classList.add('hide');
    }
}
