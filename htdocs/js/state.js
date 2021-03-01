"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function parseStats(obj) {
    document.getElementById('mpdstats_artists').innerText =  obj.result.artists;
    document.getElementById('mpdstats_albums').innerText = obj.result.albums;
    document.getElementById('mpdstats_songs').innerText = obj.result.songs;
    document.getElementById('mpdstats_dbPlaytime').innerText = beautifyDuration(obj.result.dbPlaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.result.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.result.uptime);
    document.getElementById('mpdstats_mympd_uptime').innerText = beautifyDuration(obj.result.myMPDuptime);
    document.getElementById('mpdstats_dbUpdated').innerText = localeDate(obj.result.dbUpdated);
    document.getElementById('mympdVersion').innerText = obj.result.mympdVersion;
    document.getElementById('mpdInfo_version').innerText = obj.result.mpdVersion;
    document.getElementById('mpdInfo_libmpdclientVersion').innerText = obj.result.libmpdclientVersion;
    document.getElementById('mpdInfo_libmympdclientVersion').innerText = obj.result.libmympdclientVersion;
}

function getServerinfo() {
    let ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('GET', subdir + '/api/serverinfo', true);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState === 4) {
            let obj = JSON.parse(ajaxRequest.responseText);
            document.getElementById('wsIP').innerText = obj.result.ip;
            document.getElementById('wsMongooseVersion').innerText = obj.result.version;
        }
    };
    ajaxRequest.send();
}

function parseOutputs(obj) {
    let btns = '';
    let nr = 0;
    for (let i = 0; i < obj.result.numOutputs; i++) {
        if (obj.result.data[i].plugin !== 'dummy') {
            nr++;
            btns += '<button id="btnOutput' + obj.result.data[i].id +'" data-output-name="' + encodeURI(obj.result.data[i].name) + '" data-output-id="' + 
                obj.result.data[i].id + '" class="btn btn-secondary btn-block';
            if (obj.result.data[i].state === 1) {
                btns += ' active';
            }
            btns += '"><span class="mi float-left">volume_up</span> ' + e(obj.result.data[i].name);
            if (Object.keys(obj.result.data[i].attributes).length > 0) {
                btns += '<a class="mi float-right text-white" title="' + t('Edit attributes') + '">settings</a>';
            }
            else {
                btns += '<a class="mi float-right text-white" title="' + t('Show attributes') + '">settings</a>';
            }
            btns += '</button>';
        }
    }
    if (nr === 0) {
        btns = '<span class="mi">error_outline</span> ' + t('No outputs');
    }
    document.getElementById('outputs').innerHTML = btns;
}

function showListOutputAttributes(outputName) {
    sendAPI("MPD_API_PLAYER_OUTPUT_LIST", {}, function(obj) {
        modalOutputAttributes.show();
        let output;
        for (let i = 0; i < obj.result.data.length; i++) {
            if (obj.result.data[i].name === outputName) {
                output = obj.result.data[i];
                break;
            }
        }
        document.getElementById('modalOutputAttributesId').value = e(output.id);        
        let list = '<tr><td>' + t('Name') + '</td><td>' + e(output.name) + '</td></tr>' +
            '<tr><td>' + t('State') + '</td><td>' + (output.state === 1 ? t('enabled') : t('disabled')) + '</td></tr>' +
            '<tr><td>' + t('Plugin') + '</td><td>' + e(output.plugin) + '</td></tr>';
        let i = 0;
        Object.keys(output.attributes).forEach(function(key) {
            i++;
            list += '<tr><td>' + e(key) + '</td><td><input name="' + e(key) + '" class="form-control border-secondary" type="text" value="' + 
                e(output.attributes[key]) + '"/></td></tr>';
        });
        if (i > 0) {
            enableEl('btnOutputAttributesSave');
        }
        else {
            disableEl('btnOutputAttributesSave');
        }
        document.getElementById('outputAttributesList').innerHTML = list;
    });
}

