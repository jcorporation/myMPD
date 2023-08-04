"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module elements_js */

/**
 * Shortcut for elCreateTextTn for smartCount only
 * @param {string} tagName name of the tag to create
 * @param {object} attributes attributes to set
 * @param {string} text phrase to translate
 * @param {number} smartCount smart number for translation
 * @returns {HTMLElement} created dom node
 */
function elCreateTextTnNr(tagName, attributes, text, smartCount) {
    attributes["data-phrase-number"] = smartCount;
    return elCreateTextTn(tagName, attributes, text, undefined);
}

/**
 * Creates a html element and translates the text
 * @param {string} tagName name of the tag to create
 * @param {object} attributes tag attributes
 * @param {string} text text phrase to translate
 * @param {object} data object to resolve variables from the phrase
 * @returns {HTMLElement} created dom node
 */
function elCreateTextTn(tagName, attributes, text, data) {
    attributes["data-phrase"] = text;
    if (data !== undefined) {
        attributes["data-phrase-data"] = JSON.stringify(data);
    }
    if (attributes["data-phrase-number"] !== undefined) {
        if (data === undefined) {
            data = {};
        }
        //add smartCount to data from data-phrase-number attribute
        data.smartCount = Number(attributes["data-phrase-number"]);
    }
    return elCreateText(tagName, attributes, tn(text, data));
}

/**
 * Creates a html element with text content
 * @param {string} tagName name of the tag
 * @param {object} attributes tag attributes
 * @param {string} text text for the elements, respects \n for newlines
 * @returns {HTMLElement} created dom node
 */
function elCreateText(tagName, attributes, text) {
    if (attributes["data-title-phrase"] !== undefined) {
        attributes["title"] = tn(attributes["data-title-phrase"]);
    }
    const tag = elCreateEmpty(tagName, attributes);
    if (text.length > 0) {
        const lines = text.split(/\n/);
        for (let i = 0, j = lines.length; i < j; i++) {
            if (i > 0) {
                tag.appendChild(document.createElement('br'));
            }
            tag.appendChild(document.createTextNode(lines[i]));
        }
    }
    else {
        tag.textContent = text;
    }
    return tag;
}

/**
 * Creates a html element with a child node
 * @param {string} tagName name of the tag
 * @param {object} attributes tag attributes
 * @param {Element | Node} node node to add as child
 * @returns {HTMLElement} created dom node
 */
function elCreateNode(tagName, attributes, node) {
    const tag = elCreateEmpty(tagName, attributes);
    tag.appendChild(node);
    return tag;
}

/**
 * Creates a html element with child nodes
 * @param {string} tagName name of the tag
 * @param {object} attributes tag attributes
 * @param {object} nodes array of nodes to add as childs
 * @returns {HTMLElement} created dom node
 */
function elCreateNodes(tagName, attributes, nodes) {
    const tag = elCreateEmpty(tagName, attributes);
    for (const node of nodes) {
        if (node !== null) {
            tag.appendChild(node);
        }
    }
    return tag;
}

/**
 * Creates an empty html element
 * @param {string} tagName name of the tag
 * @param {object} attributes tag attributes
 * @returns {HTMLElement} created dom node
 */
function elCreateEmpty(tagName, attributes) {
    const tag = document.createElement(tagName);
    for (const key in attributes) {
        switch(key) {
            case 'class':
                tag.classList.add(...attributes[key]);
                break;
            case 'is':
                tag.setAttribute('data-is', attributes[key]);
                break;
            default:
                tag.setAttribute(key, attributes[key]);
        }
    }
    return tag;
}

/**
 * Clears the element with given id and appends the new child
 * @param {string} id id of the parent element
 * @param {Element | Node} child element to add
 * @returns {void}
 */
function elReplaceChildId(id, child) {
    elReplaceChild(document.getElementById(id), child);
}

