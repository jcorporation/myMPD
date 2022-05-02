"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

//we do not use the custom element is="" feature - safari does not support it

function initElements(parent) {
    for (const el of parent.querySelectorAll('[data-is]')) {
        initElement(el, el.getAttribute('data-is'));
    }
}

function initElement(el, elType) {
    switch(elType) {
        case 'mympd-input-clear':
            setInputClear(el);
            break;
        case 'mympd-input-reset':
            setInputReset(el);
            break;
        case 'mympd-input-password':
            setInputPassword(el);
            break;
        case 'mympd-select-search':
            setSelectSearch(el);
            break;
    }
    el.removeAttribute('data-is');
}

function setInputClear(el) {
    const button = elCreateText('button', {"data-title-phrase": "Clear", "title": tn('Clear'), "class": ["mi", "mi-small", "input-inner-button"]}, 'clear');
    el.button = button;
    el.classList.add('innerButton');
    if (el.parentNode.classList.contains('col')) {
        el.button.style.right = '1rem';
    }
    el.parentNode.insertBefore(el.button, el.nextElementSibling);
    if (el.value === '') {
        elHide(el.button);
    }
    else {
        elShow(el.button);
    }
    el.addEventListener('keyup', function(event) {
        if (event.target.value === '') {
            elHide(event.target.button);
        }
        else {
            elShow(event.target.button);
        }
    }, false);

    el.button.addEventListener('click', function(event) {
        event.preventDefault();
    }, false);

    el.button.addEventListener('mouseup', function(event) {
        event.target.previousElementSibling.value = '';
        const dataClearEvent = event.target.previousElementSibling.getAttribute('data-clear-event');
        if (dataClearEvent !== null) {
            const clearEvent = new Event(dataClearEvent);
            event.target.previousElementSibling.dispatchEvent(clearEvent);
        }
    }, false);
}

function setInputReset(el) {
    const button = elCreateText('button', {"data-title-phrase": "Reset to default", "title": tn('Reset to default'), "class": ["mi", "mi-small", "input-inner-button"]}, 'settings_backup_restore');
    el.button = button;
    el.classList.add('innerButton');
    if (el.parentNode.firstElementChild.getAttribute('type') === 'color' ||
        el.parentNode.classList.contains('col-sm-8'))
    {
        el.button.style.right = '1rem';
    }
    if (el.nextElementSibling) {
        el.parentNode.insertBefore(el.button, el.nextElementSibling);
    }
    else {
        el.parentNode.appendChild(el.button);
    }
    el.button.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const input = event.target.previousElementSibling;
        input.value = getData(input, 'default') !== undefined ? getData(input, 'default') :
            (input.getAttribute('placeholder') !== null ? input.getAttribute('placeholder') : '');
    }, false);
}

function setInputPassword(el) {
    const button = elCreateText('button', {"data-title-phrase": "Show or hide", "title": tn('Show or hide'), "class": ["mi", "mi-small", "input-inner-button"]}, 'visibility');
    el.button = button;
    el.classList.add('innerButton');
    if (el.parentNode.classList.contains('col-sm-8')) {
        el.button.style.right = '1rem';
    }
    if (el.nextElementSibling) {
        el.parentNode.insertBefore(el.button, el.nextElementSibling);
    }
    else {
        el.parentNode.appendChild(el.button);
    }
    el.button.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        const input = event.target.previousElementSibling;
        if (input.type === 'password') {
            input.type = 'text';
            event.target.textContent = 'visibility_off';
        }
        else {
            input.type = 'password';
            event.target.textContent = 'visibility';
        }
    }, false);
}

function setSelectSearch(el) {
    const filterInput = elCreateEmpty('input', {"class": ["form-control", "form-control-sm", "mb-1"], "data-placeholder-phrase": "Filter", "placeholder": tn('Filter')});
    const filterResult = elCreateEmpty('ul', {"class": ["list-group", "list-group-scroll", "border", "border-secondary"]});
    const dropdown = elCreateNodes('div', {"class": ["dropdown-menu", "dropdown-menu-dark", "p-2", "w-100"]}, [
        filterInput,
        filterResult
    ]);
    el.parentNode.insertBefore(dropdown, el.nextElementSibling);

    const button = elCreateEmpty('button', {"class": ["input-inner-button", "select-inner-button"], "data-bs-toggle": "dropdown"});
    if (el.parentNode.classList.contains('col-sm-8')) {
        button.style.right = '1rem';
    }
    button.style.cursor = 'default';
    el.parentNode.insertBefore(button, el.nextElementSibling);
    el.dropdownButton = button;
    el.filterInput = filterInput;
    el.filterResult = filterResult;
    el.classList.add('innerButton');
    setData(el, 'value', el.value);
    el.addEventListener('keyup', function(event) {
        setData(el, 'value', event.target.value);
    }, false);
    el.filterResult.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
        el.value = event.target.textContent;
        setData(el, 'value', getData(event.target, 'value'));
        BSN.Dropdown.getInstance(el.nextElementSibling).hide();
        const changeEvent = new Event('change');
        el.dispatchEvent(changeEvent);
    }, false);
    el.filterInput.addEventListener('keyup', function(event) {
        const cb = getData(el, 'cb-filter');
        const cbOptions = getData(el, 'cb-filter-options');
        window[cb](... cbOptions, event.target.value);
    }, false);
    el.filterInput.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    el.addFilterResult = function(text, value) {
        const item = elCreateText('li', {"class": ["list-group-item", "list-group-item-action", "clickable"]}, text);
        setData(item, 'value', value);
        el.filterResult.appendChild(item);
    };
    new BSN.Dropdown(el.dropdownButton);
    if (el.getAttribute('readonly') === 'readonly') {
        el.addEventListener('click', function(event) {
            BSN.Dropdown.getInstance(event.target.nextElementSibling).toggle();
        }, false);
    }
    if (isMobile === true) {
        el.parentNode.addEventListener('shown.bs.dropdown', function() {
            domCache.body.style.overflow = 'hidden';
        }, false);
        el.parentNode.addEventListener('hidden.bs.dropdown', function() {
            domCache.body.style.overflow = 'initial';
        }, false);
    }
    el.dropdownButton.addEventListener('click', function(event) {
        event.preventDefault();
        event.stopPropagation();
    }, false);
}