//eslint-disable-next-line no-unused-vars
function saveOutputAttributes() {
    let params = {};
    params.outputId =  parseInt(document.getElementById('modalOutputAttributesId').value);
    params.attributes = {};
    let el = document.getElementById('outputAttributesList').getElementsByTagName('input');
    for (let i = 0; i < el.length; i++) {
        params.attributes[el[i].name] = el[i].value;
    }
    sendAPI('MPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET', params);
    modalOutputAttributes.hide();
}

function setCounter(currentSongId, totalTime, elapsedTime) {
    currentSong.totalTime = totalTime;
    currentSong.elapsedTime = elapsedTime;
    currentSong.currentSongId = currentSongId;

    const progressPx = totalTime > 0 ? Math.ceil(domCache.progress.offsetWidth * elapsedTime / totalTime) : 0;
    if (progressPx === 0) {
        domCache.progressBar.style.transition = 'none';
    }
    domCache.progressBar.style.width = progressPx + 'px'; 
    if (progressPx === 0) {    
        setTimeout(function() {
            domCache.progressBar.style.transition = progressBarTransition;
        }, 10);
    }
    
    if (totalTime <= 0) {
        domCache.progress.style.cursor = 'default';
    }
    else {
        domCache.progress.style.cursor = 'pointer';
    }

    let counterText = beautifySongDuration(elapsedTime) + "&nbsp;/&nbsp;" + beautifySongDuration(totalTime);
    domCache.counter.innerHTML = counterText;
    
    //Set playing track in queue view
    if (lastState) {
        if (lastState.currentSongId !== currentSongId) {
            let tr = document.getElementById('queueTrackId' + lastState.currentSongId);
            if (tr) {
                let durationTd = tr.querySelector('[data-col=Duration]');
                if (durationTd) {
                    durationTd.innerText = getAttDec(tr, 'data-duration');
                }
                let posTd = tr.querySelector('[data-col=Pos]');
                if (posTd) {
                    posTd.classList.remove('mi');
                    posTd.innerText = getAttDec(tr, 'data-songpos');
                }
                tr.classList.remove('font-weight-bold');
            }
        }
    }
    let tr = document.getElementById('queueTrackId' + currentSongId);
    if (tr) {
        let durationTd = tr.querySelector('[data-col=Duration]');
        if (durationTd) {
            durationTd.innerHTML = counterText;
        }
        let posTd = tr.querySelector('[data-col=Pos]');
        if (posTd) {
            if (!posTd.classList.contains('mi')) {
                posTd.classList.add('mi');
                posTd.innerText = 'play_arrow';
            }
        }
        tr.classList.add('font-weight-bold');
    }

    //synced lyrics
    if (showSyncedLyrics === true && settings.colsPlayback.includes('Lyrics')) {
        const sl = document.getElementById('currentLyrics');
        const toHighlight = sl.querySelector('[data-sec="' + elapsedTime + '"]');
        const highlighted = sl.getElementsByClassName('highlight')[0];
        if (highlighted !== toHighlight) {
            if (toHighlight !== null) {
                toHighlight.classList.add('highlight');
                toHighlight.scrollIntoView();
                document.getElementById('currentLyricsLine').innerText = toHighlight.innerText !== null ? toHighlight.innerText : '';
                if (highlighted !== undefined) {
                    highlighted.classList.remove('highlight');
                }
            }
        }
    }
    else {
        document.getElementById('currentLyricsLine').innerText = '';
    }
    
    if (progressTimer) {
        clearTimeout(progressTimer);
    }
    if (playstate === 'play') {
        progressTimer = setTimeout(function() {
            currentSong.elapsedTime ++;
            requestAnimationFrame(function() {
                setCounter(currentSong.currentSongId, currentSong.totalTime, currentSong.elapsedTime);
            });
        }, 1000);
    }
}

