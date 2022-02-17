"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function initHome() {
    //home screen
    document.getElementById('HomeList').addEventListener('click', function(event) {
        if (event.target.classList.contains('card-body')) {
            const href = getData(event.target.parentNode, 'href');
            if (href !== undefined) {
               parseCmd(event, href);
            }
        }
        else if (event.target.classList.contains('card-footer')){
            showPopover(event);
        }
    }, false);

    document.getElementById('HomeList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('card-body') ||
            event.target.classList.contains('card-footer'))
        {
            showPopover(event);
        }
    }, false);

    document.getElementById('HomeList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('card-body') ||
            event.target.classList.contains('card-footer'))
        {
            showPopover(event);
        }
    }, false);

    dragAndDropHome();

    //modals
    const selectHomeIconCmd = document.getElementById('selectHomeIconCmd');
    selectHomeIconCmd.addEventListener('change', function() {
        showHomeIconCmdOptions();
    }, false);

    document.getElementById('inputHomeIconBgcolor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.backgroundColor = event.target.value;
    }, false);

    document.getElementById('inputHomeIconColor').addEventListener('change', function(event) {
        document.getElementById('homeIconPreview').style.color = event.target.value;
    }, false);

    document.getElementById('inputHomeIconImage').addEventListener('change', function(event) {
        const value = getData(event.target, 'value');
        if (value !== '') {
            document.getElementById('homeIconPreview').style.backgroundImage =
                isHttpUri(value) === true ?
                    'url("' + myEncodeURIhost(value)  + '")':
                    'url("' + subdir + '/browse/pics/thumbs/' + myEncodeURI(value)  + '")';
            elHideId('divHomeIconLigature');
            elClearId('homeIconPreview');
        }
        else {
            document.getElementById('homeIconPreview').style.backgroundImage = '';
            elShowId('divHomeIconLigature');
            document.getElementById('homeIconPreview').textContent =
                document.getElementById('inputHomeIconLigature').value;
        }
    }, false);

    setDataId('inputHomeIconImage', 'cb-filter', 'filterImageSelect');
    setDataId('inputHomeIconImage', 'cb-filter-options', ['inputHomeIconImage']);

    document.getElementById('btnHomeIconLigature').parentNode.addEventListener('show.bs.dropdown', function () {
        populateHomeIconLigatures();
        const selLig = document.getElementById('inputHomeIconLigature').value;
        if (selLig !== '') {
            document.getElementById('searchHomeIconLigature').value = selLig;
            if (selLig !== '') {
                elShow(document.getElementById('searchHomeIconLigature').nextElementSibling);
            }
            else {
                elHide(document.getElementById('searchHomeIconLigature').nextElementSibling);
            }
            filterHomeIconLigatures();
        }
    }, false);

    document.getElementById('listHomeIconLigature').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            event.preventDefault();
            selectHomeIconLigature(event.target);
            uiElements.dropdownHomeIconLigature.hide();
        }
    });

    document.getElementById('searchHomeIconLigature').addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    const searchHomeIconCat = document.getElementById('searchHomeIconCat');
    searchHomeIconCat.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);

    searchHomeIconCat.addEventListener('change', function() {
        filterHomeIconLigatures();
    }, false);

    const searchHomeIconLigature = document.getElementById('searchHomeIconLigature');
    searchHomeIconLigature.addEventListener('keydown', function(event) {
        event.stopPropagation();
        if (event.key === 'Enter') {
            event.preventDefault();
        }
    }, false);

    searchHomeIconLigature.addEventListener('keyup', function(event) {
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

function populateHomeIconLigatures() {
    const listHomeIconLigature = document.getElementById('listHomeIconLigature');
    const searchHomeIconCat = document.getElementById('searchHomeIconCat');
    if (searchHomeIconCat.firstChild !== null) {
        return;
    }
    elClear(listHomeIconLigature);
    elClear(searchHomeIconCat);
    searchHomeIconCat.appendChild(
        elCreateText('option', {"value": "all"}, tn('All'))
    );
    for (const cat in materialIcons) {
        listHomeIconLigature.appendChild(
            elCreateText('h5', {"class": ["ml-1", "mt-2"]}, ucFirst(cat))
        );
        searchHomeIconCat.appendChild(
            elCreateText('option', {"value": cat}, ucFirst(cat))
        );
        for (const icon of materialIcons[cat]) {
            listHomeIconLigature.appendChild(
                elCreateText('button', {"class": ["btn", "btn-sm", "mi", "m-1"], "title": icon, "data-cat": cat}, icon)
            );
        }
    }
}

function selectHomeIconLigature(x) {
    document.getElementById('inputHomeIconLigature').value = x.getAttribute('title');
    document.getElementById('homeIconPreview').textContent = x.getAttribute('title');
    document.getElementById('homeIconPreview').style.backgroundImage = '';
    document.getElementById('inputHomeIconImage').value = tn('Use ligature');
    setData(document.getElementById('inputHomeIconImage'), 'value', '');
}

function filterHomeIconLigatures() {
    const str = document.getElementById('searchHomeIconLigature').value.toLowerCase();
    const cat = getSelectValueId('searchHomeIconCat');
    const els = document.getElementById('listHomeIconLigature').getElementsByTagName('button');
    for (let i = 0, j = els.length; i < j; i++) {
        if ((str === '' || els[i].getAttribute('title').indexOf(str) > -1) &&
            (cat === 'all' || els[i].getAttribute('data-cat') === cat))
        {
            elShow(els[i]);
            if (els[i].getAttribute('title') === str) {
                els[i].classList.add('active');
            }
            else {
                els[i].classList.remove('active');
            }
        }
        else {
            elHide(els[i]);
            els[i].classList.remove('active' );
        }
    }
    const catTitles = document.getElementById('listHomeIconLigature').getElementsByTagName('h5');
    if (cat === '') {
        for (let i = 0, j = catTitles.length; i < j; i++) {
            elShow(catTitles[i]);
        }
    }
    else {
        for (let i = 0, j = catTitles.length; i < j; i++) {
            elHide(catTitles[i]);
        }
    }
}

const friendlyActions = {
    'replaceQueue': 'Replace queue',
    'replacePlayQueue': 'Replace queue and play',
    'insertAfterCurrentQueue': 'Insert after current playing song',
    'appendQueue': 'Append to queue',
    'appendPlayQueue': 'Append to queue and play',
    'replaceQueueAlbum': 'Replace queue',
    'replacePlayQueueAlbum': 'Replace queue and play',
    'insertAfterCurrentQueueAlbum': 'Insert after current playing song',
    'appendQueueAlbum': 'Append to queue',
    'appendPlayQueueAlbum': 'Append to queue and play',
    'appGoto': 'Show',
    'homeIconGoto': 'Show',
    'execScriptFromOptions': 'Execute script'
};

function parseHome(obj) {
    const cardContainer = document.getElementById('HomeList');
    const cols = cardContainer.getElementsByClassName('col');
    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateText('div', {"class": ["ms-3", "mb-3", "not-clickable", "alert", "alert-danger"]}, tn(obj.error.message, obj.error.data))
        );
        setPagination(obj.result.totalEntities, obj.result.returnedEntities);
        return;
    }
    if (cols.length === 0) {
        elClear(cardContainer);
    }
    if (obj.result.returnedEntities === 0) {
        elClear(cardContainer);
        const div = elCreateNodes('div', {"class": ["px-3", "py-1"]}, [
            elCreateText('h3', {}, tn('Homescreen')),
            elCreateNodes('p', {}, [
                document.createTextNode(tn('Homescreen welcome')),
                elCreateText('span', {"class": ["mi"]}, 'add_to_home_screen'),
                document.createTextNode(' '),
                elCreateText('span', {"class": ["mi"]}, 'library_add')
            ])
        ]);
        cardContainer.appendChild(div);
        return;
    }
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const col = elCreateEmpty('div', {"class": ["col", "px-0", "flex-grow-0"]});
        const homeType = obj.result.data[i].cmd === 'appGoto' ? 'View' :
            obj.result.data[i].cmd === 'execScriptFromOptions' ? 'Script' :
            typeFriendly[obj.result.data[i].options[0]];
        const actionType = friendlyActions[obj.result.data[i].cmd];

        const card = elCreateEmpty('div', {"data-popover": "home", "class": ["card", "home-icons"], "draggable": "true",
            "title": tn(homeType) + ':' + smallSpace + obj.result.data[i].name +
            '\n' + tn(actionType)});
        //decode json options
        for (let j = 0, k = obj.result.data[i].options.length; j < k; j++) {
            if (obj.result.data[i].options[j].indexOf('{"') === 0 ||
                obj.result.data[i].options[j].indexOf('["') === 0)
            {
                obj.result.data[i].options[j] = JSON.parse(obj.result.data[i].options[j]);
            }
        }

        setData(card, 'href', {"cmd": obj.result.data[i].cmd, "options": obj.result.data[i].options});
        setData(card, 'pos', i);
        const cardBody = elCreateText('div', {"class": ["card-body", "mi", "rounded", "clickable"]}, obj.result.data[i].ligature);
        if (obj.result.data[i].image !== '') {
            cardBody.style.backgroundImage = isHttpUri(obj.result.data[i].image) === true ?
                'url("' + myEncodeURIhost(obj.result.data[i].image) +'")' :
                'url("' + subdir + '/browse/pics/thumbs/' + myEncodeURI(obj.result.data[i].image) + '")';
        }
        if (obj.result.data[i].bgcolor !== '') {
            cardBody.style.backgroundColor = obj.result.data[i].bgcolor;
        }
        if (obj.result.data[i].color !== '' &&
            obj.result.data[i].color !== undefined)
        {
            cardBody.style.color = obj.result.data[i].color;
        }
        card.appendChild(cardBody);
        card.appendChild(elCreateText('div', {"class": ["card-footer", "card-footer-grid", "p-2", "clickable"]}, obj.result.data[i].name));
        col.appendChild(card);
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
    }
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }
}

