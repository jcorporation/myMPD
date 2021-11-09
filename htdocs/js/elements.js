"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

class inputClear extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreateText('button', {"class": ["mi", "mi-small", "input-inner-button", "btn-secondary"]}, 'clear');
        this.button = button;
    }
    connectedCallback() {
        this.parentNode.insertBefore(this.button, this.nextElementSibling);
        if (this.value === '')  {
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
        this.button.addEventListener('mouseup', function(event) {
            event.target.previousSibling.value = '';
            const dataClearEvent = event.target.previousSibling.getAttribute('data-clear-event');
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
        const button = elCreateText('button', {"class": ["mi", "mi-small", "input-inner-button"]}, 'settings_backup_restore');
        this.button = button;
    }
    connectedCallback() {
        if (this.parentNode.firstElementChild.getAttribute('type') === 'color') {
            this.button.style.right = '1.5rem';
        }
        else if (this.parentNode.classList.contains('col-sm-8')) {
            this.button.style.right = '1rem';
        }
        if (this.nextElementSibling) {
            this.parentNode.insertBefore(this.button, this.nextElementSibling);
        }
        else {
            this.parentNode.appendChild(this.button);
        }
        this.button.addEventListener('mouseup', function(event) {
            const input = event.target.previousElementSibling;
            input.value = getData(input, 'data-default') !== undefined ? getData(input, 'data-default') : 
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
        this.parentNode.insertBefore(dropdown, this.nextSibling);
        
        const button = elCreateEmpty('button', {"class": ["input-inner-button", "select-inner-button"], "data-bs-toggle": "dropdown"});
        if (this.parentNode.classList.contains('col-sm-8')) {
            button.style.right = '1rem';
        }
        button.style.cursor = 'default';
        this.parentNode.insertBefore(button, this.nextSibling);
        this.dropdownButton = button;
        this.filterInput = filterInput;
        this.filterResult = filterResult;
    }
    connectedCallback() {
        const input = this;
        this.filterResult.addEventListener('click', function(event) {
            input.value = event.target.text;
            setData(input, 'data-value', event.target.value);
            input.dropdownButton.Dropdown.hide();
        }, false);
        this.filterInput.addEventListener('keyup', function(event) {
            const cb = getData(input, 'data-cb-filter');
            const cbOptions = getData(input, 'data-cb-filter-options');
            window[cb](... cbOptions, event.target.value);
        }, false);
        new BSN.Dropdown(input.dropdownButton);
        if (input.getAttribute('readonly') === 'readonly') {
            input.addEventListener('click', function() {
                input.dropdownButton.Dropdown.toggle();
            }, false);
        }
    }
}

customElements.define('mympd-input-clear', inputClear, {extends: 'input'});
customElements.define('mympd-input-reset', inputReset, {extends: 'input'});
customElements.define('mympd-select-search', selectSearch, {extends: 'input'});
