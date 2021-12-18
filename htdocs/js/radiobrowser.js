"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

const radiobrowserTags = [
    {'key': 'name', 'desc': 'Name'},
    {'key': 'country', 'desc': 'Country'},
    {'key': 'tag', 'desc': 'Tags'}
];

function initRadioBrowser() {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    for (const tag of radiobrowserTags) {
        stack.appendChild(elCreateText('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": tag.key}, tn(tag.desc)));
    }
    stack.firstElementChild.classList.add('active');
    document.getElementById('radiobrowsetags').appendChild(stack);

    document.getElementById('searchRadioStr').addEventListener('keyup', function(event) {
        if (event.key === 'Escape') {
            this.blur();
        }
        else {
            appGoto('Browse', 'Radio', undefined, 0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);

    document.getElementById('BrowseRadioList').addEventListener('click', function(event) {
        if (event.target.nodeName === 'TD') {
            clickRadio(getData(event.target.parentNode, 'uri'));
        }
        else if (event.target.nodeName === 'A') {
            showPopover(event);
        }
    }, false);

    document.getElementById('radiobrowsetags').addEventListener('click', function(event) {
        if (event.target.nodeName === 'BUTTON') {
            document.getElementById('radiobrowsetagsBtn').Dropdown.hide();
            app.current.filter = getData(event.target, 'tag');
            appGoto('Browse', 'Radio', undefined, 0, app.current.limit, app.current.filter, '-', '-', this.value);
        }
    }, false);
}

function radiobrowserSearch() {
    document.getElementById('BrowseRadioList').classList.add('opacity05');
    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('GET', 'https://de1.api.radio-browser.info/json/stations/search' +
        '?' + app.current.filter + '=' + myEncodeURI(app.current.search) +
        '&limit=' + app.current.limit + '&offset=' + app.current.offset + '&hidebroken=true', true);
    ajaxRequest.setRequestHeader('User-Agent', 'myMPD/' + settings.mympdVersion);
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.status === 200 &&
            ajaxRequest.responseText !== '')
        {
            let obj;
            try {
                obj = JSON.parse(ajaxRequest.responseText);
            }
            catch(error) {
                return;
            }
            parseStationList(obj);
        }
    };
    ajaxRequest.send();
}

function parseStationList(obj) {
    const table = document.getElementById('BrowseRadioList');
    setScrollViewHeight(table);
    const tbody = table.getElementsByTagName('tbody')[0];
    elClear(tbody);
    const nrItems = obj.length;
    for (const station of obj) {
        const row = elCreateNodes('tr', {}, [
            elCreateText('td', {}, station.name),
            elCreateText('td', {}, station.tags.replace(/,/g, ', ')),
            elCreateText('td', {}, station.country)
        ]);
        setData(row, 'uri', station.url);
        setData(row, 'name', station.name);
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
