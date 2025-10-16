"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewsGrid_js */

/**
 * Initializes a grid for drag and drop
 * @param {string} gridId grid id
 * @returns {void}
 */
function dragAndDropGrid(gridId) {
    const gridBody = elGetById(gridId);

    gridBody.addEventListener('dragstart', function(event) {
        const target = event.target.classList.contains('col')
            ? event.target
            : event.target.closest('.col');
        if (target === null) {
            return false;
        }
        event.target.classList.add('opacity05');
        // @ts-ignore
        event.dataTransfer.setDragImage(event.target, 0, 0);
        event.dataTransfer.effectAllowed = 'move';
        dragEl = target;
    }, false);

    gridBody.addEventListener('dragenter', function(event) {
        const target = event.target.classList.contains('col')
            ? event.target
            : event.target.closest('.col');
        if (target === null) {
            return false;
        }
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover-left');
        }
    }, false);

    gridBody.addEventListener('dragleave', function(event) {
        const target = event.target.classList.contains('col')
            ? event.target
            : event.target.closest('.col');
        if (target === null) {
            return false;
        }
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.remove('dragover-left');
        }
    }, false);

    gridBody.addEventListener('dragover', function(event) {
        // prevent default to allow drop
        event.preventDefault();
        event.dataTransfer.dropEffect = 'move';
        const target = event.target.classList.contains('col')
            ? event.target
            : event.target.closest('.col');
        if (target === null) {
            return false;
        }
        if (dragEl !== undefined &&
            dragEl.nodeName === target.nodeName)
        {
            target.classList.add('dragover-left');
        }
    }, false);

    gridBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (dragEl === undefined ||
            dragEl.classList.contains('col') === false)
        {
            return;
        }
        const target = event.target.classList.contains('col')
            ? event.target
            : event.target.closest('.col');
        if (target === null) {
            return false;
        }
        target.classList.remove('dragover');
        const newPos = getData(target.firstElementChild, 'pos');
        const oldPos = getData(dragEl.firstElementChild, 'pos');
        if (oldPos === newPos) {
            return;
        }
        // set dragged element uri to undefined to force table row replacement
        setData(dragEl, 'uri', undefined);
        elHide(dragEl);
        // apply new order
        setUpdateViewId(gridId);
        switch(app.id) {
            case 'Home': {
                homeMoveIcon(oldPos, newPos);
                break;
            }
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

    gridBody.addEventListener('dragend', function() {
        dragEl.classList.remove('opacity05');
        dragEl = undefined;
    }, false);
}

/**
 * Replaces a grid col and tries to keep the selection state
 * @param {boolean} inSelectMode the selection mode
 * @param {HTMLElement} col col to replace
 * @param {HTMLElement} el replacement col
 * @returns {void}
 */
function replaceGridCol(inSelectMode, col, el) {
    if (inSelectMode === true) {
        const colCard = col.firstElementChild;
        const elCard = el.firstElementChild;

        const prevCol = col.previousElementSibling;
        const prevUri = prevCol !== null
            ? getData(prevCol.firstElementChild, 'uri')
            : null;

        const nextCol = col.nextElementSibling;
        const nextUri = nextCol !== null
            ? getData(nextCol.firstElementChild, 'uri')
            : null;

        const newUri = getData(elCard, 'uri');
        if (getData(colCard, 'uri') === newUri) {
            copyGridSelection(colCard, elCard);
        }
        else if (nextUri === newUri) {
            copyGridSelection(nextCol.firstElementChild, elCard);
        }
        else if (prevUri === newUri) {
            copyGridSelection(prevCol.firstElementChild, elCard);
        }
    }
    col.replaceWith(el);
}

/**
 * Copy the selection state
 * @param {HTMLElement} colCard card to replace
 * @param {HTMLElement} elCard replacement card
 * @returns {void}
 */
function copyGridSelection(colCard, elCard) {
    if (colCard.lastElementChild.lastElementChild.textContent === ligatures.checked) {
        elCard.lastElementChild.lastElementChild.textContent = ligatures.checked;
        elCard.classList.add('selected');
    }
}

