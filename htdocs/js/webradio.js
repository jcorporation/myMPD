"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const radiobrowserTags = [
    {'key': 'name', 'desc': 'Name'},
    {'key': 'country', 'desc': 'Country'},
    {'key': 'tag', 'desc': 'Tags'}
];

function initWebradio() {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    for (const tag of radiobrowserTags) {
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": tag.key}, tn(tag.desc)));
    }
    stack.firstElementChild.classList.add('active');
    document.getElementById('BrowseRadioOnlineTags').appendChild(stack);

    document.getElementById('BrowseRadioOnlineSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto('Browse', 'Radio', undefined, 0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioOnlineList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickRadio(getData(event.target.parentNode, 'uri'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioOnlineTags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            document.getElementById('BrowseRadioOnlineTagsBtn').Dropdown.hide();
            app.current.filter = getData(event.target, 'tag');
            appGoto('Browse', 'Radio', undefined, 0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('click', function(event) {
        if (event.target.classList.contains('row')) {
            return;
        }
        if (event.target.classList.contains('card-body')) {
            if (settings.webuiSettings.clickPlaylist === 'view') {
                //show edit dialog
            }
            else {
                //load the playlist over http(s)
                const uri = window.location.protocol + '//' + window.location.hostname +
                    (window.location.port !== '' ? ':' + window.location.port : '') +
                    subdir + '/browse/webradios/' + myEncodeURI(getData(event.target.parentNode, 'uri'));
                clickPlaylist(uri);
            }
        }
        else if (event.target.classList.contains('card-footer')){
            showPopover(event);
        }
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('contextmenu', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseRadioFavoritesList').addEventListener('long-press', function(event) {
        if (event.target.classList.contains('row') ||
            event.target.parentNode.classList.contains('not-clickable'))
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('BrowseRadioFavoritesSearchStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto(app.current.card, app.current.tab, app.current.view,
                0, app.current.limit, app.current.filter, app.current.sort, '-', this.value);
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function addRadioFavorite(uri, name, genre, picture) {
    sendAPI("MYMPD_API_WEBRADIO_SAVE", {
        "uri": uri,
        "name": name,
        "genre": genre,
        "picture": picture
    }, false);
}

//eslint-disable-next-line no-unused-vars
function deleteRadioFavorite(el, name) {
    sendAPI("MYMPD_API_WEBRADIO_RM", {
        "name": name
    }, function() {
        sendAPI("MYMPD_API_WEBRADIO_LIST", {
            "offset": app.current.offset,
            "limit": app.current.limit
        }, parseWebradioList, true);
    }, false);
}

function parseWebradioList(obj) {
    const cardContainer = document.getElementById('BrowseRadioFavoritesList');

    const cols = cardContainer.getElementsByClassName('col');
    cardContainer.classList.remove('opacity05');

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateText('div', {"class": ["col", "not-clickable", "alert", "alert-danger"]}, tn(obj.error.message, obj.error.data))
        );
        setPagination(0, 0);
        return;
    }

    const nrItems = obj.result.returnedEntities;
    if (nrItems === 0) {
        elReplaceChild(cardContainer,
            elCreateText('div', {"class": ["col", "not-clickable", "alert", "alert-secondary"]}, tn('Empty list'))
        );
        setPagination(0, 0);
        return;
    }

    if (cardContainer.getElementsByClassName('not-clickable').length > 0) {
        elClear(cardContainer);
    }
    for (let i = 0; i < nrItems; i++) {
        //id is used only to check if card should be refreshed
        const id = genId('WebradioFavorite' + obj.result.data[i].uri);

        if (cols[i] !== undefined && cols[i].firstChild.firstChild.getAttribute('id') === id) {
            continue;
        }

        const card = elCreateNodes('div', {"data-popover": "webradio", "class": ["card", "card-grid", "clickable"], "tabindex": 0}, [
            elCreateEmpty('div', {"class": ["card-body", "album-cover-loading", "album-cover-grid", "d-flex"], "id": id}),
            elCreateNodes('div', {"class": ["card-footer", "card-footer-grid", "p-2"]}, [
                document.createTextNode(obj.result.data[i].PLAYLIST),
                elCreateEmpty('br', {}),
                elCreateText('small', {}, obj.result.data[i].EXTGENRE)
            ])
        ]);
        setData(card, 'picture', obj.result.data[i].EXTIMG);
        setData(card, 'uri', obj.result.data[i].uri);
        setData(card, 'type', 'plist');

        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);

        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }

        if (hasIO === true) {
            const options = {
                root: null,
                rootMargin: '0px',
            };
            const observer = new IntersectionObserver(setGridImage, options);
            observer.observe(col);
        }
        else {
            col.firstChild.firstChild.style.backgroundImage = myEncodeURI(obj.result.data[i].EXTIMG);
        }
    }
    for (let i = cols.length - 1; i >= nrItems; i--) {
        cols[i].remove();
    }

    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(cardContainer);
}

function parseRadiobrowserList(obj) {
    const table = document.getElementById('BrowseRadioOnlineList');
    const tbody = table.getElementsByTagName('tbody')[0];
    setScrollViewHeight(table);

    if (checkResult(obj, tbody) === false) {
        return;
    }

    elClear(tbody);
    const nrItems = obj.result.data.length;
    for (const station of obj.result.data) {
        const row = elCreateNodes('tr', {}, [
            elCreateText('td', {}, station.name),
            elCreateText('td', {}, station.tags.replace(/,/g, ', ')),
            elCreateText('td', {}, station.country)
        ]);
        setData(row, 'uri', station.url);
        setData(row, 'name', station.name);
        setData(row, 'genre', station.tags);
        setData(row, 'picture', station.favicon);
        setData(row, 'type', 'stream');
        row.appendChild(
            elCreateNode('td', {},
                elCreateText('a', {"data-col": "Action", "href": "#", "class": ["mi", "color-darkgrey"], "title": tn('Actions')}, ligatureMore)
            )
        );
        tbody.appendChild(row);
    }
    table.classList.remove('opacity05');

    setPagination(-1, nrItems);

    if (nrItems === 0) {
        tbody.appendChild(emptyRow(4));
    }
}