/**
 * Clears the given element and appends the new child
 * @param {Element} el id of the parent element
 * @param {Element | Node} child element to add
 * @returns {void}
 */
function elReplaceChild(el, child) {
    elClear(el);
    el.appendChild(child);
}

/**
 * Hides the element with the given id
 * @param {string} id element id
 * @returns {void}
 */
function elHideId(id) {
    document.getElementById(id).classList.add('d-none');
}

/**
 * Shows the element with the given id
 * @param {string} id element id
 * @returns {void}
 */
function elShowId(id) {
    document.getElementById(id).classList.remove('d-none');
}

/**
 * Clears the element with the given id
 * @param {string} id element id to clear
 * @returns {void}
 */
function elClearId(id) {
    document.getElementById(id).textContent = '';
}

/**
 * Hides the element
 * @param {Element | EventTarget} el element to hide
 * @returns {void}
 */
function elHide(el) {
    el.classList.add('d-none');
}

/**
 * Shows the element
 * @param {Element} el element to show
 * @returns {void}
 */
function elShow(el) {
    el.classList.remove('d-none');
}

/**
 * Clears the element
 * @param {Element} el element to clear
 * @returns {void}
 */
function elClear(el) {
    el.textContent = '';
}

/**
 * Disables the element with the given id
 * @param {string} id element id
 * @returns {void}
 */
function elDisableId(id) {
    elDisable(document.getElementById(id));
}

/**
 * Disables the element
 * @param {Node} el element to disable
 * @returns {void}
 */
function elDisable(el) {
    el.setAttribute('disabled', 'disabled');
    el.classList.replace('clickable', 'not-clickable');
}

/**
 * Enables the element with the given id
 * @param {string} id element id
 * @returns {void}
 */
function elEnableId(id) {
    elEnable(document.getElementById(id));
}

/**
 * Enables the element
 * @param {Element | Node} el element to enable
 * @returns {void}
 */
function elEnable(el) {
    el.removeAttribute('disabled');
    el.classList.replace('not-clickable', 'clickable');
}

/**
 * Triggers a layout reflow
 * @param {Element} el element to trigger the reflow
 * @returns {number} element height
 */
function elReflow(el) {
    return el.offsetHeight;
}

/**
 * Sets the focus on the element with given id for desktop view.
 * @param {string} id element id
 * @returns {void}
 */
 function setFocusId(id) {
    setFocus(document.getElementById(id));
}

/**
 * Set the focus on the given element for desktop view.
 * @param {HTMLElement} el element to focus
 * @returns {void}
 */
function setFocus(el) {
    if (userAgentData.isMobile === false) {
        el.focus();
    }
}

/**
 * Sets an attribute on the element given by id.
 * @param {string} id element id
 * @param {string} attribute attribute name
 * @param {object} value could be any type
 * @returns {void}
 */
function setDataId(id, attribute, value) {
    document.getElementById(id)['myMPD-' + attribute] = value;
}

/**
 * Sets an attribute on the given element.
 * @param {Element | Node | EventTarget} el element
 * @param {string} attribute attribute name
 * @param {object} value could be any type
 * @returns {void}
 */
function setData(el, attribute, value) {
    el['myMPD-' + attribute] = value;
}

/**
 * Removes an attribute on the element given by id.
 * @param {string} id element id
 * @param {string} attribute attribute name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function rmDataId(id, attribute) {
    document.getElementById(id)['myMPD-' + attribute] = undefined;
}

/**
 * Removes an attribute on the given element.
 * @param {Element | Node} el element
 * @param {string} attribute attribute name
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function rmData(el, attribute) {
    el['myMPD-' + attribute] = undefined;
}

/**
 * Gets the attributes value from the element given by id.
 * @param {string} id element id
 * @param {string} attribute attribute name
 * @returns {object} attribute value
 */
function getDataId(id, attribute) {
    return getData(document.getElementById(id), attribute);
}

