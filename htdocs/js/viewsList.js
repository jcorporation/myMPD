"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewsList_js */

/**
 * Initializes a list for drag and drop of list-group-items
 * @param {string} listId table id
 * @returns {void}
 */
function dragAndDropList(listId) {
    const listBody = document.querySelector('#' + listId);
    listBody.addEventListener('dragstart', function(event) {
        if (event.target.classList.contains('list-group-item')) {
            event.target.classList.add('opacity05');
            // @ts-ignore
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            dragEl = event.target;
        }
    }, false);

    listBody.addEventListener('dragenter', function(event) {
        const target = event.target.classList.contains('list-group-item')
            ? event.target
            : event.target.closest('.list-group-item');
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover');
        }
    }, false);

    listBody.addEventListener('dragleave', function(event) {
        const target = event.target.classList.contains('list-group-item')
            ? event.target
            : event.target.closest('.list-group-item');
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.remove('dragover');
        }
    }, false);

    listBody.addEventListener('dragover', function(event) {
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
        const target = event.target.classList.contains('list-group-item')
            ? event.target
            : event.target.closest('.list-group-item');
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover');
        }
    }, false);

    listBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined ||
            dragEl.classList.contains('list-group-item') === false)
        {
            return;
        }
        const target = event.target.classList.contains('list-group-item')
            ? event.target
            : event.target.closest('.list-group-item');
        target.classList.remove('dragover');
        const newPos = getData(target, 'pos');
        const oldPos = getData(dragEl, 'pos');
        if (oldPos === newPos) {
            return;
        }
        // set dragged element uri to undefined to force table row replacement
        setData(dragEl, 'uri', undefined);
        elHide(dragEl);
        // apply new order
        setUpdateViewId(listId);
        switch(app.id) {
            case 'QueueCurrent': {
                queueMoveSong(oldPos, newPos);
                break;
            }
            case 'BrowsePlaylistDetail': {
                currentPlaylistMoveSong(oldPos, newPos);
                break;
            }
            // No Default
        }
    }, false);

    listBody.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
}

/**
 * Replaces a list item and tries to keep the selection state
 * @param {boolean} mode the selection mode
 * @param {HTMLElement} item item to replace
 * @param {HTMLElement} el replacement col
 * @returns {void}
 */
function replaceListItem(mode, item, el) {
    if (getData(item, 'uri') === getData(el, 'uri')) {
        if (mode === true &&
            item.firstElementChild.lastElementChild.lastElementChild.textContent === ligatures.checked)
        {
            el.firstElementChild.lastElementChild.lastElementChild.textContent = ligatures.checked;
            el.classList.add('selected');
        }
        if (item.classList.contains('queue-playing')) {
            el.classList.add('queue-playing');
            el.style.background = item.style.background;
        }
    }
    item.replaceWith(el);
}

/**
 * Returns the text and tag name for the badge
 * @param {object} data Song data
 * @returns {object} [Tag value, Tag name]
 */
function getBadgeText(data) {
    // A badge is only displayed if a thumbnail is shown
    if (settings['view' + app.id].fields.includes('Thumbnail') === false ||
        data.Thumbnail === undefined)
    {
        return [null, null];
    }
    if (settings['view' + app.id].fields.includes('Pos')) {
        return [data.Pos + 1, 'Pos'];
    }
    if (settings['view' + app.id].fields.includes('Track')) {
        return [data.Track, 'Track'];
    }
    return [null, null];
}

/**
 * Updates the list from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list list name to populate
 * @param {Function} [perCardCallback] callback per card
 * @param {Function} [createCardBodyCallback] callback to create the body
 * @param {Function} [createCardActionsCallback] callback to create the footer
 * @returns {void}
 */
