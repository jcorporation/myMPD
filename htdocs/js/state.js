"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseStats(obj) {
    document.getElementById('mpdstats_artists').innerText =  obj.data.artists;
    document.getElementById('mpdstats_albums').innerText = obj.data.albums;
    document.getElementById('mpdstats_songs').innerText = obj.data.songs;
    document.getElementById('mpdstats_dbPlaytime').innerText = beautifyDuration(obj.data.dbPlaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.data.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.data.uptime);
    document.getElementById('mpdstats_dbUpdated').innerText = localeDate(obj.data.dbUpdated);
    document.getElementById('mympdVersion').innerText = obj.data.mympdVersion;
    document.getElementById('mpdInfo_version').innerText = obj.data.mpdVersion;
    document.getElementById('mpdInfo_libmpdclientVersion').innerText = obj.data.libmpdclientVersion;
}

function parseOutputs(obj) {
    let btns = '';
    let outputsLen = obj.data.length;
    for (let i = 0; i < outputsLen; i++) {
        btns += '<button id="btnOutput' + obj.data[i].id +'" data-output-id="' + obj.data[i].id + '" class="btn btn-secondary btn-block';
        if (obj.data[i].state == 1) {
            btns += ' active';
        }
        btns += '"><span class="material-icons float-left">volume_up</span> ' + e(obj.data[i].name) + '</button>';
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
        if (lastState.data.currentSongId != currentSongId) {
            let tr = document.getElementById('queueTrackId' + lastState.data.currentSongId);
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
    if (playstate == 'play') {
        progressTimer = setTimeout(function() {
            currentSong.elapsedTime ++;
            requestAnimationFrame(function() {
                setCounter(currentSong.currentSongId, currentSong.totalTime, currentSong.elapsedTime);
            });
        }, 1000);
    }
}

function parseState(obj) {
    if (JSON.stringify(obj) === JSON.stringify(lastState)) {
        toggleUI();
        return;
    }

    //Set play and queue state
    parseUpdateQueue(obj);
    
    //Set volume
    parseVolume(obj);

    //Set play counters
    setCounter(obj.data.currentSongId, obj.data.totalTime, obj.data.elapsedTime);
    
    //Get current song
    if (!lastState || lastState.data.currentSongId != obj.data.currentSongId ||
        lastState.data.queueVersion != obj.data.queueVersion)
    {
        sendAPI("MPD_API_PLAYER_CURRENT_SONG", {}, songChange);
    }
    //clear playback card if not playing
    if (obj.data.songPos == '-1') {
        domCache.currentTitle.innerText = 'Not playing';
        document.title = 'myMPD';
        document.getElementById('headerTitle').innerText = '';
        document.getElementById('headerTitle').removeAttribute('title');
        clearCurrentCover();
        if (settings.bgCover == true) {
            clearBackgroundImage();
        }
        let pb = document.getElementById('cardPlaybackTags').getElementsByTagName('h4');
        for (let i = 0; i < pb.length; i++) {
            pb[i].innerText = '';
        }
    }


    lastState = obj;                    
    
    if (settings.mpdConnected == false || uiEnabled == false) {
        getSettings(true);
    }
}

function parseVolume(obj) {
    if (obj.data.volume == -1) {
        domCache.volumePrct.innerText = t('Volumecontrol disabled');
        domCache.volumeControl.classList.add('hide');
    } 
    else {
        domCache.volumeControl.classList.remove('hide');
        domCache.volumePrct.innerText = obj.data.volume + ' %';
        if (obj.data.volume == 0) {
            domCache.volumeMenu.innerText = 'volume_off';
        }
        else if (obj.data.volume < 50) {
            domCache.volumeMenu.innerText = 'volume_down';
        }
        else {
            domCache.volumeMenu.innerText = 'volume_up';
        }
    }
    domCache.volumeBar.value = obj.data.volume;
}

function setBackgroundImage(imageUrl) {
    let old = document.querySelectorAll('.albumartbg');
    for (let i = 0; i < old.length; i++) {
        if (old[i].style.zIndex == -10) {
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
        if (old[i].style.zIndex == -10) {
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
        if (old[i].style.zIndex == 2) {
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
        if (old[i].style.zIndex == 2) {
            old[i].remove();        
        }
        else {
            old[i].style.zIndex = 2;
            old[i].style.opacity = 0;
        }
    }
}

function songChange(obj) {
    if (obj.method != 'MPD_API_PLAYER_CURRENT_SONG' && obj.method != 'song_change') {
        return;
    }
    let curSong = obj.data.Title + obj.data.Artist + obj.data.Album + obj.data.uri + obj.data.currentSongId;
    if (lastSong == curSong) 
        return;
    let textNotification = '';
    let htmlNotification = '';
    let pageTitle = '';

    setCurrentCover(obj.data.cover);
    if (settings.bgCover == true && settings.featCoverimage == true) {
        if (obj.data.cover.indexOf('coverimage-') > -1 ) {
            clearBackgroundImage();
        }
        else {
            setBackgroundImage(obj.data.cover);
        }
    }

    if (typeof obj.data.Artist != 'undefined' && obj.data.Artist.length > 0 && obj.data.Artist != '-') {
        textNotification += obj.data.Artist;
        htmlNotification += obj.data.Artist;
        pageTitle += obj.data.Artist + ' - ';
    } 

    if (typeof obj.data.Album != 'undefined' && obj.data.Album.length > 0 && obj.data.Album != '-') {
        textNotification += ' - ' + obj.data.Album;
        htmlNotification += '<br/>' + obj.data.Album;
    }

    if (typeof obj.data.Title != 'undefined' && obj.data.Title.length > 0) {
        pageTitle += obj.data.Title;
        domCache.currentTitle.innerText = obj.data.Title;
        domCache.currentTitle.setAttribute('data-uri', encodeURI(obj.data.uri));
    }
    else {
        domCache.currentTitle.innerText = '';
        domCache.currentTitle.setAttribute('data-uri', '');
    }
    document.title = 'myMPD: ' + pageTitle;
    document.getElementById('headerTitle').innerText = pageTitle;
    document.getElementById('headerTitle').title = pageTitle;

    if (settings.featStickers == true) {
        setVoteSongBtns(obj.data.like, obj.data.uri);
    }

    for (let i = 0; i < settings.colsPlayback.length; i++) {
        let c = document.getElementById('current' + settings.colsPlayback[i]);
        if (c) {
            c.getElementsByTagName('h4')[0].innerText = obj.data[settings.colsPlayback[i]];
            c.setAttribute('data-name', encodeURI(obj.data[settings.colsPlayback[i]]));
        }
    }
    
    //Update Artist in queue view for http streams
    let playingTr = document.getElementById('queueTrackId' + obj.data.currentSongId);
    if (playingTr) {
        playingTr.getElementsByTagName('td')[1].innerText = obj.data.Title;
    }

    if (playstate == 'play') {
        showNotification(obj.data.Title, textNotification, htmlNotification, 'success');
    }
    lastSong = curSong;
    lastSongObj = obj;
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
    if (uri != '') {
        songDetails(uri);
    }
}
