"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//eslint-disable-next-line no-unused-vars
function openFullscreen() {
    const elem = document.documentElement;
    if (elem.requestFullscreen) {
        elem.requestFullscreen();
    }
    else if (elem.mozRequestFullScreen) {
        //Firefox
        elem.mozRequestFullScreen();
    }
    else if (elem.webkitRequestFullscreen) {
        //Chrome, Safari and Opera
        elem.webkitRequestFullscreen();
    }
    else if (elem.msRequestFullscreen) {
        //IE and Edge
        elem.msRequestFullscreen();
    }
}

function setViewport(store) {
    document.querySelector("meta[name=viewport]").setAttribute('content', 'width=device-width, initial-scale=' + scale + ', maximum-scale=' + scale);
    if (store === true) {
        try {
            localStorage.setItem('scale-ratio', scale);
        }
        catch(err) {
            logError('Can not save scale-ratio in localStorage: ' + err.message);
        }
    }
}

//eslint-disable-next-line no-unused-vars
function addStream() {
    const streamUriEl = document.getElementById('streamUrl');
    if (validateStream(streamUriEl) === true) {
        sendAPI("MYMPD_API_QUEUE_ADD_URI", {"uri": streamUriEl.value});
        uiElements.modalAddToPlaylist.hide();
        showNotification(t('Added stream %{streamUri} to queue', {"streamUri": streamUriEl.value}), '', 'queue', 'info');
    }
}

function seekRelativeForward() {
    seekRelative(5);
}

function seekRelativeBackward() {
    seekRelative(-5);
}

function seekRelative(offset) {
    sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {"seek": offset, "relative": true});
}

//eslint-disable-next-line no-unused-vars
function clickPlay() {
    if (playstate === 'stop') {
        sendAPI("MYMPD_API_PLAYER_PLAY", {});
    }
    else if (playstate === 'play') {
        if (settings.webuiSettings.uiFooterPlaybackControls === 'stop') {
            sendAPI("MYMPD_API_PLAYER_STOP", {});
        }
        else {
            sendAPI("MYMPD_API_PLAYER_PAUSE", {});
        }
    }
    else if (playstate === 'pause') {
        sendAPI("MYMPD_API_PLAYER_RESUME", {});
    }
}

//eslint-disable-next-line no-unused-vars
function clickStop() {
    sendAPI("MYMPD_API_PLAYER_STOP", {});
}

//eslint-disable-next-line no-unused-vars
function clickPrev() {
    sendAPI("MYMPD_API_PLAYER_PREV", {});
}

//eslint-disable-next-line no-unused-vars
function clickNext() {
    sendAPI("MYMPD_API_PLAYER_NEXT", {});
}

//eslint-disable-next-line no-unused-vars
function clearCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CLEAR", {});
}

//eslint-disable-next-line no-unused-vars
function cropCovercache() {
    sendAPI("MYMPD_API_COVERCACHE_CROP", {});
}

//eslint-disable-next-line no-unused-vars
function updateDB(uri, showModal, rescan) {
    const method = rescan === true ? "MYMPD_API_DATABASE_RESCAN" : "MYMPD_API_DATABASE_UPDATE";
    sendAPI(method, {"uri": uri}, function(obj) {
        if (obj.error !== undefined) {
            updateDBerror(true, obj.error.message);
        }
        else {
            updateDBstarted(showModal);
        }
    }, true);
}

function updateDBerror(showModal, message) {
    const msg = t('Database update failed') + ': ' + t(message);
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        document.getElementById('updateDBfooter').classList.remove('hide');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '0';
        updateDBprogress.style.marginLeft = '0px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        errorUpdateDB.classList.remove('hide');
        errorUpdateDB.innerHTML = msg;
        uiElements.modalUpdateDB.show();
    }
    showNotification(msg, '', 'database', 'error');
}

function updateDBstarted(showModal) {
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        document.getElementById('updateDBfooter').classList.add('hide');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        errorUpdateDB.classList.add('hide');
        errorUpdateDB.textContent = '';
        uiElements.modalUpdateDB.show();
        updateDBprogress.classList.add('updateDBprogressAnimate');
    }
    showNotification(t('Database update started'), '', 'database', 'info');
}

function updateDBfinished(idleEvent) {
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        _updateDBfinished(idleEvent);
    }
    else {
        //on small databases the modal opens after the finish event
        setTimeout(function() {
            _updateDBfinished(idleEvent);
        }, 100);
    }
}

