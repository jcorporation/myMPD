"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseStats(obj) {
    document.getElementById('mpdstats_artists').innerText =  obj.result.artists;
    document.getElementById('mpdstats_albums').innerText = obj.result.albums;
    document.getElementById('mpdstats_songs').innerText = obj.result.songs;
    document.getElementById('mpdstats_dbPlaytime').innerText = beautifyDuration(obj.result.dbPlaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.result.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.result.uptime);
    document.getElementById('mpdstats_dbUpdated').innerText = localeDate(obj.result.dbUpdated);
    document.getElementById('mympdVersion').innerText = obj.result.mympdVersion;
    document.getElementById('mpdInfo_version').innerText = obj.result.mpdVersion;
    document.getElementById('mpdInfo_libmpdclientVersion').innerText = obj.result.libmpdclientVersion;
}

function parseOutputs(obj) {
    let btns = '';
    for (let i = 0; i < obj.result.numOutputs; i++) {
        btns += '<button id="btnOutput' + obj.result.data[i].id +'" data-output-id="' + obj.result.data[i].id + '" class="btn btn-secondary btn-block';
        if (obj.result.data[i].state === 1) {
            btns += ' active';
        }
        btns += '"><span class="material-icons float-left">volume_up</span> ' + e(obj.result.data[i].name) + '</button>';
    }
    domCache.outputs.innerHTML = btns;
}

function setCounter(currentSongId, totalTime, elapsedTime) {
    currentSong.totalTime = totalTime;
    currentSong.elapsedTime = elapsedTime;
    currentSong.currentSongId = currentSongId;

    domCache.progressBar.value = Math.floor(1000 * elapsedTime / totalTime);

    let counterText = beautifySongDuration(elapsedTime) + "&nbsp;/&nbsp;" + beautifySongDuration(totalTime);
    domCache.counter.innerHTML = counterText;
    
    //Set playing track in queue view
    if (lastState) {
        if (lastState.currentSongId !== currentSongId) {
            let tr = document.getElementById('queueTrackId' + lastState.currentSongId);
            if (tr) {
                let durationTd = tr.querySelector('[data-col=Duration]');
                if (durationTd)
                    durationTd.innerText = tr.getAttribute('data-duration');
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
    //clear playback card if not playing
    if (obj.result.songPos === '-1') {
        domCache.currentTitle.innerText = 'Not playing';
        document.title = 'myMPD';
        document.getElementById('headerTitle').innerText = '';
        document.getElementById('headerTitle').removeAttribute('title');
        clearCurrentCover();
        if (settings.bgCover === true) {
            clearBackgroundImage();
        }
        let pb = document.getElementById('cardPlaybackTags').getElementsByTagName('h4');
        for (let i = 0; i < pb.length; i++) {
            pb[i].innerText = '';
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
            domCache.volumeMenu.innerText = 'volume_off';
        }
        else if (obj.result.volume < 50) {
            domCache.volumeMenu.innerText = 'volume_down';
        }
        else {
            domCache.volumeMenu.innerText = 'volume_up';
        }
    }
    domCache.volumeBar.value = obj.result.volume;
}

function setBackgroundImage(imageUrl) {
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === -10) {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = -10;
        }
    }
    let div = document.createElement('div');
    div.classList.add('albumartbg');
    div.style.filter = settings.bgCssFilter;
    div.style.backgroundImage = 'url("' + subdir + imageUrl + '")';
    div.style.opacity = 0;
    let body = document.getElementsByTagName('body')[0];
    body.insertBefore(div, body.firstChild);

    let img = new Image();
    img.onload = function() {
        document.querySelector('.albumartbg').style.opacity = 1;
    };
    img.src = imageUrl;
}

function clearBackgroundImage() {
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === -10) {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = -10;
            old[i].style.opacity = 0;
        }
    }
}

function setCurrentCover(imageUrl) {
    let old = domCache.currentCover.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === 2) {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = 2;
        }
    }

    let div = document.createElement('div');
    div.classList.add('coverbg');
    div.style.backgroundImage = 'url("' + subdir + imageUrl + '")';
    div.style.opacity = 0;
    domCache.currentCover.insertBefore(div, domCache.currentCover.firstChild);

    let img = new Image();
    img.onload = function() {
        domCache.currentCover.querySelector('.coverbg').style.opacity = 1;
    };
    img.src = imageUrl;
}

function clearCurrentCover() {
    let old = domCache.currentCover.querySelectorAll('.coverbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex === 2) {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = 2;
            old[i].style.opacity = 0;
        }
    }
}

function songChange(obj) {
    let curSong = obj.result.uri + ':' + obj.result.currentSongId;
    if (lastSong === curSong) {
        return;
    }
    let textNotification = '';
    let htmlNotification = '';
    let pageTitle = '';

    setCurrentCover(obj.result.cover);
    if (settings.bgCover === true && settings.featCoverimage === true) {
        if (obj.result.cover.indexOf('coverimage-') > -1 ) {
            clearBackgroundImage();
        }
        else {
            setBackgroundImage(obj.result.cover);
        }
    }

    if (obj.result.Artist !== undefined && obj.result.Artist.length > 0 && obj.result.Artist !== '-') {
        textNotification += obj.result.Artist;
        htmlNotification += obj.result.Artist;
        pageTitle += obj.result.Artist + ' - ';
    } 

    if (obj.result.Album !== undefined && obj.result.Album.length > 0 && obj.result.Album !== '-') {
        textNotification += ' - ' + obj.result.Album;
        htmlNotification += '<br/>' + obj.result.Album;
    }

    if (obj.result.Title !== undefined && obj.result.Title.length > 0) {
        pageTitle += obj.result.Title;
        domCache.currentTitle.innerText = obj.result.Title;
        domCache.currentTitle.setAttribute('data-uri', encodeURI(obj.result.uri));
    }
    else {
        domCache.currentTitle.innerText = '';
        domCache.currentTitle.setAttribute('data-uri', '');
    }
    document.title = 'myMPD: ' + pageTitle;
    document.getElementById('headerTitle').innerText = pageTitle;
    document.getElementById('headerTitle').title = pageTitle;

    if (settings.featStickers === true) {
        setVoteSongBtns(obj.result.like, obj.result.uri);
    }

    logDebug('colsPlayback: ' + JSON.stringify(settings.colsPlayback));
    for (let i = 0; i < settings.colsPlayback.length; i++) {
        let c = document.getElementById('current' + settings.colsPlayback[i]);
        if (c) {
            c.getElementsByTagName('h4')[0].innerText = obj.result[settings.colsPlayback[i]];
            c.setAttribute('data-name', encodeURI(obj.result[settings.colsPlayback[i]]));
        }
    }
    
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
    appGoto(app.current.app, app.current.tab, app.current.view, '0/-/-/');
}

//eslint-disable-next-line no-unused-vars
function chVolume(increment) {
    let newValue = parseInt(domCache.volumeBar.value) + increment;
    if (newValue < 0)  {
        newValue = 0;
    }
    else if (newValue > 100) {
        newValue = 100;
    }
    domCache.volumeBar.value = newValue;
    sendAPI("MPD_API_PLAYER_VOLUME_SET", {"volume": newValue});
}

//eslint-disable-next-line no-unused-vars
function clickTitle() {
    let uri = decodeURI(domCache.currentTitle.getAttribute('data-uri'));
    if (uri !== '') {
        songDetails(uri);
    }
}
