"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
 * Initializes the home feature elements
 * @returns {void}
 */
function initViewHome() {
    elGetById('HomeList').addEventListener('click', function(event) {
        const target = gridClickHandler(event);
        if (target !== null) {
            const href = getData(target.parentNode, 'href');
            if (href !== undefined) {
               parseCmd(event, href);
            }
        }
    }, false);

    dragAndDropHome();
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

    if (obj.error !== undefined) {
        elReplaceChild(cardContainer,
            elCreateTextTn('div', {"class": ["ms-3", "mb-3", "not-clickable", "alert", "alert-danger"]}, obj.error.message, obj.error.data)
        );
        return;
    }
    if (cols.length === 0) {
        elClear(cardContainer);
    }
    if (obj.result.returnedEntities === 0) {
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
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const homeType = getHomeIconType(obj.result.data[i].cmd, obj.result.data[i].options[0]);
        const actionType = friendlyActions[obj.result.data[i].cmd];

        if (obj.result.data[i].cmd !== 'appGoto') {
            const opt0 = obj.result.data[i].options[0];
            const opt1 = [];
            // convert array to [opt0, [opt1,...]] and parse 
            if (obj.result.data[i].options[1] !== undefined) {
                for (let j = 1; j < obj.result.data[i].options.length; j++) {
                    opt1.push(convertType(obj.result.data[i].options[j]));
                }
            }
            obj.result.data[i].options = [opt0, opt1];
        }

        const col = elCreateEmpty('div', {"class": ["col", "px-0", "flex-grow-0"]});
        const card = elCreateEmpty('div', {"data-contextmenu": "home", "class": ["card", "home-icons"], "draggable": "true",
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
            cardBody.style.backgroundImage = getCssImageUri(obj.result.data[i].image);
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
        card.appendChild(
            elCreateText('div', {"class": ["card-footer", "card-footer-grid", "p-2", "clickable"]}, obj.result.data[i].name)
        );
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
    setScrollViewHeight(cardContainer);
}

/**
 * Shows the dragover tip
 * @param {EventTarget} from from element
 * @param {EventTarget} to to element
 * @returns {void}
 */
function showDropoverIcon(from, to) {
    const fromPos = getData(from, 'pos');
    const toPos = getData(to, 'pos');
    if (toPos > fromPos) {
        to.classList.add('dragover-icon-right');
    }
    else {
        to.classList.add('dragover-icon-left');
    }
    to.classList.add('dragover-icon');
}

/**
 * Hides the dragover tip
 * @param {EventTarget} el element
 * @returns {void}
 */
function hideDropoverIcon(el) {
    el.classList.remove('dragover-icon-left', 'dragover-icon-right');
}

/**
 * Drag and drop event handler
 * @returns {void}
 */
function dragAndDropHome() {
    const HomeList = elGetById('HomeList');

    HomeList.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('home-icons')) {
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragEl = event.target;
        }
    }, false);

    HomeList.addEventListener('dragenter', function(event) {
        if (dragEl !== undefined &&
            event.target.classList.contains('home-icons'))
        {
            showDropoverIcon(dragEl, event.target);
        }
    }, false);

    HomeList.addEventListener('dragleave', function(event) {
        if (dragEl !== undefined &&
            event.target.classList.contains('home-icons'))
        {
            hideDropoverIcon(event.target);
        }
    }, false);

    HomeList.addEventListener('dragover', function(event) {
        // prevent default to allow drop
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
    }, false);

    HomeList.addEventListener('drop', function(event) {
        event.preventDefault();
        event.stopPropagation();
        
        const target = event.target.classList.contains('card-body')
            ? event.target.parentNode
            : event.target;
        if (target.classList.contains('home-icons')) {
            hideDropoverIcon(target);
            const to = getData(target, 'pos');
            const from = getData(dragEl, 'pos');
            if (isNaN(to) === false &&
                isNaN(from) === false &&
                from !== to)
            {
                sendAPI("MYMPD_API_HOME_ICON_MOVE", {"from": from, "to": to}, null, false);
            }
        }
    }, false);

    HomeList.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
}

/**
 * Executes the home icon action
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function executeHomeIcon(pos) {
    const el = elGetById('HomeList').children[pos].firstChild;
    parseCmd(null, getData(el, 'href'));
}
