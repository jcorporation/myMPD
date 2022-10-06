"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/**
 * Shortcut for elCreateTextTn for smartCount only
 * @param {String} tagName 
 * @param {Object} attributes 
 * @param {String} text 
 * @param {Number} smartCount 
 * @returns {HTMLElement} created dom node
 */
function elCreateTextTnNr(tagName, attributes, text, smartCount) {
    attributes["data-phrase-number"] = smartCount;
    return elCreateTextTn(tagName, attributes, text, undefined);
}

/**
 * Creates and translates a html element
 * @param {String} tagName name of the tag
 * @param {Object} attributes tag attributes
 * @param {String} text text phrase to translate
 * @param {Object} data object to resolve variables from the phrase
 * @returns {HTMLElement} created dom node
 */
function elCreateTextTn(tagName, attributes, text, data) {
    attributes["data-phrase"] = text;
    if (data !== undefined) {
        attributes["data-phrase-data"] = JSON.stringify(data);
    }
    if (attributes["data-phrase-number"] !== undefined) {
        //add smartCount to data from data-phrase-number attribute
        data.smartCount = Number(attributes["data-phrase-number"]);
    }
    return elCreateText(tagName, attributes, tn(text, data));
}

/**
 * Creates a html element with text content
 * @param {String} tagName name of the tag
 * @param {Object} attributes tag attributes
 * @param {String} text text phrase to translate
 * @returns {HTMLElement} created dom node
 */
function elCreateText(tagName, attributes, text) {
    if (attributes["data-title-phrase"] !== undefined) {
        attributes["title"] = tn(attributes["data-title-phrase"]);
    }
    const tag = elCreateEmpty(tagName, attributes);
    tag.textContent = text;
    return tag;
}

/**
 * Creates a html element with a child node
 * @param {String} tagName name of the tag
 * @param {Object} attributes tag attributes
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
 * @param {String} tagName name of the tag
 * @param {Object} attributes tag attributes
 * @param {Object} nodes array of nodes to add as childs
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
 * @param {String} tagName name of the tag
 * @param {Object} attributes tag attributes
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
 * @param {String} id id of the parent element
 * @param {Element | Node} child element to add
 */
function elReplaceChildId(id, child) {
    elReplaceChild(document.getElementById(id), child);
}

/**
 * Clears the given element and appends the new child
 * @param {Element} el id of the parent element
 * @param {Element | Node} child element to add
 */
function elReplaceChild(el, child) {
    elClear(el);
    el.appendChild(child);
}

/**
 * Hides the element with the given id
 * @param {String} id element id
 */
function elHideId(id) {
    document.getElementById(id).classList.add('d-none');
}

/**
 * Shows the element with the given id
 * @param {String} id element id
 */
function elShowId(id) {
    document.getElementById(id).classList.remove('d-none');
}

/**
 * Clears the element with the given id
 * @param {String} id element id
 */
function elClearId(id) {
    document.getElementById(id).textContent = '';
}

/**
 * Hides the element
 * @param {Element | EventTarget} el 
 */
function elHide(el) {
    el.classList.add('d-none');
}

/**
 * Shows the element
 * @param {Element} el 
 */
function elShow(el) {
    el.classList.remove('d-none');
}

/**
 * Clears the element
 * @param {Element} el 
 */
function elClear(el) {
    el.textContent = '';
}

/**
 * Disables the element with the given id
 * @param {String} id element id
 */
function elDisableId(id) {
    document.getElementById(id).setAttribute('disabled', 'disabled');
}

/**
 * Disables the element
 * @param {Node} el 
 */
function elDisable(el) {
    el.setAttribute('disabled', 'disabled');
    //manually disabled, remove disabled class
    el.classList.remove('disabled');
    el.classList.replace('clickable', 'not-clickable');
}

/**
 * Enables the element with the given id
 * @param {String} id element id
 */
function elEnableId(id) {
    document.getElementById(id).removeAttribute('disabled');
}

/**
 * Enables the element
 * @param {Element | Node} el 
 */
function elEnable(el) {
    el.removeAttribute('disabled');
    el.classList.replace('not-clickable', 'clickable');
}

/**
 * Triggers a layout reflow
 * @param {Element} el 
 * @returns {Number} element height
 */
function elReflow(el) {
    return el.offsetHeight;
}

/**
 * Sets the focus on the element with given id
 * @param {String} id 
 */
 function setFocusId(id) {
    setFocus(document.getElementById(id));
}

/**
 * Set the focus on the given element.
 * @param {HTMLElement} el 
 */
function setFocus(el) {
    if (userAgentData.isMobile === false) {
        el.focus();
    }
}

/**
 * Sets an attribute on the element given by id.
 * @param {String} id element id
 * @param {String} attribute 
 * @param {*} value could be any type
 */
function setDataId(id, attribute, value) {
    document.getElementById(id)['myMPD-' + attribute] = value;
}

/**
 * Sets an attribute on the given element.
 * @param {Element | Node} el
 * @param {String} attribute 
 * @param {*} value could be any type
 */
function setData(el, attribute, value) {
    el['myMPD-' + attribute] = value;
}

/**
 * Gets the attributes value from the element given by id.
 * @param {String} id element id
 * @param {String} attribute 
 * @returns {*} attribute value
 */
function getDataId(id, attribute) {
    return getData(document.getElementById(id), attribute);
}

/**
 * Gets the attributes value from the element
 * @param {Element | EventTarget} el
 * @param {String} attribute 
 * @returns {*} attribute value
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
 * @param {String} id element id
 * @returns {String}
 */