function updateList(obj, list, perCardCallback, createCardBodyCallback, createCardActionsCallback) {
    const grid = elGetById(list + 'List');
    let cols = grid.querySelectorAll('.list-group-item');
    const mode = grid.getAttribute('data-mode') === 'select'
        ? true
        : false;

    const footer = elCreateEmpty('div', {"class": ["list-actions", "col", "col-auto", "text-end", "px-0"]});
    addActionLinks(footer);

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const card = elCreateEmpty('div', {"class": ["list-group-item", "list-group-item-action", "clickable", "viewListItem"]});
        const row = elCreateEmpty('div', {'class': ['row', 'p-1']});
        if (perCardCallback !== undefined &&
            typeof(perCardCallback) === 'function')
        {
            perCardCallback(card, obj.result.data[i], obj.result);
        }
        setEntryData(card, obj.result.data[i]);
        if (settings['view' + app.id].fields.includes('Thumbnail') &&
            obj.result.data[i].Thumbnail !== undefined)
        {
            const badgeText = getBadgeText(obj.result.data[i])[0];
            const els = [];
            els.push(elCreateEmpty('img', {"loading": "lazy", "src": obj.result.data[i].Thumbnail}));
            if (badgeText !== null) {
                els.push(elCreateText('span', {"class": ["badge", "text-bg-secondary", "listThumbnailBadge"], 'data-col': 'Pos'}, badgeText));
            }
            row.appendChild(
                elCreateNodes('div', {"class": ["col", "list-image"]}, els)
            );
        }
        const body = elCreateEmpty('div', {"class": ["col", "ps-3"]});
        if (createCardBodyCallback !== undefined &&
            typeof(createCardBodyCallback) === 'function')
        {
            //custom body content
            createCardBodyCallback(body, obj.result.data[i], obj.result);
        }
        else {
            //default body content
            createListBody(body, obj.result.data[i], list);
        }
        row.appendChild(body);
        if (createCardActionsCallback !== undefined &&
            typeof(createCardActionsCallback) === 'function')
        {
            //custom footer content
            const customFooter = elCreateEmpty('div', {"class": ["list-actions", "col", "col-auto", "text-end", "px-0"]});
            createCardActionsCallback(customFooter, obj.result.data[i], obj.result);
            row.appendChild(customFooter);
        }
        else {
            //default footer content
            row.appendChild(footer.cloneNode(true));
        }
        card.appendChild(row);
        if (features.featPagination === true ||
            obj.result.offset === 0)
        {
            if (i < cols.length) {
                replaceListItem(mode, cols[i], card);
            }
            else {
                grid.append(card);
            }
        }
        else {
            grid.append(card);
        }
    }
    //remove obsolete list items
    if (features.featPagination === true ||
        obj.result.offset === 0)
    {
        cols = grid.querySelectorAll('.list-group-item');
        for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
            cols[i].remove();
        }
    }

    unsetUpdateView(grid);
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(grid);
    if (features.featPagination === true ||
        obj.result.offset === 0)
    {
        scrollToPosY(grid.parentNode, app.current.scrollPos);
    }
}

/**
 * Populates the list body
 * @param {Element} body list element body to populate
 * @param {object} data data to populate
 * @param {string} list view name
 * @returns {void}
 */
function createListBody(body, data, list) {
    let i = 0;
    for (const tag of settings['view' + list].fields) {
        if (tag === 'Thumbnail' ||
            getBadgeText(data)[1] === tag)
        {
            continue;
        }
        if (i === 0) {
            body.appendChild(
                elCreateNode('h5', {"class": ["d-block"], "data-col": tag},
                    printValue(tag, data[tag], data)
                )
            );
        }
        else if (isEmptyTag(data[tag]) === false ||
                 tag === 'userDefinedSticker')
        {
            if (uiSmallWidthTagRows() === true) {
                body.appendChild(
                    elCreateNode('div', {"class": ["row"]},
                        elCreateNodes('div', {"class": ["col"]},[
                            elCreateTextTn('small', {}, tag),
                            elCreateEmpty('br', {}),
                            elCreateNode('span', {"data-col": tag, "class": ["col-8"]},
                                printValue(tag, data[tag], data)
                            )
                        ])
                    )
                );
            }
            else {
                body.appendChild(
                    elCreateNodes('div', {"class": ["row"]}, [
                        elCreateTextTn('small', {"class": ["col-4"]}, tag),
                        elCreateNode('span', {"data-col": tag, "class": ["col-8"]},
                            printValue(tag, data[tag], data)
                        )
                    ])
                );
            }
        }
        i++;
    }
}
