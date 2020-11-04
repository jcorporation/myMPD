"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function parseHome(obj) {
    let nrItems = obj.result.returnedEntities;
    let cardContainer = document.getElementById('HomeCards');
    let cols = cardContainer.getElementsByClassName('col');
    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        let col = document.createElement('div');
        col.classList.add('col', 'px-0', 'flex-grow-0');
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = t('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = t('Unknown album');
        }
        let href=JSON.stringify({"cmd": obj.result.data[i].cmd, "options": obj.result.data[i].options});
        let html = '<div class="card home-icons clickable" draggable="true" tabindex="0" data-href=\'' + href + '\'>' +
                   (obj.result.data[i].ligature !== '' ? 
                       '<div class="card-body material-icons home-icons-ligature">' + e(obj.result.data[i].ligature) + '</div>' :
                       '<div class="card-body home-icons-image"></div>')+
                   '<div class="card-footer card-footer-grid p-2">' +
                   e(obj.result.data[i].name) + 
                   '</div></div>';
        col.innerHTML = html;
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
        const hii = col.getElementsByClassName('home-icons-image')[0];
        if (hii) {
            hii.style.backgroundImage = 'url("' + subdir + '/browse/pics/' + obj.result.data[i].image + '")';
        }
        else if (obj.result.data[i].bgcolor !== '') {
            document.getElementsByClassName('home-icons-ligature')[0].style.backgroundColor = obj.result.data[i].bgcolor;
        }
    }
    let colsLen = cols.length - 1;
    for (let i = colsLen; i >= nrItems; i --) {
        cols[i].remove();
    }
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div><span class="material-icons">error_outline</span>&nbsp;' + t('Empty list') + '</div>';
    }
}

function dragAndDropHome() {
    const homeCards = document.getElementById('HomeCards');
    homeCards.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('home-icons')) {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragSrc = event.target;
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    homeCards.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        if (event.target.nodeName === 'DIV' && event.target.classList.contains('home-icons')) {
            event.target.classList.remove('dragover-icon');
        }
    }, false);
    homeCards.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }
        if (event.target.nodeName === 'DIV' && event.target.classList.contains('home-icons')) {
            event.target.classList.add('dragover-icon');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);
    homeCards.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }
        dragSrc.classList.remove('opacity05');
    }, false);
    homeCards.addEventListener('drop', function(event) {
        event.preventDefault();
        event.stopPropagation();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let t = event.target;
        if (t.nodeName === 'DIV') {
            if (t.classList.contains('card-body')) {
                t = t.parentNode;
            }
            if (t.classList.contains('home-icons')) {
                dragEl.classList.remove('opacity05');
                let dragDst = t.cloneNode(true);
                dragSrc.parentNode.replaceChild(dragDst, dragSrc);
                t.parentNode.replaceChild(dragSrc, t);
                //sendAPI();
            }
        }
        let th = homeCards.getElementsByClassName('dragover-icon');
        let thLen = th.length;
        for (let i = 0; i < thLen; i++) {
            th[i].classList.remove('dragover-icon');
        }

    }, false);
}
