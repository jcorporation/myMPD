"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
            btns += '<button id="btnOutput' + obj.result.data[i].id +'" data-output-name="' + encodeURI(obj.result.data[i].name) + '" data-output-id="' + obj.result.data[i].id + '" class="btn btn-secondary btn-block';
            if (obj.result.data[i].state === 1) {
                btns += ' active';
            }
            btns += '"><span class="material-icons float-left">volume_up</span> ' + e(obj.result.data[i].name);
            if (Object.keys(obj.result.data[i].attributes).length > 0) {
                btns += '<a class="material-icons float-right text-white" title="' + t('Edit attributes') + '">settings</a>';
            }
            else {
                btns += '<a class="material-icons float-right text-white" title="' + t('Show attributes') + '">settings</a>';
            }
            btns += '</button>';
        }
    }
    if (nr === 0) {
        btns = '<span class="material-icons">error_outline</span> ' + t('No outputs');
    }
    domCache.outputs.innerHTML = btns;
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
            document.getElementById('btnOutputAttributesSave').removeAttribute('disabled');
        }
        else {
            document.getElementById('btnOutputAttributesSave').setAttribute('disabled', 'disabled');
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
                    durationTd.innerText = tr.getAttribute('data-duration');
                }
                let posTd = tr.querySelector('[data-col=Pos]');
                if (posTd) {
                    posTd.classList.remove('material-icons');
                    posTd.innerText = tr.getAttribute('data-songpos');
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
            if (!posTd.classList.contains('material-icons')) {
                posTd.classList.add('material-icons');
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
                if (highlighted !== undefined) {
                    highlighted.classList.remove('highlight');
                }
            }
        }
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
        domCache.currentTitle.innerText = 'Not playing';
        document.title = 'myMPD';
        domCache.footerTitle.innerText = '';
        domCache.footerTitle.removeAttribute('title');
        domCache.footerTitle.classList.remove('clickable');
        domCache.footerCover.classList.remove('clickable');
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
    
    if (settings.mpdConnected === false || uiEnabled === false) {
        getSettings(true);
    }
}

function parseVolume(obj) {
    if (obj.result.volume === -1) {
        domCache.volumePrct.innerText = t('Volumecontrol disabled');
        domCache.volumeControl.classList.add('hide');
    } 
    else {
        domCache.volumeControl.classList.remove('hide');
        domCache.volumePrct.innerText = obj.result.volume + ' %';
        if (obj.result.volume === 0) {
            domCache.volumeMenu.firstChild.innerText = 'volume_off';
        }
        else if (obj.result.volume < 50) {
            domCache.volumeMenu.firstChild.innerText = 'volume_down';
        }
        else {
            domCache.volumeMenu.firstChild.innerText = 'volume_up';
        }
    }
    domCache.volumeBar.value = obj.result.volume;
}

function setBackgroundImage(url) {
    if (url === undefined) {
        clearBackgroundImage();
        return;
    }
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === '-10') {
            old[i].remove();
        }
        else {
            old[i].style.zIndex = '-10';
            old[i].style.opacity = '0';
            //old[i].style.filter = '';
        }
    }
    let div = document.createElement('div');
    div.classList.add('albumartbg');
    div.style.filter = settings.bgCssFilter;
    div.style.backgroundImage = 'url("' + subdir + '/albumart/' + url + '")';
    div.style.opacity = 0;
    let body = document.getElementsByTagName('body')[0];
    body.insertBefore(div, body.firstChild);

    let img = new Image();
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
            //old[i].style.filter = '';
        }
    }
}

function setCurrentCover(url) {
    _setCurrentCover(url, domCache.currentCover);
    _setCurrentCover(url, domCache.footerCover);
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
    div.setAttribute('data-uri', url);
    el.insertBefore(div, el.firstChild);

    let img = new Image();
    img.onload = function() {
        el.querySelector('.coverbg').style.opacity = 1;
    };
    img.src = subdir + '/albumart/' + url;
}

