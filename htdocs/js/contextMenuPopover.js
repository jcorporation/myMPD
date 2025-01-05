"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module contextMenuPopover_js */

/**
 * Shows the context menu popover
 * @param {HTMLElement} target calculated target
 * @param {string} contextMenuType type of the context menu
 * @returns {void}
 */
function showPopover(target, contextMenuType) {
    hidePopover(target);
    //context menu is shown
    if (target.getAttribute('aria-describedby') !== null ||
        target.classList.contains('not-clickable'))
    {
        return;
    }
    //check for existing popover instance
    let popoverInit = BSN.Popover.getInstance(target);
    //create it, if no popover instance is found
    if (popoverInit === null) {
        switch (contextMenuType) {
            case 'NavbarPlayback':
            case 'NavbarQueue':
            case 'NavbarBrowse':
                popoverInit = createPopoverSimple(target, target.getAttribute('title'), addMenuItemsNavbarActions);
                break;
            case 'footer':
                popoverInit = createPopoverSimple(target, 'Playback controls', addActionsPopoverFooter);
                domCache.footer.addEventListener('updated.bs.popover', function() {
                    updatePlaybackControls();
                }, false);
                domCache.footer.addEventListener('show.bs.popover', function() {
                    elGetById('advPlaybackControlsBtn').classList.add('active');
                }, false);
                domCache.footer.addEventListener('hidden.bs.popover', function() {
                    elGetById('advPlaybackControlsBtn').classList.remove('active');
                }, false);
                break;
            // No Default
        }
    }
    popoverInit.show();
}

/**
 * Hides all popovers
 * @param {EventTarget} [thisEl] triggering element
 * @returns {void}
 */
function hidePopover(thisEl) {
    const popoverEls = document.querySelectorAll('[aria-describedby]');
    for (const el of popoverEls) {
        if (thisEl === el) {
            //do not hide popover that should be opened
            continue;
        }
        BSN.Popover.getInstance(el).hide();
    }
    if (popoverEls.length === 0) {
        //handle popover dom nodes without a trigger element
        const popover = document.querySelector('.popover');
        if (popover !== null) {
            //simply remove the popover dom node
            popover.remove();
        }
    }
}

/**
 * Creates the popover body
 * @param {string} template tabs = create a popover body with two tabes, else create an empty body
 * @returns {HTMLElement} the popover body
 */
function createPopoverBody(template) {
    if (template === 'tabs') {
        return elCreateNodes('div', {"class": ["popover-tabs", "py-2"]}, [
                   elCreateNodes('ul', {"class": ["nav", "nav-tabs", "px-2"]}, [
                       elCreateNode('li', {"class": ["nav-item"]},
                           elCreateEmpty('a', {"class": ["nav-link", "active"], "href": "#"})
                       ),
                       elCreateNode('li', {"class": ["nav-item"]},
                           elCreateEmpty('a', {"class": ["nav-link"], "href": "#"})
                       )
                   ]),
                   elCreateNodes('div', {"class": ["tab-content"]}, [
                       elCreateEmpty('div', {"class": ["tab-pane", "pt-2", "active", "show"], "id": "popoverTab0"}),
                       elCreateEmpty('div', {"class": ["tab-pane", "pt-2"], "id": "popoverTab1"})
                   ])
               ]);
    }
    return elCreateEmpty('div', {"class": ["popover-body"]});
}

/**
 * Creates a new BSN popover
 * @param {EventTarget} target triggering element
 * @param {string} title popover title 
 * @param {string} [bodyTemplate] the popover body
 * @returns {object} BSN popover object
 */
function createPopoverInit(target, title, bodyTemplate) {
    const template = elCreateNodes('div', {"class": ["popover"]}, [
            elCreateEmpty('div', {"class": ["popover-arrow"]}),
            elCreateEmpty('h3', {"class": ["popover-header"]}),
            createPopoverBody(bodyTemplate)
        ]
    );
    const options = {
        trigger: 'manual',
        delay: 0,
        dismissible: false,
        title: (title !== '' ? elCreateText('span', {}, title) : ''),
        template: template, content: document.createTextNode('dummy')
    };

    let popoverType = target.getAttribute('data-contextmenu');
    if (popoverType === null) {
        popoverType = target.getAttribute('data-col');
    }
    if (popoverType === null) {
        popoverType = target.parentNode.getAttribute('data-col');
    }
    switch(popoverType) {
        case 'NavbarPlayback':
        case 'NavbarQueue':
        case 'NavbarBrowse':
            // @ts-ignore
            options.placement = getXpos(target) < 100
                ? 'right'
                : 'bottom';
            break;
        // No Default
    }
    return new BSN.Popover(target, options);
}

/**
 * Creates the click handler for the popover menu
 * @param {HTMLElement} el container of the menu items
 * @returns {void}
 */
