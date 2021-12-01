"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

class inputClear extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreateText('button', {"data-title-phrase": "Clear", "title": tn('Clear'), "class": ["mi", "mi-small", "input-inner-button"]}, 'clear');
        this.button = button;
        this.classList.add('innerButton');
    }
    connectedCallback() {
        if (this.parentNode.classList.contains('col')) {
            this.button.style.right = '1rem';
        }
        this.parentNode.insertBefore(this.button, this.nextElementSibling);
        if (this.value === '') {
            elHide(this.button);
        }
        else {
            elShow(this.button);
        }
        this.addEventListener('keyup', function(event) {
            if (event.target.value === '') {
                elHide(event.target.button);
            }
            else {
                elShow(event.target.button);
            }
        }, false);

        this.button.addEventListener('click', function(event) {
            event.preventDefault();
        }, false);

        this.button.addEventListener('mouseup', function(event) {
            event.target.previousElementSibling.value = '';
            const dataClearEvent = event.target.previousElementSibling.getAttribute('data-clear-event');
            if (dataClearEvent !== null) {
                const clearEvent = new Event(dataClearEvent);
                event.target.previousElementSibling.dispatchEvent(clearEvent);
            }
        }, false);
    }
}

class inputReset extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreateText('button', {"data-title-phrase": "Reset to default", "title": tn('Reset to default'), "class": ["mi", "mi-small", "input-inner-button"]}, 'settings_backup_restore');
        this.button = button;
        this.classList.add('innerButton');
    }
    connectedCallback() {
        if (this.parentNode.firstElementChild.getAttribute('type') === 'color' ||
            this.parentNode.classList.contains('col-sm-8'))
        {
            this.button.style.right = '1rem';
        }
        if (this.nextElementSibling) {
            this.parentNode.insertBefore(this.button, this.nextElementSibling);
        }
        else {
            this.parentNode.appendChild(this.button);
        }
        this.button.addEventListener('click', function(event) {
            event.preventDefault();
            event.stopPropagation();
            const input = event.target.previousElementSibling;
            input.value = getData(input, 'default') !== undefined ? getData(input, 'default') :
                (input.getAttribute('placeholder') !== null ? input.getAttribute('placeholder') : '');
        }, false);
    }
}

class selectSearch extends HTMLInputElement {
    constructor() {
        super();
        const filterInput = elCreateEmpty('input', {"class": ["form-control", "form-control-sm", "mb-1"], "data-placeholder-phrase": "Filter", "placeholder": tn('Filter')});
        const filterResult = elCreateEmpty('select', {"class": ["form-select", "form-select-sm"], "size": 10});
        const dropdown = elCreateNodes('div', {"class": ["dropdown-menu", "dropdown-menu-dark", "p-2", "w-100"]}, [
            filterInput,
            filterResult
        ]);
        this.parentNode.insertBefore(dropdown, this.nextElementSibling);

        const button = elCreateEmpty('button', {"class": ["input-inner-button", "select-inner-button"], "data-bs-toggle": "dropdown"});
        if (this.parentNode.classList.contains('col-sm-8')) {
            button.style.right = '1rem';
        }
        button.style.cursor = 'default';
        this.parentNode.insertBefore(button, this.nextElementSibling);
        this.dropdownButton = button;
        this.filterInput = filterInput;
        this.filterResult = filterResult;
        this.classList.add('innerButton');
    }
    connectedCallback() {
        const input = this;
        this.filterResult.addEventListener('click', function(event) {
            event.preventDefault();
            event.stopPropagation();
            input.value = event.target.text;
            setData(input, 'value', event.target.value);
            input.dropdownButton.Dropdown.hide();
        }, false);
        this.filterInput.addEventListener('keyup', function(event) {
            const cb = getData(input, 'cb-filter');
            const cbOptions = getData(input, 'cb-filter-options');
            window[cb](... cbOptions, event.target.value);
        }, false);
        new BSN.Dropdown(input.dropdownButton);
        if (input.getAttribute('readonly') === 'readonly') {
            input.addEventListener('click', function() {
                input.dropdownButton.Dropdown.toggle();
            }, false);
        }
        this.dropdownButton.addEventListener('click', function(event) {
            event.preventDefault();
            event.stopPropagation();
        }, false);
    }
}

customElements.define('mympd-input-clear', inputClear, {extends: 'input'});
customElements.define('mympd-input-reset', inputReset, {extends: 'input'});
customElements.define('mympd-select-search', selectSearch, {extends: 'input'});
