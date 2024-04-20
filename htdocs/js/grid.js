"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module grid_js */

/**
 * Updates the table from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list row name to populate
 * @param {Function} [perCardCallback] callback per card
 * @param {Function} [createCardBodyCallback] callback to create the footer
 * @param {Function} [createCardActionsCallback] callback to create the footer
 * @returns {void}
 */
function updateGrid(obj, list, perCardCallback, createCardBodyCallback, createCardActionsCallback) {
    const grid = elGetById(list + 'List');
    let cols = grid.querySelectorAll('.col');

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
                elCreateEmpty('div', {"class": ["card-title", "cover-loading", "cover-grid", "d-flex"]})
            );
            setData(card, 'cssImageUrl', obj.result.data[i].Thumbnail);
            if (userAgentData.hasIO === true) {
                const observer = new IntersectionObserver(setGridImage, {root: null, rootMargin: '0px'});
                observer.observe(card);
            }
            else {
                card.firstChild.style.backgroundImage = obj.result.data[i].Thumbnail;
            }
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
            gridBody(body, obj.result.data[i], list);
        }
        card.appendChild(body);
        if (createCardActionsCallback !== undefined &&
            typeof(createCardActionsCallback) === 'function')
        {
            //custom footer content
            const customFooter = elCreateEmpty('div', {"class": ["card-footer", "card-footer-grid", "p-2"]});
            createCardActionsCallback(customFooter, obj.result.data[i], obj.result);
            card.appendChild(customFooter);
        }
        else {
            //default footer content
            card.appendChild(footer.cloneNode(true));
        }
        const col = elCreateNode('div', {"class": ["col", "px-0", "mb-2", "flex-grow-0"]}, card);
        if (i < cols.length) {
            cols[i].replaceWith(col);
        }
        else {
            grid.append(col);
        }
    }
    //remove obsolete cards
    cols = grid.querySelectorAll('.col');
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }

    unsetUpdateView(grid);
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(grid);
    scrollToPosY(grid.parentNode, app.current.scrollPos);
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
 * @param {Element} body grid footer to populate
 * @param {object} data data to populate
 * @param {string} list view name
 * @returns {void}
 */
function gridBody(body, data, list) {
    let i = 0;
    for (const tag of settings['view' + list].fields) {
        if (tag === 'Thumbnail') {
            continue;
        }
        const value = printValue(tag, data[tag]);
        const title = tag === 'Type'
            ? getTypeTitle(value.textContent)
            : value.textContent;
        body.appendChild(
            elCreateNode((i === 0 ? 'span' : 'small'), {"class": ["d-block"], "data-col": settings['view' + list].fields[i], "title": title},
                value
            )
        );
        i++;
    }
}

/**
 * Callback function for intersection observer to lazy load cover images
 * @param {object} changes IntersectionObserverEntry objects
 * @param {object} observer IntersectionObserver
 * @returns {void}
 */
function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const body = change.target.querySelector('.card-title');
            if (body) {
                body.style.backgroundImage = getData(change.target, 'cssImageUrl');
            }
        }
    });
}