/**
 * Gets the attributes value from the element
 * @param {Element | EventTarget} el element
 * @param {string} attribute attribute name
 * @returns {object} attribute value or undefined
 */
function getData(el, attribute) {
    let value = el['myMPD-' + attribute];
    if (value === undefined) {
        //fallback to attribute
        value = el.getAttribute('data-' + attribute);
        if (value === null) {
            //return undefined if attribute is null
            value = undefined;
        }
    }
    logDebug('getData: "' + attribute + '":"' + value + '"');
    return value;
}

/**
 * Gets the value of the selected option of a select element
 * @param {string} id element id
 * @returns {string} selected option value
 */
function getSelectValueId(id) {
    return getSelectValue(document.getElementById(id));
}

/**
 * Gets the value of the selected option of a select element
 * or undefined if no option is selected
 * @param {Element | EventTarget} el element
 * @returns {string} selected option value
 */
function getSelectValue(el) {
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].getAttribute('value');
    }
    return undefined;
}

/**
 * Gets the attribute value of the selected option of a select element
 * @param {string} id element id
 * @param {string} attribute attribute name
 * @returns {object} selected option data value
 */
function getSelectedOptionDataId(id, attribute) {
    return getSelectedOptionData(document.getElementById(id), attribute);
}

/**
 * Gets the attribute value of the selected option of a select element
 * @param {Element} el element
 * @param {string} attribute attribute name
 * @returns {object} selected option data value
 */
function getSelectedOptionData(el, attribute) {
    if (el && el.selectedIndex >= 0) {
        return getData(el.options[el.selectedIndex], attribute);
    }
    return undefined;
}

/**
 * Gets the value of the checked radio box
 * @param {string} id element id
 * @returns {string} radio box value
 */
function getRadioBoxValueId(id) {
    return getRadioBoxValue(document.getElementById(id));
}

/**
 * Gets the value of the checked radio box
 * @param {Element} el element
 * @returns {string} radio box value
 */
function getRadioBoxValue(el) {
    const radiobuttons = el.querySelectorAll('.form-check-input');
    for(const button of radiobuttons) {
        if (button.checked === true){
            return button.value;
        }
    }
    return null;
}

/**
 * Gets the x-position of the given element
 * @param {Element} el element
 * @returns {number} x-position
 */
function getXpos(el) {
    let xPos = 0;
    while (el) {
        xPos += (el.offsetLeft - el.scrollLeft + el.clientLeft);
        el = el.offsetParent;
    }
    return xPos;
}

/**
 * Gets the y-position of the given element
 * @param {Element | ParentNode} el element
 * @returns {number} y-position
 */
function getYpos(el) {
    let yPos = 0;
    while (el) {
        yPos += (el.offsetTop + el.clientTop);
        el = el.offsetParent;
    }
    return yPos;
}

/**
 * Gets the index of the element in the parent html collection
 * @param {HTMLElement} el element to get the index
 * @returns {number} the index
 */
function elGetIndex(el) {
    return [...el.parentNode.children].indexOf(el);
}

/**
 * Adds a waiting animation to a button
 * @param {string} id id of the button
 * @param {boolean} waiting true = add animation, false = remove animation
 * @returns {void}
 */
 function btnWaitingId(id, waiting) {
    btnWaiting(document.getElementById(id), waiting);
}

/**
 * Adds a waiting animation to a button
 * @param {HTMLElement} btn id of the button
 * @param {boolean} waiting true = add animation, false = remove animation
 * @returns {void}
 */
function btnWaiting(btn, waiting) {
    if (waiting === true) {
        const spinner = elCreateEmpty('span', {"class": ["spinner-border", "spinner-border-sm", "me-2"]});
        btn.insertBefore(spinner, btn.firstChild);
        elDisable(btn);
    }
    else {
        //add a small delay, user should notice the change
        setTimeout(function() {
            elEnable(btn);
            if (btn.firstChild === null) {
                return;
            }
            if (btn.firstChild.nodeName === 'SPAN' &&
                btn.firstChild.classList.contains('spinner-border'))
            {
                btn.firstChild.remove();
            }
        }, 100);
    }
}