function createPopoverClickHandler(el) {
    el.addEventListener('click', function(eventClick) {
        if (eventClick.target.nodeName === 'A') {
            const cmd = getData(eventClick.target, 'href');
            if (cmd) {
                parseCmd(eventClick, cmd);
                hidePopover();
            }
        }
        else if (eventClick.target.nodeName === 'BUTTON') {
            const cmd = getData(eventClick.target, 'href');
            if (typeof(cmd) === 'object') {
                parseCmd(eventClick, cmd);
            }
            else {
                parseCmdFromJSON(eventClick, cmd);
            }
        }
        eventClick.preventDefault();
        eventClick.stopPropagation();
    }, false);
}

/**
 * Creates a simple popover
 * @param {EventTarget} target triggering element
 * @param {string} title popover title
 * @param {Function} contentCallback callback to create the popover content
 * @returns {object} BSN popover object
 */
function createPopoverSimple(target, title, contentCallback) {
    const popoverInit = createPopoverInit(target, tn(title));
    //update content on each show event
    target.addEventListener('show.bs.popover', function() {
        const popoverBody = elCreateEmpty('div', {"class": ["popover-body", "px-0"]});
        popoverInit.tooltip.querySelector('.popover-body').replaceWith(popoverBody);
        contentCallback(target, popoverBody);
        createPopoverClickHandler(popoverBody);
    }, false);
    return popoverInit;
}

/**
 * Creates a popover with two tabs
 * @param {EventTarget} target triggering element
 * @param {Function} tab1Callback callback to create the popover content for the first tab
 * @param {Function} tab2Callback callback to create the popover content for the second tab
 * @returns {object} BSN popover object
 */
//eslint-disable-next-line no-unused-vars
function createPopoverTabs(target, tab1Callback, tab2Callback) {
    const popoverInit = createPopoverInit(target, '', 'tabs');
    //update content on each show event
    target.addEventListener('show.bs.popover', function() {
        popoverInit.tooltip.querySelector('.popover-tabs').replaceWith(createPopoverBody('tabs'));
        const tabHeader = popoverInit.tooltip.querySelectorAll('.nav-link');
        const tabPanes = popoverInit.tooltip.querySelectorAll('.tab-pane');
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
            const created = i === 0 ?
                tab1Callback(target, tabHeader[0], tabPanes[0]) :
                tab2Callback(target, tabHeader[1], tabPanes[1]);

            if (created === true) {
                createPopoverClickHandler(tabPanes[i]);
            }
            else {
                popoverInit.tooltip.querySelector('.popover-header').textContent = tabHeader[0].textContent;
                tabHeader[0].parentNode.parentNode.remove();
            }
        }
    }, false);
    return popoverInit;
}

/**
 * Populates the footer popover
 * @param {EventTarget} target the triggering target
 * @param {Element} popoverBody the popover body
 * @returns {void}
 */
function addActionsPopoverFooter(target, popoverBody) {
    const elapsed = currentState !== null
        ? fmtSongDuration(currentState.elapsedTime)
        : '0:00';
    popoverBody.appendChild(
        elCreateNodes('form', {"class": ["px-3"]}, [
            elCreateNodes('div', {"class": ["btn-group-vertical", "playbackPopoverBtns"]}, [
                elCreateNodes('div', {"class": ["btn-group"]}, [
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "rounded-start"], "id": "popoverFooterPrevBtn", "data-href": '{"cmd": "clickPrev", "options": []}'}, 'skip_previous'),
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi"], "id": "popoverFooterPlayBtn", "data-href": '{"cmd": "clickPlay", "options": []}'}, 'play_arrow'),
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi"], "id": "popoverFooterStopBtn", "data-href": '{"cmd": "clickStop", "options": []}'}, 'stop'),
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "rounded-end"], "id": "popoverFooterNextBtn", "data-href": '{"cmd": "clickNext", "options": []}'}, 'skip_next')
                ]),
                elCreateTextTn('div', {"class": ["w-100", "text-center", "p-2"]}, 'Seek seconds'),
                elCreateNodes('div', {"class": ["btn-group"]}, [
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "rounded-start"], "id": "popoverFooterFastRewindBtn", "data-href": '{"cmd": "clickFastRewindValue", "options": []}'}, 'fast_rewind'),
                    elCreateEmpty('input', {"class": ["form-control", "rounded-0", "text-center"], "id": "popoverFooterSeekInput", "value": lastSeekStep.toString()}),
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "rounded-end"], "id": "popoverFooterFastForwardBtn", "data-href": '{"cmd": "clickFastForwardValue", "options": []}'}, 'fast_forward')
                ]),
                elCreateTextTn('div', {"class": ["w-100", "text-center", "p-2"]}, 'Goto position'),
                elCreateNodes('div', {"class": ["btn-group"]}, [
                    elCreateEmpty('input', {"class": ["form-control", "rounded-start", "rounded-end-0"], "id": "popoverFooterGotoInput", "value": elapsed}),
                    elCreateText('button', {"class": ["btn", "btn-secondary", "mi", "rounded-end"], "id": "popoverFooterGotoBtn", "data-href": '{"cmd": "clickGotoPos", "options": []}'}, 'play_for_work')
                ])
            ])
        ])
    );
}