function dragAndDropHome() {
    const HomeList = document.getElementById('HomeList');

    HomeList.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('home-icons')) {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragSrc = event.target;
            dragEl = event.target.cloneNode(true);
        }
    }, false);

    HomeList.addEventListener('dragleave', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        if (event.target.nodeName === 'DIV' &&
            event.target.classList.contains('home-icons'))
        {
            event.target.classList.remove('dragover-icon');
        }
    }, false);

    HomeList.addEventListener('dragover', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        const ths = HomeList.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
        if (event.target.nodeName === 'DIV' &&
            event.target.classList.contains('home-icons'))
        {
            event.target.classList.add('dragover-icon');
        }
        else if (event.target.nodeName === 'DIV' &&
                 event.target.parentNode.classList.contains('home-icons'))
        {
            event.target.parentNode.classList.add('dragover-icon');
        }
        event.dataTransfer.dropEffect = 'move';
    }, false);

    HomeList.addEventListener('dragend', function(event) {
        event.preventDefault();
        if (dragEl.classList.contains('home-icons') === false) {
            return;
        }
        const ths = HomeList.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
        dragSrc.classList.remove('opacity05');
    }, false);

    HomeList.addEventListener('drop', function(event) {
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
                const to = getData(dst, 'pos');
                const from = getData(dragSrc, 'pos');
                if (isNaN(to) === false &&
                    isNaN(from) === false &&
                    from !== to)
                {
                    sendAPI("MYMPD_API_HOME_ICON_MOVE", {"from": from, "to": to}, function(obj) {
                        parseHome(obj);
                    });
                }
            }
        }
        const ths = HomeList.getElementsByClassName('dragover-icon');
        for (const th of ths) {
            th.classList.remove('dragover-icon');
        }
    }, false);
}