function getSelectValueId(id) {
    return getSelectValue(document.getElementById(id));
}

/**
 * Gets the value of the selected option of a select element
 * @param {Element | EventTarget} el 
 * @returns {String}
 */
function getSelectValue(el) {
    if (el && el.selectedIndex >= 0) {
        return el.options[el.selectedIndex].getAttribute('value');
    }
    return undefined;
}

/**
 * Gets the attribute value of the selected option of a select element
 * @param {String} id 
 * @param {String} attribute 
 * @returns {*}
 */
function getSelectedOptionDataId(id, attribute) {
    return getSelectedOptionData(document.getElementById(id), attribute)
}

/**
 * Gets the attribute value of the selected option of a select element
 * @param {Element} el 
 * @param {String} attribute 
 * @returns {*}
 */
function getSelectedOptionData(el, attribute) {
    if (el && el.selectedIndex >= 0) {
        return getData(el.options[el.selectedIndex], attribute);
    }
    return undefined;
}

/**
 * Gets the value of the checked radio box
 * @param {String} id 
 * @returns {String} radio box value
 */
function getRadioBoxValueId(id) {
    return getRadioBoxValue(document.getElementById(id));
}

/**
 * Gets the value of the checked radio box
 * @param {Element} el
 * @returns {String} radio box value
 */
function getRadioBoxValue(el) {
    const radiobuttons = el.querySelectorAll('.form-check-input');
    for(const button of radiobuttons) {
        if (button.checked === true){
            return button.value;
        }
    }
}

/**
 * Gets the x-position of the given element
 * @param {Element} el 
 * @returns {Number} x-position
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
 * @param {Element | ParentNode} el 
 * @returns {Number} y-position
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
 * Returns the nearest parent of type nodeName
 * @param {HTMLElement | EventTarget} el 
 * @param {String} nodeName 
 * @returns {ParentNode}
 */
 function getParent(el, nodeName) {
    /** @type {ParentNode} */
    let target = el.parentNode;
    let i = 0;
    while (target.nodeName !== nodeName) {
        i++;
        if (i > 10) {
            return null;
        }
        target = target.parentNode;
    }
    return target;
}

/**
 * Adds a waiting animation to a button
 * @param {String} id id of the button
 * @param {Boolean} waiting true = add animation, false = remove animation
 */
 function btnWaitingId(id, waiting) {
    btnWaiting(document.getElementById(id), waiting);
}

/**
 * Adds a waiting animation to a button
 * @param {HTMLElement} btn id of the button
 * @param {Boolean} waiting true = add animation, false = remove animation
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
 * @param {String} id button group id
 * @param {String | Number} value value to select
 * @returns {HTMLElement} selected button
 */
function toggleBtnGroupValueId(id, value) {
    return toggleBtnGroupValue(document.getElementById(id), value)
}

/**
 * Toggles a button group by value
 * @param {Element} btngrp button group to toggle
 * @param {String | Number} value value to select
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
 * @param {String} collapseId id of element to collapse
 * @param {String | Number} value value to select
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
 * @param {String} id id of triggered button
 * @returns {HTMLElement} active button
 */
//eslint-disable-next-line no-unused-vars
function toggleBtnGroupId(id) {
    return toggleBtnGroup(document.getElementById(id));
}

/**
 * Toggles a button group by triggering element
 * @param {HTMLElement} btn triggered button
 * @returns {HTMLElement} active button
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
 * @param {String} collapseId id of element to collapse
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
 * @param {String} id id of the button group
 * @returns {*} value
 */
function getBtnGroupValueId(id) {
    let activeBtn = document.querySelector('#' + id + ' > .active');
    if (activeBtn === null) {
        //fallback to first button
        activeBtn = document.querySelector('#' + id + ' > button');
    }
    return getData(activeBtn, 'value');
}

//eslint-disable-next-line no-unused-vars
function toggleBtnId(id, state) {
    toggleBtn(document.getElementById(id), state);
}

/**
 * Toggles the active state of a button
 * @param {HTMLElement | EventTarget} btn button to toggle
 * @param {Boolean | Number} state true = active, false = inactive 
 */
function toggleBtn(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true ||
        state === 1)
    {
        btn.classList.add('active');
    }
    else {
        btn.classList.remove('active');
    }
}

/**
 * Toggles a check button
 * @param {String} id id of the button to toggle
 * @param {Boolean} state true = active, false = inactive 
 */
function toggleBtnChkId(id, state) {
    toggleBtnChk(document.getElementById(id), state);
}

/**
 * Toggles a check button
 * @param {HTMLElement | EventTarget} btn the button to toggle
 * @param {Boolean | Number} state true = active, false = inactive 
 */
function toggleBtnChk(btn, state) {
    if (state === undefined) {
        //toggle state
        state = btn.classList.contains('active') ? false : true;
    }

    if (state === true ||
        state === 1)
    {
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
 * @param {String} id the id of the triggering button
 * @param {String} collapseId id of element to collapse
 * @param {Boolean | Number} state true = active, false = inactive 
 */
function toggleBtnChkCollapseId(id, collapseId, state) {
    toggleBtnChkCollapse(document.getElementById(id), collapseId, state);
}

/**
 * Toggles a check button and an assigned collapse
 * @param {HTMLElement} btn triggering button
 * @param {String} collapseId id of element to collapse
 * @param {Boolean | Number} state true = active, false = inactive 
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