function parseState(obj) {
    if (JSON.stringify(obj.result) === JSON.stringify(lastState)) {
        toggleUI();
        return;
    }

    //Set play and queue state
    parseUpdateQueue(obj);
    
    //Set volume
    parseVolume(obj);

    //Set play counters
    setCounter(obj.result.currentSongId, obj.result.totalTime, obj.result.elapsedTime);
    
    //Get current song
    if (!lastState || lastState.currentSongId !== obj.result.currentSongId ||
        lastState.queueVersion !== obj.result.queueVersion)
    {
        sendAPI("MPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }
    //clear playback card if no current song
    if (obj.result.songPos === '-1') {
        document.getElementById('currentTitle').innerText = 'Not playing';
        document.title = 'myMPD';
        document.getElementById('footerTitle').innerText = '';
        document.getElementById('footerTitle').removeAttribute('title');
        document.getElementById('footerTitle').classList.remove('clickable');
        document.getElementById('footerCover').classList.remove('clickable');
        clearCurrentCover();
        if (settings.bgCover === true) {
            clearBackgroundImage();
        }
        let pb = document.getElementById('cardPlaybackTags').getElementsByTagName('p');
        for (let i = 0; i < pb.length; i++) {
            pb[i].innerText = '';
        }
    }
    else {
        let cff = document.getElementById('currentFileformat');
        if (cff) {
            cff.getElementsByTagName('p')[0].innerText = fileformat(obj.result.audioFormat);
        }
    }

    lastState = obj.result;                    
    
    //refresh settings if mpd is not connected or ui is disabled
    //true on startup
    if (settings.mpdConnected === false || uiEnabled === false) {
        getSettings(true);
    }
}

function parseVolume(obj) {
    if (obj.result.volume === -1) {
        document.getElementById('volumePrct').innerText = t('Volumecontrol disabled');
        document.getElementById('volumeControl').classList.add('hide');
    } 
    else {
        document.getElementById('volumeControl').classList.remove('hide');
        document.getElementById('volumePrct').innerText = obj.result.volume + ' %';
        if (obj.result.volume === 0) {
            document.getElementById('volumeMenu').firstChild.innerText = 'volume_off';
        }
        else if (obj.result.volume < 50) {
            document.getElementById('volumeMenu').firstChild.innerText = 'volume_down';
        }
        else {
            document.getElementById('volumeMenu').firstChild.innerText = 'volume_up';
        }
    }
    document.getElementById('volumeBar').value = obj.result.volume;
}

function setBackgroundImage(url) {
    if (url === undefined) {
        clearBackgroundImage();
        return;
    }
    const bgImageUrl = 'url("' + subdir + '/albumart/' + url + '")';
    const old = document.querySelectorAll('.albumartbg');
    if (old[0] && old[0].style.backgroundImage === bgImageUrl) {
        logDebug('Background image already set');
        return;
    }
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
        }
    }
    const div = document.createElement('div');
    div.classList.add('albumartbg');
    div.style.filter = settings.bgCssFilter;
    div.style.backgroundImage = bgImageUrl;
    div.style.opacity = 0;
    domCache.body.insertBefore(div, domCache.body.firstChild);

    const img = new Image();
    img.onload = function() {
        document.querySelector('.albumartbg').style.opacity = 1;
    };
    img.src = subdir + '/albumart/' + url;
}

function clearBackgroundImage() {
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
        }
    }
}

function setCurrentCover(url) {
    _setCurrentCover(url, document.getElementById('currentCover'));
    _setCurrentCover(url, document.getElementById('footerCover'));
}

function _setCurrentCover(url, el) {
    if (url === undefined) {
        clearCurrentCover();
        return;
    }
    let old = el.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '2') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '2';
        }
    }

    let div = document.createElement('div');
    div.classList.add('coverbg', 'carousel');
    div.style.backgroundImage = 'url("' + subdir + '/albumart/' + url + '")';
    div.style.opacity = 0;
    setAttEnc(div, 'data-uri', url);
    el.insertBefore(div, el.firstChild);

    let img = new Image();
    img.onload = function() {
        el.querySelector('.coverbg').style.opacity = 1;
    };
    img.src = subdir + '/albumart/' + url;
}

function clearCurrentCover() {
    _clearCurrentCover(document.getElementById('currentCover'));
    _clearCurrentCover(document.getElementById('footerCover'));
}

function _clearCurrentCover(el) {
    let old = el.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '2') {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = '2';
            old[i].style.opacity = '0';
        }
    }
}