/**
 * Toggles a button group by value
 * @param {string} id button group id
 * @param {string | number} value value to select
 * @returns {HTMLElement} selected button
 */
function toggleBtnGroupValueId(id, value) {
    return toggleBtnGroupValue(document.getElementById(id), value);
}

/**
 * Toggles a button group by value
 * @param {Element} btngrp button group to toggle
 * @param {string | number} value value to select
 * @returns {HTMLElement} selected button
 */
function toggleBtnGroupValue(btngrp, value) {
    const btns = btngrp.querySelectorAll('button');
    //first button
    let b = btns[0];
    // @ts-ignore
    const valuestr = isNaN(value) ? value : value.toString();

    for (let i = 0, j = btns.length; i < j; i++) {
        if (getData(btns[i], 'value') === valuestr) {
            b = btns[i];
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    b.classList.add('active');
    return b;
}

/**
 * Toggles a button group by value and toggle a collapse
 * @param {Element} btngrp button group to toggle
 * @param {string} collapseId id of element to collapse
 * @param {string | number} value value to select
 * @returns {void}
 */
function toggleBtnGroupValueCollapse(btngrp, collapseId, value) {
    const activeBtn = toggleBtnGroupValue(btngrp, value);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        document.getElementById(collapseId).classList.add('show');
    }
    else {
        document.getElementById(collapseId).classList.remove('show');
    }
}

/**
 * Toggles a button group by triggering element
 * @param {string} id id of triggered button
 * @returns {HTMLElement | EventTarget} active button
 */
//eslint-disable-next-line no-unused-vars
function toggleBtnGroupId(id) {
    return toggleBtnGroup(document.getElementById(id));
}

/**
 * Toggles a button group by triggering element
 * @param {HTMLElement | EventTarget} btn triggered button
 * @returns {HTMLElement | EventTarget} active button
 */
function toggleBtnGroup(btn) {
    const btns = btn.parentNode.querySelectorAll('button');
    for (let i = 0, j = btns.length; i < j; i++) {
        if (btns[i] === btn) {
            btns[i].classList.add('active');
        }
        else {
            btns[i].classList.remove('active');
        }
    }
    return btn;
}

/**
 * Toggles a button group by triggering element and toggle a collapse
 * @param {HTMLElement} el triggering button 
 * @param {string} collapseId id of element to collapse
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleBtnGroupCollapse(el, collapseId) {
    const activeBtn = toggleBtnGroup(el);
    if (activeBtn.getAttribute('data-collapse') === 'show') {
        if (document.getElementById(collapseId).classList.contains('show') === false) {
            uiElements[collapseId].show();
        }
    }
    else {
        uiElements[collapseId].hide();
    }
}

/**
 * Gets the value from the active button in a button group
 * @param {string} id id of the button group
 * @returns {object} value the value of the active button
 */
function getBtnGroupValueId(id) {
    let activeBtn = document.querySelector('#' + id + ' > .active');
    if (activeBtn === null) {
        //fallback to first button
        activeBtn = document.querySelector('#' + id + ' > button');
    }
    return getData(activeBtn, 'value');
}

/**
 * Toggles the active state of a button
 * @param {string} id id of button to toggle
 * @param {boolean | number} state true, 1 = active; false, 0 = inactive
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleBtnId(id, state) {
    toggleBtn(document.getElementById(id), state);
}

/**
 * Toggles the active state of a button
 * @param {HTMLElement | EventTarget} btn button to toggle
 * @param {boolean | number} state true, 1 = active; false, 0 = inactive
 * @returns {void}
 */
function toggleBtn(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true) {
        btn.classList.add('active');
    }
    else {
        btn.classList.remove('active');
    }
}