function populateHomeIconCmdSelect(cmd, type) {
    const selectHomeIconCmd = document.getElementById('selectHomeIconCmd');
    elClear(selectHomeIconCmd);
    if (cmd === 'appGoto') {
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "appGoto"}, tn('Goto view')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["App", "Tab", "View", "Offset", "Limit", "Filter", "Sort", "Tag", "Search"]});
    }
    else if (cmd === 'execScriptFromOptions') {
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "execScriptFromOptions"}, tn('Execute Script')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options":["Script", "Arguments"]});
    }
    else if (type === 'album') {
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "replaceQueueAlbum"}, tn('Replace queue')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "replacePlayQueueAlbum"}, tn('Replace queue and play')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
        if (features.featWhence === true) {
            selectHomeIconCmd.appendChild(elCreateText('option', {"value": "insertAfterCurrentQueueAlbum"}, tn('Insert after current playing song')));
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
        }
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "appendQueueAlbum"}, tn('Append to queue')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "appendPlayQueueAlbum"}, tn('Append to queue and play')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "homeIconGoto"}, tn('Album details')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", "Albumartist", "Album"]});
    }
    else {
        const paramName = type === 'search' ? 'Expression' : 'Uri';
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "replaceQueue"}, tn('Replace queue')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "replacePlayQueue"}, tn('Replace queue and play')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        if (features.featWhence === true) {
            selectHomeIconCmd.appendChild(elCreateText('option', {"value": "insertAfterCurrentQueue"}, tn('Insert after current playing song')));
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        }
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "appendQueue"}, tn('Append to queue')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        selectHomeIconCmd.appendChild(elCreateText('option', {"value": "appendPlayQueue"}, tn('Append to queue and play')));
        setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        if (type === 'dir' ||
            type === 'search' ||
            type === 'plist' ||
            type === 'smartpls')
        {
            const title = type === 'dir' ? 'Show directory' : 
                          type === 'search' ? 'Show search' : 'View playlist';
            selectHomeIconCmd.appendChild(elCreateText('option', {"value": "homeIconGoto"}, tn(title)));
            setData(selectHomeIconCmd.lastChild, 'options', {"options": ["Type", paramName]});
        }
    }
}

