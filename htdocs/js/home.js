"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initHome() {
    //home screen
    document.getElementById('HomeCards').addEventListener('click', function(event) {
        if (event.target.classList.contains('card-body')) {
            const href = getCustomDomProperty(event.target.parentNode, 'data-href');
            if (href !== undefined) {
               parseCmd(event, href);
            }
        }
        else if (event.target.classList.contains('card-footer')){
            popoverMenuHome(event);
        }
    }, false);
    
    document.getElementById('HomeCards').addEventListener('contextmenu', function(event) {
        popoverMenuHome(event);
    }, false);

    document.getElementById('HomeCards').addEventListener('long-press', function(event) {
        popoverMenuHome(event);
    }, false);
    
    document.getElementById('HomeCards').addEventListener('keydown', function(event) {
        navigateGrid(event.target, event.key);
    }, false);
    
    dragAndDropHome();

    //modals
    document.getElementById('selectHomeIconCmd').addEventListener('change', function() {
        showHomeIconCmdOptions();
    }, false);

    document.getElementById('inputHomeIconBgcolor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.backgroundColor = event.target.value;
    }, false);

    document.getElementById('selectHomeIconImage').addEventListener('change', function(event) {
        const value = getSelectValue(event.target);
        document.getElementById('homeIconPreview').style.backgroundImage = 'url("' + subdir + '/pics/' + myEncodeURI(value)  + '")';
        if (value !== '') {
            document.getElementById('divHomeIconLigature').classList.add('hide');
            document.getElementById('homeIconPreview').innerHTML = '';
        }
        else {
            document.getElementById('divHomeIconLigature').classList.remove('hide');
            document.getElementById('homeIconPreview').textContent = document.getElementById('inputHomeIconLigature').value;
        }
    }, false);
    
    document.getElementById('btnHomeIconLigature').parentNode.addEventListener('show.bs.dropdown', function () {
        const selLig = document.getElementById('inputHomeIconLigature').value;
        if (selLig !== '') {
            document.getElementById('searchHomeIconLigature').value = selLig;
            filterHomeIconLigatures();
        }
    }, false);
    
    let ligatureList = '';
    let catList = '<option value="all">' + t('All') + '</option>';
    for (const cat in materialIcons) {
        ligatureList += '<h5 class="ml-1 mt-2">' + e(ucFirst(cat)) + '</h5>';
        catList += '<option value="' + cat + '">' + e(ucFirst(cat)) + '</option>';
        for (let i = 0, j = materialIcons[cat].length; i < j; i++) {
            ligatureList += '<button title="' + materialIcons[cat][i] + '" data-cat="' + cat + '" class="btn btn-sm mi m-1">' + materialIcons[cat][i] + '</button>';
        }
    }
    document.getElementById('listHomeIconLigature').innerHTML = ligatureList;
    document.getElementById('searchHomeIconCat').innerHTML = catList;

    document.getElementById('listHomeIconLigature').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            selectHomeIconLigature(event.target);
        }
    });
    
    document.getElementById('searchHomeIconLigature').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    document.getElementById('searchHomeIconCat').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    
    document.getElementById('searchHomeIconCat').addEventListener('change', function() {
        filterHomeIconLigatures();
    }, false);
    
    document.getElementById('searchHomeIconLigature').addEventListener('keydown', function(event) {
        event.stopPropagation();
        if (event.key === 'Enter') {
            event.preventDefault();
        }
    }, false);
    
    document.getElementById('searchHomeIconLigature').addEventListener('keyup', function(event) {
        if (event.key === 'Enter') {
            const sel = document.getElementById('listHomeIconLigature').getElementsByClassName('active')[0];
            if (sel !== undefined) {
                selectHomeIconLigature(sel);
                uiElements.dropdownHomeIconLigature.toggle();
            }
        }
        else {
            filterHomeIconLigatures();
        }
    }, false);
}

function selectHomeIconLigature(x) {
    document.getElementById('inputHomeIconLigature').value = x.getAttribute('title');
    document.getElementById('homeIconPreview').textContent = x.getAttribute('title');
    document.getElementById('homeIconPreview').style.backgroundImage = '';
    document.getElementById('selectHomeIconImage').value = '';
}

function filterHomeIconLigatures() {
    const str = document.getElementById('searchHomeIconLigature').value.toLowerCase();
    const cat = getSelectValue('searchHomeIconCat');
    const els = document.getElementById('listHomeIconLigature').getElementsByTagName('button');
    for (let i = 0, j = els.length; i < j; i++) {
        if ((str === '' || els[i].getAttribute('title').indexOf(str) > -1) && (cat === 'all' || els[i].getAttribute('data-cat') === cat)) {
            els[i].classList.remove('hide');
            if (els[i].getAttribute('title') === str) {
                els[i].classList.add('active');
            }
            else {
                els[i].classList.remove('active');
            }
        }
        else {
            els[i].classList.add('hide');
            els[i].classList.remove('active' );
        }
    }
    const catTitles = document.getElementById('listHomeIconLigature').getElementsByTagName('h5');
    if (cat === '') {
        for (let i = 0, j = catTitles.length; i < j; i++) {
            catTitles[i].classList.remove('hide');
        }
    }
    else {
        for (let i = 0, j = catTitles.length; i < j; i++) {
            catTitles[i].classList.add('hide');
        }
    }
}

