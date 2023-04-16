"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
        const contextMenuEl = document.getElementById('offcanvasContext');
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
    const contextMenuEl = document.getElementById('offcanvasContext');
    const contextMenu = BSN.Offcanvas.getInstance(contextMenuEl);

    switch (contextMenuType) {
        case 'columns':
            //column select in table header
            createContextMenuOffcanvas(target, contextMenuEl, 'Columns', createMenuColumns, undefined);
            break;
        case 'disc':
            //disc actions in album details view
            createContextMenuOffcanvas(target, contextMenuEl, 'Disc', addMenuItemsDiscActions, undefined);
            break;
        case 'home':
            //home card actions
            createContextMenuOffcanvas(target, contextMenuEl, '', createMenuHome, createMenuHomeSecondary);
            break;
        case 'webradio':
            //webradio favorite actions
            createContextMenuOffcanvas(target, contextMenuEl, 'Webradio', addMenuItemsWebradioFavoritesActions, undefined);
            break;
        case 'album':
            //album action in album list
            createContextMenuOffcanvas(target, contextMenuEl, 'Album', addMenuItemsAlbumActions, undefined);
            break;
        default:
            createContextMenuOffcanvas(target, contextMenuEl, '', createMenuLists, createMenuListsSecondary);
    }
    contextMenu.show();
}

/**
 * Populates the offcanvas contextmenu
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {string} title contextmenu title 
 * @returns {void}
 */
function createContextMenuOffcanvasInit(contextMenuEl, title) {
    const contextMenuHeader = contextMenuEl.querySelector('.offcanvas-header');
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    contextMenuBody.removeAttribute('id');
    contextMenuBody.classList.remove('px-3');
    elClear(contextMenuHeader);
    elClear(contextMenuBody);
    //title
    contextMenuHeader.appendChild(
        elCreateText('h4', {"class": ["offcanvas-title", "ms-3"]}, title)
    );
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
 * @param {string} title contextmenu title
 * @param {Function} contentCallback1 first callback to create the content
 * @param {Function} contentCallback2 second callback to create the content
 * @returns {void}
 */
function createContextMenuOffcanvas(target, contextMenuEl, title, contentCallback1, contentCallback2) {
    createContextMenuOffcanvasInit(contextMenuEl, tn(title));
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
