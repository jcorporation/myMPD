"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module contextMenuOffcanvas_js */

/**
 * Initializes the offcanvas contextmenu
 * @returns {void}
 */
function initContextMenuOffcanvas() {
    document.querySelector('#offcanvasContext > .offcanvas-body').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            const cmd = getData(event.target, 'href');
            if (cmd) {
                parseCmd(event, cmd);
            }
        }
        event.preventDefault();
        const contextMenuEl = elGetById('offcanvasContext');
        BSN.Offcanvas.getInstance(contextMenuEl).hide();
    }, false);
}

/**
 * Shows the context menu offcanvas
 * @param {HTMLElement} target calculated target
 * @param {string} contextMenuType type of the context menu
 * @returns {void}
 */
function showContextMenuOffcanvas(target, contextMenuType) {
    const contextMenuEl = elGetById('offcanvasContext');
    const contextMenu = BSN.Offcanvas.getInstance(contextMenuEl);

    switch (contextMenuType) {
        case 'viewSettings':
            //column select in table header
            createContextMenuOffcanvas(target, contextMenuEl, contextMenuType, createMenuViewSettings, undefined);
            break;
        case 'disc':
            //disc actions in album details view
            createContextMenuOffcanvas(target, contextMenuEl, contextMenuType, addMenuItemsDiscActions, undefined);
            break;
        case 'home':
            //home card actions
            createContextMenuOffcanvas(target, contextMenuEl, '', createMenuHome, createMenuHomeSecondary);
            break;
        case 'webradio':
            //webradio favorite actions
            createContextMenuOffcanvas(target, contextMenuEl, contextMenuType, addMenuItemsWebradioFavoritesActions, undefined);
            break;
        case 'album':
            //album action in album list
            createContextMenuOffcanvas(target, contextMenuEl, contextMenuType, addMenuItemsAlbumActions, undefined);
            break;
        default:
            createContextMenuOffcanvas(target, contextMenuEl, '', createMenuLists, createMenuListsSecondary);
    }
    contextMenu.show();
}

/**
 * Populates the offcanvas contextmenu
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {string} type type of contextmenu
 * @returns {void}
 */
function createContextMenuOffcanvasInit(contextMenuEl, type) {
    const contextMenuHeader = contextMenuEl.querySelector('.offcanvas-header');
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    contextMenuBody.removeAttribute('id');
    contextMenuBody.classList.remove('px-3');
    elClear(contextMenuHeader);
    elClear(contextMenuBody);
    //title
    if (type !== '') {
        contextMenuHeader.appendChild(
            elCreateTextTn('h4', {"class": ["offcanvas-title", "ms-3", "offcanvas-title-" + type]}, typeFriendly[type])
        );
    }
    else {
        contextMenuHeader.appendChild(
            elCreateTextTn('h4', {"class": ["offcanvas-title", "ms-3"]}, '')
        );
    }
    contextMenuHeader.style.borderBottom = '';
    //close button
    const closeBtn = elCreateEmpty('button', {"class": ["btn-close", "me-3"]});
    closeBtn.addEventListener('click', function() {
        BSN.Offcanvas.getInstance(contextMenuEl).hide();
    }, false);
    contextMenuHeader.appendChild(closeBtn);
}

/**
 * Creates the contextmenu
 * @param {EventTarget} target triggering element
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {string} type contextmenu type
 * @param {Function} contentCallback1 first callback to create the content
 * @param {Function} contentCallback2 second callback to create the content
 * @returns {void}
 */
function createContextMenuOffcanvas(target, contextMenuEl, type, contentCallback1, contentCallback2) {
    createContextMenuOffcanvasInit(contextMenuEl, type);
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    const contextMenuTitle = contextMenuEl.querySelector('.offcanvas-title');
    contentCallback1(target, contextMenuTitle, contextMenuBody);
    if (contentCallback2 !== undefined &&
        typeof contentCallback2 === 'function')
    {
        contextMenuBody.appendChild(
            elCreateEmpty('div', {"class": ["dropdown-divider2"]})
        );
        const contextMenuSubtitle = elCreateEmpty('h4', {"class": ["offcanvas-title", "ms-3", "mt-4", "mb-2"]});
        contextMenuBody.appendChild(contextMenuSubtitle);
        contentCallback2(target, contextMenuSubtitle, contextMenuBody);
        //remove empty secondary menu
        let lastChild = contextMenuBody.lastElementChild;
        while (lastChild.nodeName !== 'A') {
            lastChild.remove();
            lastChild = contextMenuBody.lastElementChild;
        }
    }
}