function parseHome(obj) {
    const nrItems = obj.result.returnedEntities;
    const cardContainer = document.getElementById('HomeCards');
    const cols = cardContainer.getElementsByClassName('col');
    if (cols.length === 0) {
        cardContainer.innerHTML = '';
    }
    for (let i = 0; i < nrItems; i++) {
        const col = document.createElement('div');
        col.classList.add('col', 'px-0', 'flex-grow-0');
        if (obj.result.data[i].AlbumArtist === '') {
            obj.result.data[i].AlbumArtist = t('Unknown artist');
        }
        if (obj.result.data[i].Album === '') {
            obj.result.data[i].Album = t('Unknown album');
        }
        const homeType = obj.result.data[i].cmd === 'replaceQueue' ? 'Playlist' :
            obj.result.data[i].cmd === 'appGoto' ? 'View' : 'Script';
        
        const href = JSON.stringify({"cmd": obj.result.data[i].cmd, "options": obj.result.data[i].options});
        const html = '<div class="card home-icons clickable" draggable="true" tabindex="0" ' + 
                   'title="' + t(homeType) +': ' + e(obj.result.data[i].name) + '">' +
                   '<div class="card-body mi rounded">' + e(obj.result.data[i].ligature) + '</div>' +
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
        setCustomDomProperty(col.firstChild, 'data-href', href);
        setCustomDomProperty(col.firstChild, 'data-pos', i);
        if (obj.result.data[i].image !== '') {
            col.getElementsByClassName('card-body')[0].style.backgroundImage = 'url("' + subdir + '/pics/' + myEncodeURI(obj.result.data[i].image) + '")';
        }
        if (obj.result.data[i].bgcolor !== '') {
            col.getElementsByClassName('card-body')[0].style.backgroundColor = obj.result.data[i].bgcolor;
        }
    }
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }
                    
    if (nrItems === 0) {
        cardContainer.innerHTML = '<div class="ml-3"><h3>' + t('Homescreen') + '</h3><p>' + t('Homescreen welcome') + '</p>' +
            '<ul>' +
            '<li><b>' + t('View') + '</b>: ' + t('Homescreen help view') + '</li>' + 
            '<li><b>' + t('Playlist') + '</b>: ' + t('Homescreen help playlist') + '</li>' +
            (features.featScripting === true ? '<li><b>' + t('Script') + '</b>: ' + t('Homescreen help script') + '</li>' : '') +
            '</div>';
    }
}