//eslint-disable-next-line no-unused-vars
function executeHomeIcon(pos) {
    const el = document.getElementById('HomeList').children[pos].firstChild;
    parseCmd(null, getData(el, 'href'));
}

//eslint-disable-next-line no-unused-vars
function addViewToHome() {
    _addHomeIcon('appGoto', '', 'preview', '', [app.current.card, app.current.tab, app.current.view,
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search]);
}

//eslint-disable-next-line no-unused-vars
function addScriptToHome(name, script) {
    const options = [script.script, script.arguments.join(',')];
    _addHomeIcon('execScriptFromOptions', name, 'code', '', options);
}

//eslint-disable-next-line no-unused-vars
function addPlistToHome(uri, type, name) {
    _addHomeIcon('replaceQueue', name, 'list', '', [type, uri]);
}

//eslint-disable-next-line no-unused-vars
function addRadioFavoriteToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

//eslint-disable-next-line no-unused-vars
function addWebRadiodbToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

//eslint-disable-next-line no-unused-vars
function addDirToHome(uri, name) {
    if(uri === undefined) {
        uri = app.current.search;
        name = basename(app.current.search, false);
    }
    _addHomeIcon('replaceQueue', name, 'folder_open', '', ['dir', uri]);
}

//eslint-disable-next-line no-unused-vars
function addSongToHome(uri, type, name) {
    const ligature = type === 'song' ? 'music_note' : 'stream';
    _addHomeIcon('replaceQueue', name, ligature, '', [type, uri]);
}

//eslint-disable-next-line no-unused-vars
function addSearchToHome() {
    _addHomeIcon('replaceQueue', tn('Current search'), '', 'saved_search', ['search', app.current.search]);
}

//eslint-disable-next-line no-unused-vars
function addAlbumToHome(albumArtist, album) {
    if (albumArtist === undefined) {
        album = app.current.tag;
        albumArtist = app.current.search;
    }
    _addHomeIcon('replaceQueueAlbum', album, 'album', '', ['album', JSON.stringify(albumArtist), album]);
}