function _updateDBfinished(idleEvent) {
    //spinner in mounts modal
    const el = document.getElementById('spinnerUpdateProgress');
    if (el) {
        const parent = el.parentNode;
        el.remove();
        for (let i = 0, j = parent.children.length; i < j; i++) {
            parent.children[i].classList.remove('hide');
        }
    }

    //update database modal
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent === 'update_database') {
            document.getElementById('updateDBfinished').textContent = t('Database successfully updated');
        }
        else if (idleEvent === 'update_finished') {
            document.getElementById('updateDBfinished').textContent = t('Database update finished');
        }
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        document.getElementById('updateDBfooter').classList.remove('hide');
    }

    //general notification
    if (idleEvent === 'update_database') {
        showNotification(t('Database successfully updated'), '', 'database', 'info');
    }
    else if (idleEvent === 'update_finished') {
        showNotification(t('Database update finished'), '', 'database', 'info');
    }
}

//eslint-disable-next-line no-unused-vars
function zoomPicture(el) {
    if (el.classList.contains('booklet')) {
        window.open(getCustomDomProperty(el, 'data-href'));
        return;
    }
    
    if (el.classList.contains('carousel')) {
        let images;
        const dataImages = getCustomDomProperty(el, 'data-images');
        if (dataImages !== undefined && dataImages !== null) {
            images = dataImages.slice();
        }
        else if (lastSongObj.images) {
            images = lastSongObj.images.slice();
        }
        else {
            return;
        }
        
        //add uri to image list to get embedded albumart
        let aImages = [];
        //use uri encoded attribute
        const uri = getCustomDomProperty(el, 'data-uri');
        if (uri) {
            aImages = [ subdir + '/albumart/' + uri ];
        }
        //add all but coverfiles to image list
        for (let i = 0, j = images.length; i < j; i++) {
            if (isCoverfile(images[i]) === false) {
                aImages.push(subdir + '/browse/music/' + images[i]);
            }
        }
        const imgEl = document.getElementById('modalPictureImg');
        imgEl.style.paddingTop = 0;
        createImgCarousel(imgEl, 'picsCarousel', aImages);
        document.getElementById('modalPictureZoom').classList.add('hide');
        uiElements.modalPicture.show();
        return;
    }
    
    if (el.style.backgroundImage !== '') {
        const imgEl = document.getElementById('modalPictureImg');
        elClear(imgEl);
        imgEl.style.paddingTop = '100%';
        imgEl.style.backgroundImage = el.style.backgroundImage;
        document.getElementById('modalPictureZoom').classList.remove('hide');
        uiElements.modalPicture.show();
    }
}

//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}

function createImgCarousel(imgEl, name, images) {
    const nrImages = images.length;
    let carousel = '<div id="' + name + '" class="carousel slide" data-ride="carousel">';
    if (nrImages > 1) {
        carousel += '<ol class="carousel-indicators">';
        for (let i = 0; i < nrImages; i++) {
            carousel += '<li data-target="#' + name + '" data-slide-to="' + i + '"' +
                (i === 0 ? ' class="active"' : '') + '></li>';
        }
        carousel += '</ol>';
    }
    carousel += '<div class="carousel-inner">';
    for (let i = 0; i < nrImages; i++) {
        carousel += '<div class="carousel-item' + (i === 0 ? ' active' : '') + '"><div></div></div>';
    }
    carousel += '</div>';
    if (nrImages > 1) {
        carousel += '<a class="carousel-control-prev" href="#' + name + '" data-slide="prev">' +
                '<span class="carousel-control-prev-icon"></span>' +
            '</a>' +
            '<a class="carousel-control-next" href="#' + name + '" data-slide="next">' +
                '<span class="carousel-control-next-icon"></span>' +
            '</a>';
    }
    carousel += '</div>';
    imgEl.innerHTML = carousel;
    const carouselItems = imgEl.getElementsByClassName('carousel-item');
    for (let i = 0, j = carouselItems.length; i < j; i++) {
        carouselItems[i].children[0].style.backgroundImage = 'url("' + myEncodeURI(images[i]) + '")';
    }
    
    uiElements.albumartCarousel = new BSN.Carousel(document.getElementById(name), {
        interval: false,
        pause: false
    });
}

function ucFirst(string) {
    return string[0].toUpperCase() + string.slice(1);
}