function popoverMenuHome(event) {
    const sels = document.getElementById('HomeCards').getElementsByClassName('selected');
    for (let i = 0, j = sels.length; i < j; i++) {
        sels[i].classList.remove('selected');
    }
    event.target.parentNode.classList.add('selected');
    showMenu(event.target, event);
    event.preventDefault();
    event.stopPropagation();
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
        const ths = homeCards.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
        if (event.target.nodeName === 'DIV' && event.target.classList.contains('home-icons')) {
            event.target.classList.add('dragover-icon');
        }
        else if (event.target.nodeName === 'DIV' && event.target.parentNode.classList.contains('home-icons')) {
            event.target.parentNode.classList.add('dragover-icon');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);

    homeCards.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        const ths = homeCards.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
        dragSrc.classList.remove('opacity05');
    }, false);

    homeCards.addEventListener('drop', function(event) {
        event.preventDefault();
        event.stopPropagation();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        let dst = event.target;
        if (dst.nodeName === 'DIV') {
            if (dst.classList.contains('card-body')) {
                dst = dst.parentNode;
            }
            if (dst.classList.contains('home-icons')) {
                dragEl.classList.remove('opacity05');
                const to = getCustomDomProperty(dst, 'data-pos');
                const from = getCustomDomProperty(dragSrc, 'data-pos');
                if (isNaN(to) === false && isNaN(from) === false && from !== to) {
                    sendAPI("MYMPD_API_HOME_ICON_MOVE", {"from": from, "to": to}, function(obj) {
                        parseHome(obj);
                    });
                }
            }
        }
        const ths = homeCards.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function executeHomeIcon(pos) {
    const el = document.getElementById('HomeCards').children[pos].firstChild;
    parseCmd(null, getCustomDomProperty(el, 'data-href'));
}

//eslint-disable-next-line no-unused-vars
function addViewToHome() {
    _addHomeIcon('appGoto', '', 'preview', [app.current.app, app.current.tab, app.current.view, 
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search]); 
}

//eslint-disable-next-line no-unused-vars
function addScriptToHome(name, scriptDef) {
    const script = JSON.parse(scriptDef);
    const options = [script.script, script.arguments.join(',')];
    _addHomeIcon('execScriptFromOptions', name, 'description', options);
}

//eslint-disable-next-line no-unused-vars
function addPlistToHome(uri, name) {
    _addHomeIcon('replaceQueue', name, 'list', ['plist', uri, name]);
}

function _addHomeIcon(cmd, name, ligature, options) {
    document.getElementById('modalEditHomeIconTitle').innerHTML = t('Add to homescreen');
    document.getElementById('inputHomeIconReplace').value = 'false';
    document.getElementById('inputHomeIconOldpos').value = '0';
    document.getElementById('inputHomeIconName').value = name;
    document.getElementById('inputHomeIconLigature').value = ligature;
    document.getElementById('inputHomeIconBgcolor').value = '#28a745';
    document.getElementById('selectHomeIconCmd').value = cmd;
    
    showHomeIconCmdOptions(options);
    getHomeIconPictureList('');
    
    document.getElementById('homeIconPreview').textContent = ligature;
    document.getElementById('homeIconPreview').style.backgroundColor = '#28a745';
    document.getElementById('homeIconPreview').style.backgroundImage = '';
    document.getElementById('divHomeIconLigature').classList.remove('hide');
    uiElements.modalEditHomeIcon.show();
}

//eslint-disable-next-line no-unused-vars
function duplicateHomeIcon(pos) {
    _editHomeIcon(pos, false, "Duplicate home icon");
}

//eslint-disable-next-line no-unused-vars
function editHomeIcon(pos) {
    _editHomeIcon(pos, true, "Edit home icon");
}

function _editHomeIcon(pos, replace, title) {
    document.getElementById('modalEditHomeIconTitle').innerHTML = t(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        document.getElementById('inputHomeIconReplace').value = replace;
        document.getElementById('inputHomeIconOldpos').value = pos;
        document.getElementById('inputHomeIconName').value = obj.result.data.name;
        document.getElementById('inputHomeIconLigature').value = obj.result.data.ligature;
        document.getElementById('inputHomeIconBgcolor').value = obj.result.data.bgcolor;
        document.getElementById('selectHomeIconCmd').value = obj.result.data.cmd;

        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList(obj.result.data.image);

        document.getElementById('homeIconPreview').textContent = obj.result.data.ligature;
        document.getElementById('homeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        
        if (obj.result.data.image === '') {
            document.getElementById('divHomeIconLigature').classList.remove('hide');
            document.getElementById('homeIconPreview').style.backgroundImage = '';
        }
        else {
            document.getElementById('divHomeIconLigature').classList.add('hide');
            document.getElementById('homeIconPreview').style.backgroundImage = 'url(' + subdir + '"/pics/' + myEncodeURI(obj.result.data.image) + '")';
        }
        //reset ligature selection
        document.getElementById('searchHomeIconLigature').value = '';
        document.getElementById('searchHomeIconCat').value = 'all';
        filterHomeIconLigatures();
        //show modal
        uiElements.modalEditHomeIcon.show();
    });
}

//eslint-disable-next-line no-unused-vars
function saveHomeIcon() {
    let formOK = true;
    const nameEl = document.getElementById('inputHomeIconName');
    if (!validateNotBlank(nameEl)) {
        formOK = false;
    }
    if (formOK === true) {
        const options = [];
        const optionEls = document.getElementById('divHomeIconOptions').getElementsByTagName('input');
        for (const optionEl of optionEls) {
            //workarround for parsing arrays with empty values in frozen
            options.push(optionEl.value !== '' ? optionEl.value : '!undefined!');
        }
        const image = getSelectValue('selectHomeIconImage');
        sendAPI("MYMPD_API_HOME_ICON_SAVE", {
            "replace": (document.getElementById('inputHomeIconReplace').value === 'true' ? true : false),
            "oldpos": Number(document.getElementById('inputHomeIconOldpos').value),
            "name": nameEl.value,
            "ligature": (image === '' ? document.getElementById('inputHomeIconLigature').value : ''),
            "bgcolor": document.getElementById('inputHomeIconBgcolor').value,
            "image": image,
            "cmd": document.getElementById('selectHomeIconCmd').value,
            "options": options
            }, function() {
                uiElements.modalEditHomeIcon.hide();
                sendAPI("MYMPD_API_HOME_LIST", {}, function(obj) {
                    parseHome(obj);
                });
            });
    }
}

//eslint-disable-next-line no-unused-vars
function deleteHomeIcon(pos) {
    sendAPI("MYMPD_API_HOME_ICON_DELETE", {"pos": pos}, function(obj) {
        parseHome(obj);
    });
}

function showHomeIconCmdOptions(values) {
    let list = '';
    const optionsText = getSelectedOptionAttribute('selectHomeIconCmd', 'data-options');
    if (optionsText !== undefined) {    
        const options = JSON.parse(optionsText);
        for (let i = 0, j = options.options.length; i < j; i++) {
            list += '<div class="form-group row">' +
                '<label class="col-sm-4 col-form-label">' + t(options.options[i]) + '</label>' +
                '<div class="col-sm-8"><input class="form-control border-secondary" value="' + 
                e(values !== undefined ? values[i] !== undefined ? values[i] : '' : '') + '"></div>' +
                '</div>';
        }
    }
    document.getElementById('divHomeIconOptions').innerHTML = list;
}

function getHomeIconPictureList(picture) {
    getImageList('selectHomeIconImage', picture, [{"value":"","text":"Use ligature"}]);
}
