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
        showNotification(tn('Added stream %{streamUri} to queue', {"streamUri": streamUriEl.value}), '', 'queue', 'info');
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
    else {
        //fallback if playstate is unknown
        sendAPI("MYMPD_API_PLAYER_PLAY", {});
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
    const msg = tn('Database update failed') + ': ' + tn(message);
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        elShowId('updateDBfooter');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '0';
        updateDBprogress.style.marginLeft = '0px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        elShow(errorUpdateDB);
        errorUpdateDB.textContent = msg;
        uiElements.modalUpdateDB.show();
    }
    showNotification(msg, '', 'database', 'error');
}

function updateDBstarted(showModal) {
    if (showModal === true) {
        document.getElementById('updateDBfinished').textContent = '';
        elHideId('updateDBfooter');
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        const errorUpdateDB = document.getElementById('errorUpdateDB');
        elHide(errorUpdateDB);
        errorUpdateDB.textContent = '';
        uiElements.modalUpdateDB.show();
        updateDBprogress.classList.add('updateDBprogressAnimate');
    }
    showNotification(tn('Database update started'), '', 'database', 'info');
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
            elShow(parent.children[i]);
        }
    }

    //update database modal
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent === 'update_database') {
            document.getElementById('updateDBfinished').textContent = tn('Database successfully updated');
        }
        else if (idleEvent === 'update_finished') {
            document.getElementById('updateDBfinished').textContent = tn('Database update finished');
        }
        const updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        elShowId('updateDBfooter');
    }

    //general notification
    if (idleEvent === 'update_database') {
        showNotification(tn('Database successfully updated'), '', 'database', 'info');
    }
    else if (idleEvent === 'update_finished') {
        showNotification(tn('Database update finished'), '', 'database', 'info');
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
        const uri = getCustomDomProperty(el, 'data-uri');
        if (uri) {
            aImages = [ subdir + '/albumart/' + myEncodeURI(uri) ];
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
        elHideId('modalPictureZoom');
        uiElements.modalPicture.show();
        return;
    }
    
    if (el.style.backgroundImage !== '') {
        const imgEl = document.getElementById('modalPictureImg');
        elClear(imgEl);
        imgEl.style.paddingTop = '100%';
        imgEl.style.backgroundImage = el.style.backgroundImage;
        elShowId('modalPictureZoom');
        uiElements.modalPicture.show();
    }
}

//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}

function createImgCarousel(imgEl, name, images) {
    const nrImages = images.length;
    const carousel = elCreateEmpty('div', {"id": name, "class": ["carousel", "slide"], "data-bs-ride": "carousel"});
    if (nrImages > 0) {
        const carouselIndicators = elCreateEmpty('div', {"class": ["carousel-indicators"]});
        for (let i = 0; i < nrImages; i++) {
            carouselIndicators.appendChild(elCreateEmpty('button', {"type": "button", "data-bs-target": "#" + name, "data-bs-slide-to": i}));
            if (i === 0) {
                carouselIndicators.lastChild.classList.add('active');
            }
        }
        carousel.appendChild(carouselIndicators);
    }
    const carouselInner = elCreateEmpty('div', {"class": ["carousel-inner"]});
    for (let i = 0; i < nrImages; i++) {
        carouselInner.appendChild(elCreateEmpty('div', {"class": ["carousel-item"]}));
        carouselInner.lastChild.appendChild(elCreateEmpty('div', {}));
        carouselInner.lastChild.style.backgroundImage = 'url("' + myEncodeURI(images[i]) + '")';
        if (i === 0) {
            carouselInner.lastChild.classList.add('active');
        }
    }
    carousel.appendChild(carouselInner);
    if (nrImages > 0) {
        const prev = elCreateEmpty('a', {"href": "#" + name, "data-bs-slide": "prev", "class": ["carousel-control-prev"]});
        prev.appendChild(elCreateEmpty('span', {"class": ["carousel-control-prev-icon"]}));
        carousel.appendChild(prev);
        const next = elCreateEmpty('a', {"href": "#" + name, "data-bs-slide": "next", "class": ["carousel-control-next"]});
        next.appendChild(elCreateEmpty('span', {"class": ["carousel-control-next-icon"]}));
        carousel.appendChild(next);
    }

    elClear(imgEl);
    imgEl.appendChild(carousel);  
    uiElements.albumartCarousel = new BSN.Carousel(carousel, {
        interval: false,
        pause: false
    });
}

function ucFirst(string) {
    return string[0].toUpperCase() + string.slice(1);
}
