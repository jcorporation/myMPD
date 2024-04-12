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
 * @param {Function} [createCardFooterCallback] callback to create the footer
 * @returns {void}
 */
function updateGrid(obj, list, perCardCallback, createCardFooterCallback) {
    const grid = elGetById(list + 'List');
    let cols = grid.querySelectorAll('.col');
    for (let i = 0; i < obj.result.returnedEntities; i++) {
        const card = elCreateEmpty('div', {"class": ["card", "card-grid", "clickable"]});
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
                elCreateEmpty('div', {"class": ["card-body", "cover-loading", "cover-grid", "d-flex"]})
            );
            setData(card, 'cssImageUrl', obj.result.data[i].Thumbnail);
            if (userAgentData.hasIO === true) {
                const observer = new IntersectionObserver(setGridImage, {root: null, rootMargin: '0px'});
                observer.observe(card);
            }
            else {
                card.firstChild.style.backgroundImage = obj.result.data[i].Thumbnail;
            }
            addGridQuickButtons(card.firstChild);
        }
        const footer = elCreateEmpty('div', {"class": ["card-footer", "card-footer-grid", "p-2"]});
        if (createCardFooterCallback !== undefined &&
            typeof(createCardFooterCallback) === 'function')
        {
            //custom footer content
            createCardFooterCallback(footer, obj.result.data[i], obj.result);
        }
        else {
            //default footer content
            gridFooter(footer, obj.result.data[i], list);
        }
        card.appendChild(footer);
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
 * Populates the grid footer
 * @param {Element} footer grid footer to populate
 * @param {object} data data to populate
 * @param {string} list view name
 * @returns {void}
 */
function gridFooter(footer, data, list) {
    footer.appendChild(
        pEl.gridSelectBtn.cloneNode(true)
    );
    let i = 0;
    for (const tag of settings['view' + list].fields) {
        if (tag === 'Thumbnail') {
            continue;
        }
        footer.appendChild(
            elCreateNode((i === 0 ? 'span' : 'small'), {"class": ["d-block"], "data-col": settings['view' + list].fields[i]},
                printValue(tag, data[tag])
            )
        );
        i++;
    }
}

/**
 * Adds the quick play button to a grid element
 * @param {ChildNode} parentEl the containing element
 * @returns {void}
 */
function addGridQuickButtons(parentEl) {
    switch(app.id) {
        case 'BrowsePlaylistDetail':
        case 'QueueJukeboxSong':
        case 'QueueJukeboxAlbum':
            parentEl.appendChild(
                pEl.gridPlayBtn.cloneNode(true)
            );
            parentEl.appendChild(
                pEl.gridRemoveBtn.cloneNode(true)
            );
            break;
        case 'QueueCurrent':
            parentEl.appendChild(
                pEl.gridRemoveBtn.cloneNode(true)
            );
            break;
        case 'BrowseDatabaseTagList':
            // no default buttons
            break;
        default:
            parentEl.appendChild(
                pEl.gridPlayBtn.cloneNode(true)
            );
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
            const body = change.target.querySelector('.card-body');
            if (body) {
                body.style.backgroundImage = getData(change.target, 'cssImageUrl');
            }
        }
    });
}
