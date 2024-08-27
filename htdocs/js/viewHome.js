"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewHome_js */

/**
 * Handles home
 * @returns {void}
 */
function handleHome() {
    sendAPI("MYMPD_API_HOME_ICON_LIST", {}, parseHomeIcons, false);
}

/**
 * Moves a home icon
 * @param {number} oldPos Old icon pos
 * @param {number} newPos New icon pos
 * @returns {void}
 */
function homeMoveIcon(oldPos, newPos) {
    sendAPI("MYMPD_API_HOME_ICON_MOVE", {
        "from": oldPos,
        "to": newPos
    }, null, false);
}

/**
 * Click event handler for home icons
 * @param {MouseEvent} event click event
 * @param {HTMLElement} target calculated target
 * @returns {void}
 */
function viewHomeClickHandler(event, target) {
    if (event.target.classList.contains('card-body')) {
        showContextMenu(event);
        return;
    }
    const href = getData(target, 'href');
    if (href !== undefined) {
        parseCmd(event, href);
    }
}

/**
 * Returns the friendly type of the home icon
 * @param {string} cmd the command
 * @param {string} action action of the command
 * @returns {string} friendly type
 */
function getHomeIconType(cmd, action) {
    switch(cmd) {
        case 'appGoto':
        case 'execScriptFromOptions':
        case 'openExternalLink':
        case 'openModal':
            return typeFriendly[cmd];
        default:
            return typeFriendly[action];
    }
}

/**
 * Parses the MYMPD_API_HOME_ICON_LIST response
 * @param {object} obj jsonrpc response object
 * @returns {void}
 */
function parseHomeIcons(obj) {
    const cardContainer = elGetById('HomeList');
    unsetUpdateView(cardContainer);
    const cols = cardContainer.querySelectorAll('.col');
    if (cols.length === 0) {
        // remove warning messages
        elClear(cardContainer);
    }
    if (obj.result && obj.result.returnedEntities === 0) {
        elClear(cardContainer);
        const div = elCreateNodes('div', {"class": ["px-3", "py-1"]}, [
            elCreateTextTn('h3', {}, 'Homescreen'),
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

    if (checkResult(obj, cardContainer, undefined) === false) {
        return;
    }

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const col = obj.result.data[i].type === 'widget'
            ? createHomeWidget(obj.result.data[i], i)
            : createHomeIcon(obj.result.data[i], i);
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            cardContainer.append(col);
        }
        if (obj.result.data[i].type === 'widget') {
            updateHomeWidget(col.firstElementChild);
        }
    }
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }
    setScrollViewHeight(cardContainer);
}

/**
 * Updates the widget by calling the script
 * @param {HTMLElement | ChildNode} card Widget to populate
 * @returns {void}
 */
function updateHomeWidget(card) {
    card.querySelector('.card-title a').textContent = 'autorenew';
    const data = getData(card, 'data');
    const query = [];
    for (const key in data.arguments) {
        query.push(myEncodeURIComponent(key) + '=' + myEncodeURIComponent(data.arguments[key]));
    }
    httpGet(getMyMPDuri('http') + subdir + '/script/' +  localSettings.partition + '/' + data.script + '?' + query.join('&'),
        function(response) {
            setTimeout(function() {
                card.querySelector('.card-title a').textContent = 'refresh';
            }, 200);
            const body = card.querySelector('.card-body');
            elClear(body);
            if (response === null) {
                body.appendChild(elCreateTextTn('div', {'class': ['alert', 'alert-danger', 'm-3']}, 'Error executing script'));
                return;
            }
            const parser = new DOMParser();
            const html = parser.parseFromString(response, "text/html");
            body.appendChild(... html.body.childNodes);
            body.addEventListener('click', function(event) {
                event.preventDefault();
                event.stopPropagation();
                const href = getData(event.target, 'href');
                if (href === undefined) {
                    return;
                }
                parseCmd(event, JSON.parse(href));
            }, false);
        },
        false);
}

/**
 * Creates a home widget
 * @param {object} data Widegt data
 * @param {number} pos Widget position
 * @returns {HTMLElement} Created widget wrapped in col
 */
function createHomeWidget(data, pos) {
    const col = elCreateEmpty('div', {"class": ["col", "px-0", "flex-grow-0", "float-start"]});
    const card = elCreateNodes('div', {"data-contextmenu": "homeWidget", "class": ["card", "home-widgets", "bg-secondary", "rounded-2", "home-widget-" + data.size], "draggable": "true"},
        [
            elCreateNodes('div', {'class': ['card-title', 'py-2', 'px-3', 'mb-0']}, [
                document.createTextNode(data.name),
                elCreateText('a', {'href':'#', 'data-title-phrase': 'Reload', 'title': tn('Reload'), 'data-action': 'refreshWidget', 'class': ['mi', 'float-end']}, 'refresh')
            ]),
            elCreateEmpty('div', {'class': ['card-body', 'overflow-scroll', 'p-0', 'bg-dark', 'rounded-bottom']})
        ]
    );
    setData(card, 'type', 'widget');
    setData(card, 'pos', pos);
    setData(card, 'data', data);
    col.appendChild(card);
    return col;
}

/**
 * Creates a home icon
 * @param {object} data Icon data
 * @param {number} pos Icon position
 * @returns {HTMLElement} Created icon wrapped in col
 */
function createHomeIcon(data, pos) {
    const homeType = getHomeIconType(data.cmd, data.options[0]);
    const actionType = friendlyActions[data.cmd];

    if (data.cmd !== 'appGoto') {
        const opt0 = data.options[0];
        const opt1 = [];
        // convert array to [opt0, [opt1,...]] and parse 
        if (data.options[1] !== undefined) {
            for (let j = 1; j < data.options.length; j++) {
                opt1.push(convertType(data.options[j]));
            }
        }
        data.options = [opt0, opt1];
    }

    const col = elCreateEmpty('div', {"class": ["col", "px-0", "flex-grow-0", "float-start"]});
    const card = elCreateEmpty('div', {"data-contextmenu": "homeIcon", "class": ["card", "home-icons"], "draggable": "true",
        "title": tn(homeType) + ':' + smallSpace + data.name +
        '\n' + tn(actionType)});
    //decode json options
    for (let j = 0, k = data.options.length; j < k; j++) {
        if (data.options[j].indexOf('{"') === 0 ||
            data.options[j].indexOf('["') === 0)
        {
            data.options[j] = JSON.parse(data.options[j]);
        }
    }

    setData(card, 'type', 'icon');
    setData(card, 'href', {"cmd": data.cmd, "options": data.options});
    setData(card, 'pos', pos);
    const cardTitle = elCreateText('div', {"class": ["card-title", "mi", "rounded", "clickable"]}, data.ligature);
    if (data.image !== '') {
        cardTitle.style.backgroundImage = getCssImageUri(data.image);
    }
    if (data.bgcolor !== '') {
        cardTitle.style.backgroundColor = data.bgcolor;
    }
    if (data.color !== '' &&
        data.color !== undefined)
    {
        cardTitle.style.color = data.color;
    }
    card.appendChild(cardTitle);
    card.appendChild(
        elCreateText('div', {"class": ["card-body", "card-body-grid", "p-2", "clickable"]}, data.name)
    );
    col.appendChild(card);
    return col;
}
