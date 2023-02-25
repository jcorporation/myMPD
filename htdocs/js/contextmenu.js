"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module contextmenu_js */

/**
 * Initializes the offcanvas contextmenu
 */
function initContextMenu() {
    document.querySelector('#offcanvasContext > .offcanvas-body').addEventListener('click', function(event) {
        if (event.target.nodeName === 'A') {
            const cmd = getData(event.target, 'href');
            if (cmd) {
                parseCmd(event, cmd);
            }
        }
        event.preventDefault();
    }, false);
}

/**
 * Shows the context menu sidebar
 * @param {Event} event triggering event
 * @returns {void}
 */
function showContextMenu(event) {
    event.preventDefault();
    event.stopPropagation();
    //get the dom node to attach the popover object
    let target = event.target.nodeName === 'SPAN'
               ? event.target.parentNode : event.target;
    if (target.nodeName === 'SMALL') {
        target = target.parentNode;
    }
    if (target.nodeName === 'TD' ||
        target.parentNode.classList.contains('card'))
    {
        target = target.parentNode;
    }

    const contextMenuEl = document.getElementById('offcanvasContext');
    const contextMenu = BSN.Offcanvas.getInstance(contextMenuEl);

    const contextMenuType = target.getAttribute('data-popover');
    logDebug('Create new contextmenu of type ' + contextMenuType);
    switch (contextMenuType) {
        case 'columns':
            //column select in table header
            createContextMenuColumns(contextMenuEl);
            break;
        case 'disc':
            //disc actions in album details view
            createContextMenuSimple(target, contextMenuEl, 'Disc', addMenuItemsDiscActions);
            break;
        case 'home':
            //home card actions
            createContextMenuTabs(target, contextMenuEl, createMenuHome, createMenuHomeSecondary);
            break;
        case 'webradio':
            //webradio favorite actions
            createContextMenuSimple(target, contextMenuEl, 'Webradio', addMenuItemsWebradioFavoritesActions);
            break;
        case 'album':
            //album action in album list
            createContextMenuSimple(target, contextMenuEl, 'Album', addMenuItemsAlbumActions);
            break;
        default:
            createContextMenuTabs(target, contextMenuEl, createMenuLists, createMenuListsSecondary);
    }
    contextMenu.show();
}

/**
 * Populates the offcanvas contextmenu
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {string} title contextmenu title 
 * @param {string} [template] the contextmenu body
 * @returns {void}
 */
function createContextMenuInit(contextMenuEl, title, template) {
    const contextMenuHeader = contextMenuEl.querySelector('.offcanvas-header');
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    contextMenuBody.removeAttribute('id');
    contextMenuBody.classList.remove('px-3');
    elClear(contextMenuHeader);
    elClear(contextMenuBody);
    if (template === 'tabs') {
        contextMenuHeader.appendChild(
            elCreateNodes('ul', {"class": ["nav", "nav-tabs", "px-2"]}, [
                elCreateNode('li', {"class": ["nav-item"]},
                    elCreateEmpty('a', {"class": ["nav-link", "active"], "href": "#"})
                ),
                elCreateNode('li', {"class": ["nav-item"]},
                    elCreateEmpty('a', {"class": ["nav-link"], "href": "#"})
                )
            ])
        );
        contextMenuBody.appendChild(
            elCreateNodes('div', {"class": ["tab-content"]}, [
                elCreateEmpty('div', {"class": ["tab-pane", "pt-2", "active", "show"], "id": "contextMenuTab0"}),
                elCreateEmpty('div', {"class": ["tab-pane", "pt-2"], "id": "contextMenuTab1"})
            ])
        );
        contextMenuHeader.style.borderBottom = 'var(--bs-border-width) solid var(--bs-border-color)';
    }
    else {
        contextMenuHeader.appendChild(
            elCreateText('h4', {"class": ["offcanvas-title", "ms-3"]}, title)
        );
        contextMenuHeader.style.borderBottom = '';
    }
    //close button
    const closeBtn = elCreateEmpty('button', {"class": ["btn-close", "me-3"]});
    closeBtn.addEventListener('click', function() {
        BSN.Offcanvas.getInstance(contextMenuEl).hide();
    }, false);
    contextMenuHeader.appendChild(closeBtn);
}

/**
 * Creates a contextmenu for the column select for tables
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @returns {void}
 */
function createContextMenuColumns(contextMenuEl) {
    createContextMenuInit(contextMenuEl, tn('Columns'));

    const menu = elCreateEmpty('form', {});
    setColsChecklist(app.id, menu);
    menu.addEventListener('click', function(eventClick) {
        if (eventClick.target.nodeName === 'BUTTON') {
            toggleBtnChk(eventClick.target, undefined);
            eventClick.preventDefault();
            eventClick.stopPropagation();
        }
    }, false);
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    contextMenuBody.classList.add('px-3');
    contextMenuBody.appendChild(menu);
    const applyEl = elCreateTextTn('button', {"class": ["btn", "btn-success", "btn-sm", "w-100", "mt-2"]}, 'Apply');
    contextMenuBody.appendChild(applyEl);
    applyEl.addEventListener('click', function(eventClick) {
        eventClick.preventDefault();
        saveCols(app.id);
    }, false);
    contextMenuBody.setAttribute('id', app.id + 'ColsDropdown');
}

/**
 * Creates a simple contextmenu
 * @param {EventTarget} el triggering element
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {string} title contextmenu title
 * @param {Function} contentCallback callback to create the content
 * @returns {void}
 */
function createContextMenuSimple(el, contextMenuEl, title, contentCallback) {
    createContextMenuInit(contextMenuEl, tn(title));
    const contextMenuBody = contextMenuEl.querySelector('.offcanvas-body');
    contentCallback(contextMenuBody, el);
}

/**
 * Creates a contextmenu with two tabs
 * @param {EventTarget} el triggering element
 * @param {HTMLElement} contextMenuEl contextmenu element
 * @param {Function} tab1Callback callback to create the content for the first tab
 * @param {Function} tab2Callback callback to create the content for the second tab
 * @returns {void}
 */
function createContextMenuTabs(el, contextMenuEl, tab1Callback, tab2Callback) {
    createContextMenuInit(contextMenuEl, '', 'tabs');

    const tabHeader = contextMenuEl.querySelector('.offcanvas-header').querySelectorAll('.nav-link');
    const tabPanes = contextMenuEl.querySelector('.offcanvas-body').querySelectorAll('.tab-pane');
    for (let i = 0; i < 2; i++) {
        tabHeader[i].addEventListener('click', function(event) {
            tabHeader[i].classList.add('active');
            tabPanes[i].classList.add('active', 'show');
            const j = i === 0 ? 1 : 0;
            tabHeader[j].classList.remove('active');
            tabPanes[j].classList.remove('active', 'show');
            event.preventDefault();
            event.stopPropagation();
        }, false);

        elClear(tabPanes[i]);
        if (i === 0) {
            tab1Callback(el, tabHeader[0], tabPanes[0]);
        }
        else {
            tab2Callback(el, tabHeader[1], tabPanes[1]);
        }
    }
}