function songChange(obj) {
    let curSong = obj.result.Title + ':' + obj.result.Artist + ':' + obj.result.Album + ':' + obj.result.uri + ':' + obj.result.currentSongId;
    if (lastSong === curSong) {
        return;
    }
    let textNotification = '';
    let pageTitle = '';

    mediaSessionSetMetadata(obj.result.Title, obj.result.Artist, obj.result.Album, obj.result.uri);
    
    setCurrentCover(obj.result.uri);
    if (settings.bgCover === true && settings.featCoverimage === true) {
        setBackgroundImage(obj.result.uri);
    }
    
    document.getElementById('footerArtist').classList.remove('clickable');
    document.getElementById('footerAlbum').classList.remove('clickable');
    document.getElementById('footerCover').classList.remove('clickable');

    if (obj.result.Artist !== undefined && obj.result.Artist.length > 0 && obj.result.Artist !== '-') {
        textNotification += obj.result.Artist;
        pageTitle += obj.result.Artist + ' - ';
        document.getElementById('footerArtist').innerText = obj.result.Artist;
        setAttEnc(document.getElementById('footerArtist'), 'data-name', obj.result.Artist);
        if (settings.featAdvsearch === true) {
            document.getElementById('footerArtist').classList.add('clickable');
        }
    }
    else {
        document.getElementById('footerArtist').innerText = '';
        setAttEnc(document.getElementById('footerArtist'), 'data-name', '');
    }

    if (obj.result.Album !== undefined && obj.result.Album.length > 0 && obj.result.Album !== '-') {
        textNotification += ' - ' + obj.result.Album;
        document.getElementById('footerAlbum').innerText = obj.result.Album;
        setAttEnc(document.getElementById('footerAlbum'), 'data-name', obj.result.Album);
        setAttEnc(document.getElementById('footerAlbum'), 'data-albumartist', obj.result[tagAlbumArtist]);
        if (settings.featAdvsearch === true) {
            document.getElementById('footerAlbum').classList.add('clickable');
        }
    }
    else {
        document.getElementById('footerAlbum').innerText = '';
        setAttEnc(document.getElementById('footerAlbum'), 'data-name', '');
    }

    if (obj.result.Title !== undefined && obj.result.Title.length > 0) {
        pageTitle += obj.result.Title;
        document.getElementById('currentTitle').innerText = obj.result.Title;
        setAttEnc(document.getElementById('currentTitle'), 'data-uri', obj.result.uri);
        document.getElementById('footerTitle').innerText = obj.result.Title;
        document.getElementById('footerTitle').classList.add('clickable');
        document.getElementById('footerCover').classList.add('clickable');
    }
    else {
        document.getElementById('currentTitle').innerText = '';
        setAttEnc(document.getElementById('currentTitle'), 'data-uri', '');
        document.getElementById('footerTitle').innerText = '';
        setAttEnc(document.getElementById('footerTitle'), 'data-name', '');
        document.getElementById('footerTitle').classList.remove('clickable');
        document.getElementById('footerCover').classList.remove('clickable');
    }
    document.title = 'myMPD: ' + pageTitle;
    document.getElementById('footerCover').title = pageTitle;
    
    if (isValidUri(obj.result.uri) === true && isStreamUri(obj.result.uri) === false) {
        document.getElementById('footerTitle').classList.add('clickable');
    }
    else {
        document.getElementById('footerTitle').classList.remove('clickable');
    }

    if (obj.result.uri !== undefined) {
        obj.result['Filetype'] = filetype(obj.result.uri);
        enableEl('addCurrentSongToPlaylist');
    }
    else {
        obj.result['Filetype'] = '';
        disableEl('addCurrentSongToPlaylist');
    }
    
    if (settings.featStickers === true) {
        setVoteSongBtns(obj.result.like, obj.result.uri);
    }
    
    if (lastState) {
        obj.result['Fileformat'] = fileformat(lastState.audioFormat);
    }
    else {
        obj.result['Fileformat'] = '';
    }

    for (let i = 0; i < settings.colsPlayback.length; i++) {
        let c = document.getElementById('current' + settings.colsPlayback[i]);
        if (c && settings.colsPlayback[i] === 'Lyrics') {
            document.getElementById('currentLyricsLine').innerText = '';
            getLyrics(obj.result.uri, c.getElementsByTagName('p')[0]);
        }
        else if (c) {
            let value = obj.result[settings.colsPlayback[i]];
            if (value === undefined) {
                value = '';
            }
            if (settings.colsPlayback[i] === 'Duration') {
                value = beautifySongDuration(value);
            }
            else if (settings.colsPlayback[i] === 'LastModified') {
                value = localeDate(value);
            }
            else if (settings.colsPlayback[i].indexOf('MUSICBRAINZ') === 0) {
                value = getMBtagLink(settings.colsPlayback[i], obj.result[settings.colsPlayback[i]]);
            }
            else {
                value = e(value);
            }
            c.getElementsByTagName('p')[0].innerHTML = value;
            setAttEnc(c, 'data-name', value);
            if (settings.colsPlayback[i] === 'Album' && obj.result[tagAlbumArtist] !== null) {
                setAttEnc(c, 'data-albumartist', obj.result[tagAlbumArtist]);
            }
        }
    }
    
    document.getElementById('currentBooklet').innerHTML = obj.result.bookletPath === '' || obj.result.bookletPath === undefined || settings.featBrowse === false ? '' : 
            '<span class="text-light mi">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>';
    
    //Update title in queue view for http streams
    const playingTr = document.getElementById('queueTrackId' + obj.result.currentSongId);
    if (playingTr) {
        const titleCol = playingTr.querySelector('[data-col=Title');
        if (titleCol) { 
            titleCol.innerText = obj.result.Title;
        }
    }

    if (playstate === 'play') {
        showNotification(obj.result.Title, textNotification, 'player', 'info');
    }
    
    //remember lastSong
    lastSong = curSong;
    lastSongObj = obj.result;
}