/**
 * Mirrors the button horizontal
 * @param {string} id button id to mirror
 * @param {boolean} mirror true = mirror, false = not
 * @returns {void}
 */
function mirrorBtnId(id, mirror) {
    mirrorBtn(document.getElementById(id), mirror);
}

/**
 * Mirrors the button horizontal
 * @param {HTMLElement | EventTarget} btn button to mirror
 * @param {boolean} mirror true = mirror, false = not
 * @returns {void}
 */
function mirrorBtn(btn, mirror) {
    if (mirror === true) {
        btn.classList.add('mirror');
    }
    else {
        btn.classList.remove('mirror');
    }
}

/**
 * Gets the enabled state of a check button
 * @param {string} id check button id
 * @returns {boolean} enabled = true, disabled = false
 */
function getBtnChkValueId(id) {
    return getBtnChkValue(document.getElementById(id));
}

/**
 * Gets the enabled state of a check button
 * @param {HTMLElement | EventTarget} btn check button id
 * @returns {boolean} enabled = true, disabled = false
 */
function getBtnChkValue(btn) {
    return btn.classList.contains('active');
}

/**
 * Toggles a check button
 * @param {string} id id of the button to toggle
 * @param {boolean} state true, 1 = active; false, 0 = inactive
 * @returns {void}
 */
function toggleBtnChkId(id, state) {
    toggleBtnChk(document.getElementById(id), state);
}

/**
 * Toggles a check button
 * @param {HTMLElement | EventTarget} btn the button to toggle
 * @param {boolean | number} state true, 1 = active; false, 0 = inactive
 * @returns {boolean} true if button is checked, else false
 */
function toggleBtnChk(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true) {
        btn.classList.add('active');
        btn.textContent = 'check';
        return true;
    }
    else {
        btn.classList.remove('active');
        btn.textContent = 'radio_button_unchecked';
        return false;
    }
}

/**
 * Toggles a check button and an assigned collapse
 * @param {string} id the id of the triggering button
 * @param {string} collapseId id of element to collapse
 * @param {boolean | number} state true = active, false = inactive
 * @returns {void}
 */
function toggleBtnChkCollapseId(id, collapseId, state) {
    toggleBtnChkCollapse(document.getElementById(id), collapseId, state);
}

/**
 * Toggles a check button and an assigned collapse
 * @param {HTMLElement} btn triggering button
 * @param {string} collapseId id of element to collapse
 * @param {boolean | number} state true = active, false = inactive
 * @returns {void}
 */
function toggleBtnChkCollapse(btn, collapseId, state) {
    const checked = toggleBtnChk(btn, state);
    if (checked === true) {
        document.getElementById(collapseId).classList.add('show');
    }
    else {
        document.getElementById(collapseId).classList.remove('show');
    }
}

/**
 * Gets the y-scrolling position
 * @param {HTMLElement | Element} [el] element
 * @returns {number} the vertical scrolling position
 */
function getScrollPosY(el) {
    // element in scrolling modal
    if (el) {
        const modal = el.closest('.modal');
        if (modal) {
            let scrollPos = window.scrollY;
            scrollPos += modal.scrollTop;
            return scrollPos;
        }
    }
    if (userAgentData.isMobile === true) {
        // scrolling body
        return document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
    }
    // scrolling container
    const container = document.getElementById(app.id + 'List');
    if (container) {
        return container.parentNode.scrollTop;
    }
    return 0;
}

/**
 * Scrolls the container or the window to the y-position
 * @param {Element | ParentNode} container or null
 * @param {number} pos position to scroll to
 * @returns {void}
 */
function scrollToPosY(container, pos) {
    if (userAgentData.isMobile === true ||
        container === null)
    {
        elReflow(domCache.body);
        // For Safari
        document.body.scrollTop = pos;
        // For Chrome, Firefox, IE and Opera
        document.documentElement.scrollTop = pos;
    }
    else {
        container.scrollTop = pos;
    }
}
