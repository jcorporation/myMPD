"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module viewsList_js */

/**
 * Updates the list from the jsonrpc response
 * @param {object} obj jsonrpc response
 * @param {string} list list name to populate
 * @param {Function} [perCardCallback] callback per card
 * @param {Function} [createCardBodyCallback] callback to create the footer
 * @param {Function} [createCardActionsCallback] callback to create the footer
 * @returns {void}
 */
function updateList(obj, list, perCardCallback, createCardBodyCallback, createCardActionsCallback) {
    const grid = elGetById(list + 'List');
    let cols = grid.querySelectorAll('.list-group-item');

    const footer = elCreateEmpty('div', {"class": ["list-actions", "col", "text-end"]});
    addActionLinks(footer);

    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const card = elCreateEmpty('div', {"class": ["list-group-item", "list-group-item-action", "clickable"]});
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
            row.appendChild(
                elCreateEmpty('div', {"class": ["col", "list-image"]})
            );
            setData(card, 'cssImageUrl', obj.result.data[i].Thumbnail);
            if (userAgentData.hasIO === true) {
                const observer = new IntersectionObserver(setListImage, {root: null, rootMargin: '0px'});
                observer.observe(card);
            }
            else {
                card.firstChild.style.backgroundImage = obj.result.data[i].Thumbnail;
            }
        }
        const body = elCreateEmpty('div', {"class": ["col", "ps-4"]});
        if (createCardBodyCallback !== undefined &&
            typeof(createCardBodyCallback) === 'function')
        {
            //custom body content
            createCardBodyCallback(body, obj.result.data[i], obj.result);
        }
        else {
            //default body content
            listBody(body, obj.result.data[i], list);
        }
        row.appendChild(body);
        if (createCardActionsCallback !== undefined &&
            typeof(createCardActionsCallback) === 'function')
        {
            //custom footer content
            const customFooter = elCreateEmpty('div', {"class": ["list-actions", "col", "text-end"]});
            createCardActionsCallback(customFooter, obj.result.data[i], obj.result);
            row.appendChild(customFooter);
        }
        else {
            //default footer content
            row.appendChild(footer.cloneNode(true));
        }
        card.appendChild(row);
        if (i < cols.length) {
            cols[i].replaceWith(card);
        }
        else {
            grid.append(card);
        }
    }
    //remove obsolete cards
    cols = grid.querySelectorAll('.list-group-item');
    for (let i = cols.length - 1; i >= obj.result.returnedEntities; i--) {
        cols[i].remove();
    }

    unsetUpdateView(grid);
    setPagination(obj.result.totalEntities, obj.result.returnedEntities);
    setScrollViewHeight(grid);
    scrollToPosY(grid.parentNode, app.current.scrollPos);
}

/**
 * Populates the list body
 * @param {Element} body grid footer to populate
 * @param {object} data data to populate
 * @param {string} list view name
 * @returns {void}
 */
function listBody(body, data, list) {
    let i = 0;
    for (const tag of settings['view' + list].fields) {
        if (tag === 'Thumbnail') {
            i++;
            continue;
        }
        const value = printValue(tag, data[tag]);
        if (i === 0) {
            body.appendChild(
                elCreateNode('h5', {"class": ["d-block"], "data-col": settings['view' + list].fields[i]},
                    value
                )
            );
        }
        else {
            body.appendChild(
                elCreateNodes('span', {"class": ["d-block"], "data-col": settings['view' + list].fields[i]}, [
                    elCreateTextTn('small', {}, settings['view' + list].fields[i]),
                    elCreateText('small', {'class': ['me-2']}, ':'),
                    value
                ])
            );
        }
        i++;
    }
}

/**
 * Callback function for intersection observer to lazy load cover images
 * @param {object} changes IntersectionObserverEntry objects
 * @param {object} observer IntersectionObserver
 * @returns {void}
 */
function setListImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const body = change.target.querySelector('.list-image');
            if (body) {
                body.style.backgroundImage = getData(change.target, 'cssImageUrl');
            }
        }
    });
}