function clearCurrentCover() {
    _clearCurrentCover(domCache.currentCover);
    _clearCurrentCover(domCache.footerCover);
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
    let htmlNotification = '';
    let pageTitle = '';

    mediaSessionSetMetadata(obj.result.Title, obj.result.Artist, obj.result.Album, obj.result.uri);
    
    setCurrentCover(obj.result.uri);
    if (settings.bgCover === true && settings.featCoverimage === true) {
        setBackgroundImage(obj.result.uri);
    }
    
    domCache.footerArtist.classList.remove('clickable');
    domCache.footerAlbum.classList.remove('clickable');
    domCache.footerCover.classList.remove('clickable');

    if (obj.result.Artist !== undefined && obj.result.Artist.length > 0 && obj.result.Artist !== '-') {
        textNotification += obj.result.Artist;
        htmlNotification += obj.result.Artist;
        pageTitle += obj.result.Artist + ' - ';
        domCache.footerArtist.innerText = obj.result.Artist;
        domCache.footerArtist.setAttribute('data-name', encodeURI(obj.result.Artist));
        if (settings.featAdvsearch === true) {
            domCache.footerArtist.classList.add('clickable');
        }
    }
    else {
        domCache.footerArtist.innerText = '';
        domCache.footerArtist.setAttribute('data-name', '');
    }

    if (obj.result.Album !== undefined && obj.result.Album.length > 0 && obj.result.Album !== '-') {
        textNotification += ' - ' + obj.result.Album;
        htmlNotification += '<br/>' + obj.result.Album;
        domCache.footerAlbum.innerText = obj.result.Album;
        domCache.footerAlbum.setAttribute('data-name', encodeURI(obj.result.Album));
        domCache.footerAlbum.setAttribute('data-albumartist', encodeURI(obj.result[tagAlbumArtist]));
        if (settings.featAdvsearch === true) {
            domCache.footerAlbum.classList.add('clickable');
        }
    }
    else {
        domCache.footerAlbum.innerText = '';
        domCache.footerAlbum.setAttribute('data-name', '');
    }

    if (obj.result.Title !== undefined && obj.result.Title.length > 0) {
        pageTitle += obj.result.Title;
        domCache.currentTitle.innerText = obj.result.Title;
        domCache.currentTitle.setAttribute('data-uri', encodeURI(obj.result.uri));
        domCache.footerTitle.innerText = obj.result.Title;
        domCache.footerTitle.classList.add('clickable');
        domCache.footerCover.classList.add('clickable');
    }
    else {
        domCache.currentTitle.innerText = '';
        domCache.currentTitle.setAttribute('data-uri', '');
        domCache.footerTitle.innerText = '';
        domCache.footerTitle.setAttribute('data-name', '');
        domCache.footerTitle.classList.remove('clickable');
        domCache.footerCover.classList.remove('clickable');
    }
    document.title = 'myMPD: ' + pageTitle;
    domCache.footerCover.title = pageTitle;
    
    if (isValidUri(obj.result.uri) === true && isStreamUri(obj.result.uri) === false) {
        domCache.footerTitle.classList.add('clickable');
    }
    else {
        domCache.footerTitle.classList.remove('clickable');
    }

    if (obj.result.uri !== undefined) {
        obj.result['Filetype'] = filetype(obj.result.uri);
        document.getElementById('addCurrentSongToPlaylist').removeAttribute('disabled');
    }
    else {
        obj.result['Filetype'] = '';
        document.getElementById('addCurrentSongToPlaylist').setAttribute('disabled', 'disabled');
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
            c.setAttribute('data-name', encodeURI(value));
            if (settings.colsPlayback[i] === 'Album' && obj.result[tagAlbumArtist] !== null) {
                c.setAttribute('data-albumartist', encodeURI(obj.result[tagAlbumArtist]));
            }
        }
    }
    
    document.getElementById('currentBooklet').innerHTML = obj.result.bookletPath === '' || obj.result.bookletPath === undefined|| settings.featBrowse === false ? '' : 
            '<span class="text-light material-icons">description</span>&nbsp;<a class="text-light" target="_blank" href="' + subdir + '/browse/music/' + 
            e(obj.result.bookletPath) + '">' + t('Download booklet') + '</a>';
    
    //Update Artist in queue view for http streams
    let playingTr = document.getElementById('queueTrackId' + obj.result.currentSongId);
    if (playingTr) {
        playingTr.getElementsByTagName('td')[1].innerText = obj.result.Title;
    }

    if (playstate === 'play') {
        showNotification(obj.result.Title, textNotification, htmlNotification, 'success');
    }
    
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
    let newValue = parseInt(domCache.volumeBar.value) + increment;
    if (newValue < settings.volumeMin)  {
        newValue = settings.volumeMin;
    }
    else if (newValue > settings.volumeMax) {
        newValue = settings.volumeMax;
    }
    domCache.volumeBar.value = newValue;
    sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": newValue});
}

//eslint-disable-next-line no-unused-vars
function clickTitle() {
    const uri = decodeURI(domCache.currentTitle.getAttribute('data-uri'));
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
