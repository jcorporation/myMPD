"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

//eslint-disable-next-line no-unused-vars
function saveConnection() {
    let formOK = true;
    let mpdHostEl = document.getElementById('inputMpdHost');
    let mpdPortEl = document.getElementById('inputMpdPort');
    let mpdPassEl = document.getElementById('inputMpdPass');
    let musicDirectoryEl  = document.getElementById('selectMusicDirectory');
    let musicDirectory = musicDirectoryEl.options[musicDirectoryEl.selectedIndex].value;
    
    if (musicDirectory === 'custom') {
        let musicDirectoryValueEl  = document.getElementById('inputMusicDirectory');
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
        sendAPI("MYMPD_API_CONNECTION_SAVE", {"mpdHost": mpdHostEl.value, "mpdPort": mpdPortEl.value, "mpdPass": mpdPassEl.value, "musicDirectory": musicDirectory}, getSettings);
        modalConnection.hide();    
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
        for (let key in obj.result) {
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
}

function parseSettings() {
    if (settings.locale === 'default') {
        locale = navigator.language || navigator.userLanguage;
    }
    else {
        locale = settings.locale;
    }

    if (settings.mpdConnected === true) {
        parseMPDSettings();
    }
    
    if (settings.mpdHost.indexOf('/') !== 0) {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost + ':' + settings.mpdPort;
    }
    else {
        document.getElementById('mpdInfo_host').innerText = settings.mpdHost;
    }
    
    document.getElementById('inputMpdHost').value = settings.mpdHost;
    document.getElementById('inputMpdPort').value = settings.mpdPort;
    document.getElementById('inputMpdPass').value = settings.mpdPass;

    let btnNotifyWeb = document.getElementById('btnNotifyWeb');
    if (notificationsSupported()) {
        if (settings.notificationWeb) {
            toggleBtnChk('btnNotifyWeb', settings.notificationWeb);
            Notification.requestPermission(function (permission) {
                if (!('permission' in Notification)) {
                    Notification.permission = permission;
                }
                if (permission === 'granted') {
                    toggleBtnChk('btnNotifyWeb', true);
                } 
                else {
                    toggleBtnChk('btnNotifyWeb', false);
                    settings.notificationWeb = true;
                }
            });         
        }
        else {
            toggleBtnChk('btnNotifyWeb', false);
        }
    }
    else {
        btnNotifyWeb.setAttribute('disabled', 'disabled');
        toggleBtnChk('btnNotifyWeb', false);
    }
    
    toggleBtnChk('btnNotifyPage', settings.notificationPage);
    toggleBtnChk('btnBgCover', settings.bgCover);
    toggleBtnChk('btnFeatLocalplayer', settings.featLocalplayer);
    toggleBtnChk('btnLocalplayerAutoplay', settings.localplayerAutoplay);
    toggleBtnChk('btnBookmarks', settings.featBookmarks);
    if (settings.streamUrl === '') {
        document.getElementById('selectStreamMode').value = 'port';
        document.getElementById('inputStreamUrl').value = settings.streamPort;
    }
    else {
        document.getElementById('selectStreamMode').value = 'url';
        document.getElementById('inputStreamUrl').value = settings.streamUrl;
    }
    toggleBtnChk('btnCoverimage', settings.coverimage);
    document.getElementById('selectLocale').value = settings.locale;
    document.getElementById('inputCoverimageName').value = settings.coverimageName;

    document.getElementById('inputCoverimageSize').value = settings.coverimageSize;

    let albumcover = document.querySelectorAll('.albumcover');
    for (let i = 0; i < albumcover.length; i++) {
	albumcover[i].style.width = settings.coverimageSize;
	albumcover[i].style.height = settings.coverimageSize;
    }
    
    document.getElementById('inputBgColor').value = settings.bgColor;
    document.getElementsByTagName('body')[0].style.backgroundColor = settings.bgColor;

    document.getElementById('inputBgCssFilter').value = settings.bgCssFilter;    

    let albumartbg = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < albumartbg.length; i++) {
	albumartbg[i].style.filter = settings.bgCssFilter;
    }

    toggleBtnChk('btnLoveEnable', settings.love);
    document.getElementById('inputLoveChannel').value = settings.loveChannel;
    document.getElementById('inputLoveMessage').value = settings.loveMessage;
    
    document.getElementById('inputMaxElementsPerPage').value = settings.maxElementsPerPage;
    toggleBtnChk('btnStickers', settings.stickers);
    document.getElementById('inputLastPlayedCount').value = settings.lastPlayedCount;
    toggleBtnChk('btnSmartpls', settings.smartpls);
    
    
    let features = ["featLocalplayer", "featSyscmds", "featMixramp", "featCacert", "featBookmarks"];
    for (let j = 0; j < features.length; j++) {
        let Els = document.getElementsByClassName(features[j]);
        let ElsLen = Els.length;
        let displayEl = settings[features[j]] === true ? '' : 'none';
        for (let i = 0; i < ElsLen; i++) {
            Els[i].style.display = displayEl;
        }
    }
    
    let readonlyEls = document.getElementsByClassName('warnReadonly');
    for (let i = 0; i < readonlyEls.length; i++) {
        if (settings.readonly === false) {
            readonlyEls[i].classList.add('hide');
        }
        else {
            readonlyEls[i].classList.remove('hide');
        }
    }
    if (settings.readonly == true) {
        document.getElementById('btnBookmarks').setAttribute('disabled', 'disabled');
    }
    else {
        document.getElementById('btnBookmarks').removeAttribute('disabled');
    }

    if (settings.featSyscmds) {
        let syscmdsMaxListLen = 4;
        let syscmdsList = '';
        let syscmdsListLen = settings.syscmdList.length;
        if (syscmdsListLen > 0) {
            syscmdsList = syscmdsListLen > syscmdsMaxListLen ? '' : '<div class="dropdown-divider"></div>';
            for (let i = 0; i < syscmdsListLen; i++) {
                if (settings.syscmdList[i] === 'HR') {
                    syscmdsList += '<div class="dropdown-divider"></div>';
                }
                else {
                    syscmdsList += '<a class="dropdown-item text-light bg-dark alwaysEnabled" href="#" data-href=\'{"cmd": "execSyscmd", "options": ["' + 
                        e(settings.syscmdList[i]) + '"]}\'>' + e(settings.syscmdList[i]) + '</a>';
                }
            }
        }
        document.getElementById('syscmds').innerHTML = syscmdsList;
        if (syscmdsListLen > syscmdsMaxListLen) {
            document.getElementById('navSyscmds').classList.remove('hide');
            document.getElementById('syscmds').classList.add('collapse', 'menu-indent');
        }
        else {
            document.getElementById('navSyscmds').classList.add('hide');
            document.getElementById('syscmds').classList.remove('collapse', 'menu-indent');
        }
    }
    else {
        document.getElementById('syscmds').innerHTML = '';
    }

    dropdownMainMenu = new Dropdown(document.getElementById('mainMenu'));
    
    document.getElementById('selectJukeboxMode').value = settings.jukeboxMode;
    document.getElementById('inputJukeboxQueueLength').value = settings.jukeboxQueueLength;
    
    if (settings.jukeboxMode === 0) {
        document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
    }
    else if (settings.jukeboxMode === 2) {
        document.getElementById('inputJukeboxQueueLength').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').setAttribute('disabled', 'disabled');
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
    }
    else if (settings.jukeboxMode === 1) {
        document.getElementById('inputJukeboxQueueLength').removeAttribute('disabled');
        document.getElementById('selectJukeboxPlaylist').removeAttribute('disabled');
    }

    if (settings.featLocalplayer === true) {
        if (settings.streamUrl === '') {
            settings.mpdstream = 'http://';
            if (settings.mpdHost.match(/^127\./) !== null || settings.mpdHost === 'localhost' || settings.mpdHost.match(/^\//) !== null) {
                settings.mpdstream += window.location.hostname;
            }
            else {
                settings.mpdstream += settings.mpdHost;
            }
            settings.mpdstream += ':' + settings.streamPort + '/';
        } 
        else {
            settings.mpdstream = settings.streamUrl;
        }
        let localPlayer = document.getElementById('localPlayer');
        if (localPlayer.src !== settings.mpdstream) {
            localPlayer.pause();
            document.getElementById('alertLocalPlayback').classList.remove('hide');
            localPlayer.src = settings.mpdstream;
            localPlayer.load();
        }
    }
    
    
    if (settings.musicDirectory === 'auto') {
        document.getElementById('selectMusicDirectory').value = settings.musicDirectory;
        document.getElementById('inputMusicDirectory').value = settings.musicDirectoryValue;
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

    if (app.current.app === 'Queue' && app.current.tab === 'Current') {
        getQueue();
    }
    else if (app.current.app === 'Queue' && app.current.tab === 'LastPlayed') {
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

    i18nHtml(document.getElementsByTagName('body')[0]);

    settingsParsed = 'true';
}

function parseMPDSettings() {
    toggleBtnChk('btnRandom', settings.random);
    toggleBtnChk('btnConsume', settings.consume);
    toggleBtnChk('btnSingle', settings.single);
    toggleBtnChk('btnRepeat', settings.repeat);
    toggleBtnChk('btnAutoPlay', settings.autoPlay);
    
    if (settings.crossfade !== undefined) {
        document.getElementById('inputCrossfade').removeAttribute('disabled');
        document.getElementById('inputCrossfade').value = settings.crossfade;
    }
    else {
        document.getElementById('inputCrossfade').setAttribute('disabled', 'disabled');
    }
    if (settings.mixrampdb !== undefined) {
        document.getElementById('inputMixrampdb').removeAttribute('disabled');
        document.getElementById('inputMixrampdb').value = settings.mixrampdb;
    }
    else {
        document.getElementById('inputMixrampdb').setAttribute('disabled', 'disabled');
    }
    if (settings.mixrampdelay !== undefined) {
        document.getElementById('inputMixrampdelay').removeAttribute('disabled');
        document.getElementById('inputMixrampdelay').value = settings.mixrampdelay;
    }
    else {
        document.getElementById('inputMixrampdelay').setAttribute('disabled', 'disabled');
    }

    document.getElementById('selectReplaygain').value = settings.replaygain;

    let features = ['featStickers', 'featSmartpls', 'featPlaylists', 'featTags', 'featCoverimage', 'featAdvsearch',
        'featLove'];
    for (let j = 0; j < features.length; j++) {
        let Els = document.getElementsByClassName(features[j]);
        let ElsLen = Els.length;
        let displayEl = settings[features[j]] === true ? '' : 'none';
        if (features[j] === 'featCoverimage' && settings.coverimage === false) {
            displayEl = 'none';
        }
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
        document.getElementById('btnSmartpls').removeAttribute('disabled');
    }
    else {
        document.getElementById('btnSmartpls').setAttribute('disabled', 'disabled');
    }

    if (settings.featStickers === false && settings.stickers === true) {
        document.getElementById('warnStickers').classList.remove('hide');
    }
    else {
        document.getElementById('warnStickers').classList.add('hide');
    }
    
    if (settings.featLove === false && settings.love === true) {
        document.getElementById('warnScrobbler').classList.remove('hide');
    }
    else {
        document.getElementById('warnScrobbler').classList.add('hide');
    }
    
    if (settings.featLibrary === false && settings.coverimage === true) {
        document.getElementById('warnAlbumart').classList.remove('hide');
    }
    else {
        document.getElementById('warnAlbumart').classList.add('hide');
    }
    if (settings.musicDirectoryValue === '' && settings.musicDirectory !== 'none') {
        document.getElementById('warnMusicDirectory').classList.remove('hide');
    }
    else {
        document.getElementById('warnMusicDirectory').classList.add('hide');
    }

    if (settings.bgCover === true && settings.featCoverimage === true && settings.coverimage === true) {
        if (lastSongObj.cover && lastSongObj.cover.indexOf('coverimage-') > -1 ) {
            clearBackgroundImage();
        }
        else if (lastSongObj.cover) {
             setBackgroundImage(lastSongObj.cover);
        }
        else {
            clearBackgroundImage();
        }
    }
    else {
        clearBackgroundImage();
    }
    
    if (settings.featTags === false) {
        app.apps.Browse.active = 'Filesystem';
        app.apps.Search.state = '0/filename/-/';
        app.apps.Queue.state = '0/filename/-/';
        settings.colsQueueCurrent = ["Pos", "Title", "Duration"];
        settings.colsQueueLastPlayed = ["Pos", "Title", "LastPlayed"];
        settings.colsSearch = ["Title", "Duration"];
        settings.colsBrowseFilesystem = ["Type", "Title", "Duration"];
        settings.colsBrowseDatabase = ["Track", "Title", "Duration"];
        settings.colsPlayback = [];
    }
    else {
        let pbtl = '';
        for (let i = 0; i < settings.colsPlayback.length; i++) {
            pbtl += '<div id="current' + settings.colsPlayback[i]  + '" data-tag="' + settings.colsPlayback[i] + '" '+
                    'data-name="' + (lastSongObj[settings.colsPlayback[i]] ? encodeURI(lastSongObj[settings.colsPlayback[i]]) : '') + '">' +
                    '<small>' + t(settings.colsPlayback[i]) + '</small>' +
                    '<h4';
            if (settings.browsetags.includes(settings.colsPlayback[i])) {
                pbtl += ' class="clickable"';
            }
            pbtl += '>' + (lastSongObj[settings.colsPlayback[i]] ? e(lastSongObj[settings.colsPlayback[i]]) : '') + '</h4></div>';
        }
        document.getElementById('cardPlaybackTags').innerHTML = pbtl;
    }

    if (!settings.tags.includes('AlbumArtist') && settings.featTags) {
        if (settings.tags.includes('Artist')) {
            app.apps.Browse.tabs.Database.active = 'Artist';
        }
        else {
            app.apps.Browse.tabs.Database.active = settings.tags[0];
        }
    }
    if (settings.tags.includes('Title')) {
        app.apps.Search.state = '0/any/Title/';
    }
    
    if (settings.featPlaylists) {
        playlistEl = 'selectJukeboxPlaylist';
        sendAPI("MPD_API_PLAYLIST_LIST", {"offset": 0, "filter": "-"}, getAllPlaylists);
    }
    else {
        document.getElementById('selectJukeboxPlaylist').innerHTML = '<option value="Database">' + t('Database') + '</option>';
    }

    settings.tags.sort();
    settings.searchtags.sort();
    settings.browsetags.sort();
    filterCols('colsSearch');
    filterCols('colsQueueCurrent');
    filterCols('colsQueueLastPlayed');
    filterCols('colsBrowsePlaylistsDetail');
    filterCols('colsBrowseFilesystem');
    filterCols('colsBrowseDatabase');
    filterCols('colsPlayback');
    
    setCols('QueueCurrent');
    setCols('Search');
    setCols('QueueLastPlayed');
    setCols('BrowseFilesystem');
    setCols('BrowsePlaylistsDetail');
    setCols('BrowseDatabase', '.tblAlbumTitles');
    setCols('Playback');

    addTagList('BrowseDatabaseByTagDropdown', 'browsetags');
    addTagList('searchqueuetags', 'searchtags');
    addTagList('searchtags', 'searchtags');
    
    for (let i = 0; i < settings.tags.length; i++) {
        app.apps.Browse.tabs.Database.views[settings.tags[i]] = { "state": "0/-/-/", "scrollPos": 0 };
    }
    
    initTagMultiSelect('inputEnabledTags', 'listEnabledTags', settings.allmpdtags, settings.tags);
    initTagMultiSelect('inputSearchTags', 'listSearchTags', settings.tags, settings.searchtags);
    initTagMultiSelect('inputBrowseTags', 'listBrowseTags', settings.tags, settings.browsetags);
}

//eslint-disable-next-line no-unused-vars
function resetSettings() {
    sendAPI("MYMPD_API_SETTINGS_RESET", {}, getSettings);
}

//eslint-disable-next-line no-unused-vars
function saveSettings() {
    let formOK = true;

    let inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        if (!validateInt(inputCrossfade)) {
            formOK = false;
        }
    }

    let inputJukeboxQueueLength = document.getElementById('inputJukeboxQueueLength');
    if (!validateInt(inputJukeboxQueueLength)) {
        formOK = false;
    }
    
    let selectStreamModeEl = document.getElementById('selectStreamMode');
    let streamUrl = '';
    let streamPort = '';
    let inputStreamUrl = document.getElementById('inputStreamUrl');
    if (selectStreamModeEl.options[selectStreamModeEl.selectedIndex].value === 'port') {
        streamPort = inputStreamUrl.value;
        if (!validateInt(inputStreamUrl)) {
            formOK = false;
        }
    }
    else {
        streamUrl = inputStreamUrl.value;
        if (!validateStream(inputStreamUrl)) {
            formOK = false;
        }
    }

    let inputCoverimageSize = document.getElementById('inputCoverimageSize');
    if (!validateInt(inputCoverimageSize)) {
        formOK = false;
    }
    
    let inputCoverimageName = document.getElementById('inputCoverimageName');
    if (!validateFilename(inputCoverimageName)) {
        formOK = false;
    }
    
    let inputMaxElementsPerPage = document.getElementById('inputMaxElementsPerPage');
    if (!validateInt(inputMaxElementsPerPage)) {
        formOK = false;
    }
    if (parseInt(inputMaxElementsPerPage.value) > 200) {
        formOK = false;
    }
    
    let inputLastPlayedCount = document.getElementById('inputLastPlayedCount');
    if (!validateInt(inputLastPlayedCount)) {
        formOK = false;
    }
    
    if (document.getElementById('btnLoveEnable').classList.contains('active')) {
        let inputLoveChannel = document.getElementById('inputLoveChannel');
        let inputLoveMessage = document.getElementById('inputLoveMessage');
        if (!validateNotBlank(inputLoveChannel) || !validateNotBlank(inputLoveMessage)) {
            formOK = false;
        }
    }

    if (settings.featMixramp === true) {
        let inputMixrampdb = document.getElementById('inputMixrampdb');
        if (!inputMixrampdb.getAttribute('disabled')) {
            if (!validateFloat(inputMixrampdb)) {
                formOK = false;
            } 
        }
        let inputMixrampdelay = document.getElementById('inputMixrampdelay');
        if (!inputMixrampdelay.getAttribute('disabled')) {
            if (inputMixrampdelay.value === 'nan') {
                inputMixrampdelay.value = '-1';
            }
            if (!validateFloat(inputMixrampdelay)) {
                formOK = false;
            }
        }
    }
    
    if (formOK === true) {
        let selectReplaygain = document.getElementById('selectReplaygain');
        let selectJukeboxPlaylist = document.getElementById('selectJukeboxPlaylist');
        let selectJukeboxMode = document.getElementById('selectJukeboxMode');
        let selectLocale = document.getElementById('selectLocale');
        sendAPI("MYMPD_API_SETTINGS_SET", {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random": (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single": (document.getElementById('btnSingle').classList.contains('active') ? 1 : 0),
            "repeat": (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": selectReplaygain.options[selectReplaygain.selectedIndex].value,
            "crossfade": document.getElementById('inputCrossfade').value,
            "mixrampdb": (settings.featMixramp === true ? document.getElementById('inputMixrampdb').value : settings.mixrampdb),
            "mixrampdelay": (settings.featMixramp === true ? document.getElementById('inputMixrampdelay').value : settings.mixrampdelay),
            "notificationWeb": (document.getElementById('btnNotifyWeb').classList.contains('active') ? true : false),
            "notificationPage": (document.getElementById('btnNotifyPage').classList.contains('active') ? true : false),
            "jukeboxMode": parseInt(selectJukeboxMode.options[selectJukeboxMode.selectedIndex].value),
            "jukeboxPlaylist": selectJukeboxPlaylist.options[selectJukeboxPlaylist.selectedIndex].value,
            "jukeboxQueueLength": parseInt(document.getElementById('inputJukeboxQueueLength').value),
            "autoPlay": (document.getElementById('btnAutoPlay').classList.contains('active') ? true : false),
            "bgCover": (document.getElementById('btnBgCover').classList.contains('active') ? true : false),
            "bgColor": document.getElementById('inputBgColor').value,
            "bgCssFilter": document.getElementById('inputBgCssFilter').value,
            "featLocalplayer": (document.getElementById('btnFeatLocalplayer').classList.contains('active') ? true : false),
            "localplayerAutoplay": (document.getElementById('btnLocalplayerAutoplay').classList.contains('active') ? true : false),
            "streamUrl": streamUrl,
            "streamPort": parseInt(streamPort),
            "coverimage": (document.getElementById('btnCoverimage').classList.contains('active') ? true : false),
            "coverimageName": document.getElementById('inputCoverimageName').value,
            "coverimageSize": document.getElementById('inputCoverimageSize').value,
            "locale": selectLocale.options[selectLocale.selectedIndex].value,
            "love": (document.getElementById('btnLoveEnable').classList.contains('active') ? true : false),
            "loveChannel": document.getElementById('inputLoveChannel').value,
            "loveMessage": document.getElementById('inputLoveMessage').value,
            "bookmarks": (document.getElementById('btnBookmarks').classList.contains('active') ? true : false),
            "maxElementsPerPage": document.getElementById('inputMaxElementsPerPage').value,
            "stickers": (document.getElementById('btnStickers').classList.contains('active') ? true : false),
            "lastPlayedCount": document.getElementById('inputLastPlayedCount').value,
            "smartpls": (document.getElementById('btnSmartpls').classList.contains('active') ? true : false),
            "taglist": getTagMultiSelectValues('listEnabledTags'),
            "searchtaglist": getTagMultiSelectValues('listSearchTags'),
            "browsetaglist": getTagMultiSelectValues('listBrowseTags')
        }, getSettings);
        modalSettings.hide();
    }
}

function getTagMultiSelectValues(taglist) {
    let values = [];
    let chkBoxes = document.getElementById(taglist).getElementsByTagName('input');
    for (let i = 0; i < chkBoxes.length; i++) {
        if (chkBoxes[i].checked === true) {
            values.push(chkBoxes[i].name);
        }
    }
    return values.join(',');
}

function initTagMultiSelect(inputId, listId, allTags, enabledTags) {
    let values = [];
    let list = '';
    for (let i = 0; i < allTags.length; i++) {
        if (enabledTags.includes(allTags[i])) {
            values.push(t(allTags[i]));
        }
        list += '<div class="form-check">' +
            '<input class="form-check-input" type="checkbox" value="1" name="' + allTags[i] + '" ' + 
            (enabledTags.includes(allTags[i]) ? 'checked="checked"' : '' )+ '>' +
            '<label class="form-check-label" for="' + allTags[i] + '">&nbsp;&nbsp;' + t(allTags[i]) + '</label>' +
            '</div>';
    }
    document.getElementById(inputId).value = values.join(', ');
    document.getElementById(listId).innerHTML = list;

    document.getElementById(listId).addEventListener('click', function(event) {
        event.stopPropagation();
        if (event.target.nodeName === 'INPUT') {
            let chkBoxes = event.target.parentNode.parentNode.getElementsByTagName('input');
            let values = [];
            for (let i = 0; i < chkBoxes.length; i++) {
                if (chkBoxes[i].checked === true) {
                    values.push(t(chkBoxes[i].name));
                }
            }
            event.target.parentNode.parentNode.parentNode.previousElementSibling.value = values.join(', ');
        }
    });
}

function filterCols(x) {
    let tags = settings.tags.slice();
    if (settings.featTags === false) {
        tags.push('Title');
    }
    tags.push('Duration');
    if (x === 'colsQueueCurrent' || x === 'colsBrowsePlaylistsDetail' || x === 'colsQueueLastPlayed') {
        tags.push('Pos');
    }
    else if (x === 'colsBrowseFilesystem') {
        tags.push('Type');
    }
    if (x === 'colsQueueLastPlayed') {
        tags.push('LastPlayed');
    }
        
    let cols = [];
    for (let i = 0; i < settings[x].length; i++) {
        if (tags.includes(settings[x][i])) {
            cols.push(settings[x][i]);
        }
    }
    settings[x] = cols;
}