//eslint-disable-next-line no-unused-vars
function addStreamToHome() {
    const mode = getRadioBoxValueId('addToPlaylistPos');
    const uri = document.getElementById('streamUrl').value;
    let action;
    switch(mode) {
        case 'append': action = 'appendQueue'; break;
        case 'appendPlay': action = 'appendPlayQueue'; break;
        case 'insertAfterCurrent': action = 'insertAfterCurrentQueue'; break;
        case 'insertPlayAfterCurrent': action = 'insertPlayAfterCurrentQueue'; break;
        case 'replace': action = 'replaceQueue'; break;
        case 'replacePlay': action = 'replacePlayQueue'; break;
    }
    _addHomeIcon(action, '', 'stream', '', ['stream', uri]);
}

function _addHomeIcon(cmd, name, ligature, image, options) {
    document.getElementById('modalEditHomeIconTitle').textContent = tn('Add to homescreen');
    document.getElementById('inputHomeIconReplace').value = 'false';
    document.getElementById('inputHomeIconOldpos').value = '0';
    document.getElementById('inputHomeIconName').value = name;
    document.getElementById('inputHomeIconBgcolor').value = '#28a745';
    document.getElementById('inputHomeIconColor').value = '#ffffff';

    populateHomeIconCmdSelect(cmd, options[0]);
    document.getElementById('selectHomeIconCmd').value = cmd;
    showHomeIconCmdOptions(options);
    getHomeIconPictureList();
    const homeIconPreviewEl = document.getElementById('homeIconPreview');
    const homeIconImageInput = document.getElementById('inputHomeIconImage');
    if (image !== '') {
        homeIconImageInput.value = image;
        setData(homeIconImageInput, 'value', image);
        document.getElementById('inputHomeIconLigature').value = '';
        elClear(homeIconPreviewEl);
        homeIconPreviewEl.style.backgroundImage =
            isHttpUri(image) === true ?
                'url("' + myEncodeURIhost(image) +'")' :
                'url("' + subdir + '/browse/pics/thumbs/' + myEncodeURI(image) + '")';
        elHideId('divHomeIconLigature');
    }
    else {
        //use ligature
        homeIconImageInput.value = tn('Use ligature');
        setData(homeIconImageInput, 'value', '');
        document.getElementById('inputHomeIconLigature').value = ligature;
        homeIconPreviewEl.textContent = ligature;
        homeIconPreviewEl.style.backgroundImage = '';
        elShowId('divHomeIconLigature');
    }

    homeIconPreviewEl.style.backgroundColor = '#28a745';
    homeIconPreviewEl.style.color = '#ffffff';
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
    document.getElementById('modalEditHomeIconTitle').textContent = tn(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        document.getElementById('inputHomeIconReplace').value = replace;
        document.getElementById('inputHomeIconOldpos').value = pos;
        document.getElementById('inputHomeIconName').value = obj.result.data.name;
        document.getElementById('inputHomeIconLigature').value = obj.result.data.ligature;
        document.getElementById('inputHomeIconBgcolor').value = obj.result.data.bgcolor;
        document.getElementById('inputHomeIconColor').value = obj.result.data.color;

        populateHomeIconCmdSelect(obj.result.data.cmd, obj.result.data.options[0]);
        document.getElementById('selectHomeIconCmd').value = obj.result.data.cmd;
        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList();
        document.getElementById('inputHomeIconImage').value = obj.result.data.image === '' ? tn('Use ligature') : obj.result.data.image;
        setData(document.getElementById('inputHomeIconImage'),'value', obj.result.data.image);

        document.getElementById('homeIconPreview').textContent = obj.result.data.ligature;
        document.getElementById('homeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        document.getElementById('homeIconPreview').style.color = obj.result.data.color;

        if (obj.result.data.image === '') {
            elShowId('divHomeIconLigature');
            document.getElementById('homeIconPreview').style.backgroundImage = '';
        }
        else {
            elHideId('divHomeIconLigature');
            document.getElementById('homeIconPreview').style.backgroundImage =
                isHttpUri(obj.result.data.image) === true ?
                    'url("' + myEncodeURIhost(obj.result.data.image) +'")' :
                    'url("' + subdir + '/browse/pics/thumbs/' + myEncodeURI(obj.result.data.image) + '")';
        }
        //reset ligature selection
        document.getElementById('searchHomeIconLigature').value = '';
        document.getElementById('searchHomeIconCat').value = 'all';
        filterHomeIconLigatures();
        //show modal
        cleanupModalId('modalEditHomeIcon');
        uiElements.modalEditHomeIcon.show();
    });
}