//eslint-disable-next-line no-unused-vars
function gotoTagList() {
    appGoto(app.current.app, app.current.tab, app.current.view, '0', undefined, '-', '-', '-', '');
}

//eslint-disable-next-line no-unused-vars
function volumeStep(dir) {
    let inc = dir === 'up' ? settings.volumeStep : 0 - settings.volumeStep;
    chVolume(inc);
}

function chVolume(increment) {
    const volumeBar = document.getElementById('volumeBar');
    let newValue = parseInt(volumeBar.value) + increment;
    if (newValue < settings.volumeMin)  {
        newValue = settings.volumeMin;
    }
    else if (newValue > settings.volumeMax) {
        newValue = settings.volumeMax;
    }
    volumeBar.value = newValue;
    sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": newValue});
}

//eslint-disable-next-line no-unused-vars
function clickTitle() {
    const uri = getAttDec(document.getElementById('currentTitle'), 'data-uri');
    if (isValidUri(uri) === true && isStreamUri(uri) === false) {
        songDetails(uri);
    }
}

function mediaSessionSetPositionState(duration, position) {
    if (settings.mediaSession === true && 'mediaSession' in navigator && navigator.mediaSession.setPositionState) {
        if (position < duration) {
            //streams have position > duration
            navigator.mediaSession.setPositionState({
                duration: duration,
                position: position
            });
        }
    }
}

function mediaSessionSetState() {
    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        if (playstate === 'play') {
            navigator.mediaSession.playbackState = 'playing';
        }
        else {
            navigator.mediaSession.playbackState = 'paused';
        }
    }
}

function mediaSessionSetMetadata(title, artist, album, url) {
    if (settings.mediaSession === true && 'mediaSession' in navigator) {
        let hostname = window.location.hostname;
        let protocol = window.location.protocol;
        let port = window.location.port;
        let artwork = protocol + '//' + hostname + (port !== '' ? ':' + port : '') + subdir + '/albumart/' + url;

        if (settings.coverimage === true) {
            //eslint-disable-next-line no-undef
            navigator.mediaSession.metadata = new MediaMetadata({
                title: title,
                artist: artist,
                album: album,
                artwork: [{src: artwork}]
            });
        }
        else {
            //eslint-disable-next-line no-undef
            navigator.mediaSession.metadata = new MediaMetadata({
                title: title,
                artist: artist,
                album: album
            });
        }
    }
}