/**
 * Updates the grid from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list grid name to populate
 * @param {Function} [perCardCallback] callback per card
 * @param {Function} [createCardBodyCallback] callback to create the footer
 * @param {Function} [createCardActionsCallback] callback to create the footer
 * @returns {void}
 */
function updateGrid(obj, list, perCardCallback, createCardBodyCallback, createCardActionsCallback) {
    const grid = elGetById(list + 'List');
    let cols = grid.querySelectorAll('.col');
    const mode = grid.getAttribute('data-mode') === 'select'
        ? true
        : false;

    const footer = elCreateEmpty('div', {"class": ["card-footer", "card-footer-grid", "p-0", "d-flex", "justify-content-center"]});
    addActionLinks(footer);

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const card = elCreateEmpty('div', {"class": ["card", "card-grid", "clickable", "h-100"]});
        if (perCardCallback !== undefined &&
            typeof(perCardCallback) === 'function')
        {
            perCardCallback(card, obj.result.data[i], obj.result);
        }
        setEntryData(card, obj.result.data[i]);
        if (settings['view' + app.id].fields.includes('Thumbnail') &&
            obj.result.data[i].Thumbnail !== undefined)
        {
            card.appendChild(
                elCreateNode('div', {"class": ["card-title", "cover-grid", "d-flex"]},
                    elCreateEmpty('img', {"width": settings.webuiSettings.gridSize, "height": settings.webuiSettings.gridSize,
                        "loading": "lazy", "src": obj.result.data[i].Thumbnail})
                )
            );
        }
        const body = elCreateEmpty('div', {"class": ["card-body", "card-body-grid", "p-2"]});
        if (createCardBodyCallback !== undefined &&
            typeof(createCardBodyCallback) === 'function')
        {
            //custom body content
            createCardBodyCallback(body, obj.result.data[i], obj.result);
        }
        else {
            //default body content
            createGridBody(body, obj.result.data[i], list);
        }
        card.appendChild(body);
        if (createCardActionsCallback !== undefined &&
            typeof(createCardActionsCallback) === 'function')
        {
            //custom footer content
            const customFooter = elCreateEmpty('div', {"class": ["card-footer", "card-footer-grid", "p-0", "d-flex", "justify-content-center"]});
            createCardActionsCallback(customFooter, obj.result.data[i], obj.result);
            card.appendChild(customFooter);
        }
        else {
            //default footer content
            card.appendChild(footer.cloneNode(true));
        }
        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);
        if (features.featPagination === true ||
            obj.result.offset === 0)
        {
            if (i < cols.length) {
                replaceGridCol(mode, cols[i], col);
            }
            else {
                grid.append(col);
            }
        }
        else {
            grid.append(col);
        }
    }
    //remove obsolete cards
    if (features.featPagination === true ||
        obj.result.offset === 0)
    {
        cols = grid.querySelectorAll('.col');
        for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
            cols[i].remove();
        }
    }

    unsetUpdateView(grid);
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(grid);
    restoreScrollPos(obj.result.offset, grid.parentNode, app.current.scrollPos);
}

/**
 * Returns the friendly names for type icons
 * @param {string} value tag value
 * @returns {string} friendly name
 */
function getTypeTitle(value) {
    switch(value) {
        case 'queue_music': return tn('Smart playlist');
        case 'list': return tn('Playlist');
        case 'folder_open': return tn('Folder');
        case 'music_note': return tn('Song');
        default: return value;
    }
}

/**
 * Populates the grid body
 * @param {Element} body grid body to populate
 * @param {object} data data to populate
 * @param {string} list view name
 * @returns {void}
 */
function createGridBody(body, data, list) {
    let i = 0;
    for (const tag of settings['view' + list].fields) {
        if (tag === 'Thumbnail') {
            i++;
            continue;
        }
        const value = printValue(tag, data[tag], data);
        const title = tag === 'Type'
            ? getTypeTitle(value.textContent)
            : value.textContent;
        body.appendChild(
            elCreateNode((i === 0 ? 'span' : 'small'), {"class": ["d-block"], "data-col": tag, "title": title},
                value
            )
        );
        i++;
    }
}