//eslint-disable-next-line no-unused-vars
function saveHomeIcon() {
    cleanupModalId('modalEditHomeIcon');
    let formOK = true;
    const nameEl = document.getElementById('inputHomeIconName');
    if (!validateNotBlank(nameEl)) {
        formOK = false;
    }
    if (formOK === true) {
        const options = [];
        const optionEls = document.getElementById('divHomeIconOptions').getElementsByTagName('input');
        for (const optionEl of optionEls) {
            options.push(optionEl.value);
        }
        const image = getData(document.getElementById('inputHomeIconImage'), 'value');
        sendAPI("MYMPD_API_HOME_ICON_SAVE", {
            "replace": strToBool(document.getElementById('inputHomeIconReplace').value),
            "oldpos": Number(document.getElementById('inputHomeIconOldpos').value),
            "name": nameEl.value,
            "ligature": (image === '' ? document.getElementById('inputHomeIconLigature').value : ''),
            "bgcolor": document.getElementById('inputHomeIconBgcolor').value,
            "color": document.getElementById('inputHomeIconColor').value,
            "image": image,
            "cmd": document.getElementById('selectHomeIconCmd').value,
            "options": options
        }, saveHomeIconClose, true);
    }
}

function saveHomeIconClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        uiElements.modalEditHomeIcon.hide();
        sendAPI("MYMPD_API_HOME_LIST", {}, function(obj2) {
            parseHome(obj2);
        });
    }
}

//eslint-disable-next-line no-unused-vars
function deleteHomeIcon(pos) {
    sendAPI("MYMPD_API_HOME_ICON_RM", {"pos": pos}, function(obj) {
        parseHome(obj);
    });
}

function showHomeIconCmdOptions(values) {
    const oldOptions = [];
    const optionEls = document.getElementById('divHomeIconOptions').getElementsByTagName('input');
    for (const optionEl of optionEls) {
        oldOptions.push(optionEl.value);
    }
    const divHomeIconOptions = document.getElementById('divHomeIconOptions');
    elClear(divHomeIconOptions);
    const options = getSelectedOptionDataId('selectHomeIconCmd', 'options');
    if (options !== undefined) {
        for (let i = 0, j = options.options.length; i < j; i++) {
            let value = values !== undefined ? values[i] !== undefined ? values[i] : '' : '';
            if (value === '' &&
                oldOptions[i] !== undefined) {
                value = oldOptions[i];
            }
            if (typeof value === 'object') {
                value = JSON.stringify(value);
            }
            const row = elCreateNodes('div', {"class": ["mb-3", "row"]}, [
                elCreateText('label', {"class": ["col-sm-4"]}, tn(options.options[i])),
                elCreateNode('div', {"class": ["col-sm-8"]}, 
                    elCreateEmpty('input', {"class": ["form-control", "border-secondary"], "name": options.options[i], "value": value})
                )
            ]);
            divHomeIconOptions.appendChild(row);
        }
    }
}

function getHomeIconPictureList() {
    const selectHomeIconImage = document.getElementById('inputHomeIconImage');
    getImageList(selectHomeIconImage, [{"value": "", "text": tn('Use ligature')}], 'thumbs');
}

//eslint-disable-next-line no-unused-vars
function homeIconGoto(type, uri, album) {
    switch(type) {
        case 'dir':
            gotoFilesystem(uri);
            break;
        case 'search':
            appGoto('Search', undefined, undefined, 0, undefined, 'any', 'Title', '-', uri);
            break;
        case 'album':
            //uri = AlbumArtist
            gotoAlbum(uri, album);
            break;
        case 'plist':
        case 'smartpls':
            playlistDetails(uri);
            break;
    }
}
