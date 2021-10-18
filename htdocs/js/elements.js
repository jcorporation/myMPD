"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

class inputClear extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreate('button', {"class": ["mi", "mi-small", "input-inner-button", "btn-secondary"]}, 'clear');
        this.parentNode.insertBefore(button, this.nextSibling);
        this.button = button;
        if (this.value === '')  {
            elHide(this.button);
        }
        else {
            elShow(this.button);
        }
    }
    connectedCallback() {
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
                event.target.previousSibling.dispatchEvent(clearEvent);
            }
        }, false);
    }
}

class inputReset extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreate('button', {"class": ["mi", "mi-small", "input-inner-button", "btn-secondary"]}, 'settings_backup_restore');
        if (this.parentNode.firstElementChild.getAttribute('type') === 'color') {
            button.style.right = '1.5rem';
        }
        else if (this.parentNode.classList.contains('col-sm-8')) {
            button.style.right = '1rem';
        }
        this.parentNode.insertBefore(button, this.nextSibling);
        this.button = button;
    }
    connectedCallback() {
        this.button.addEventListener('mouseup', function(event) {
            const input = event.target.previousSibling;
            input.value = getCustomDomProperty(input, 'data-default') !== null ? getCustomDomProperty(input, 'data-default') : 
                (input.getAttribute('placeholder') !== null ? input.getAttribute('placeholder') : '');
        }, false);
    }
}

customElements.define('mympd-input-clear', inputClear, {extends: 'input'});
customElements.define('mympd-input-reset', inputReset, {extends: 'input'});
