"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

class inputReset extends HTMLElement {
    constructor() {
        super();

        const placeholder = this.getAttribute('placeholder');
        const invalidPhrase = this.getAttribute('data-invalid-phrase');
        const unitPhrase = this.getAttribute('data-unit-phrase');
        const inputType = this.getAttribute('type');
        const alwaysEnabled = this.classList.contains('alwaysEnabled') ? 'alwaysEnabled' : '';
        this.innerHTML = 
            '<div class="input-group">' +
                '<input id="' + this.getAttribute('id') + '" placeholder="' + (placeholder === null ? '' : e(placeholder)) + '" type="' + 
                    (inputType === null ? 'text' : inputType) + '" class="form-control border-secondary ' + alwaysEnabled + '"/>' +
                '<div class="input-group-append">' +
                    '<button data-title-phrase="Reset to default" class="btn btn-secondary resetBtn rounded-right ' + alwaysEnabled + '">' +
                        '<span class="mi">settings_backup_restore</span>' +
                    '</button>' +
                    (unitPhrase === null ? '' : '<div class="input-group-text-nobg" data-phrase="' + e(unitPhrase) + '"></div>') +
                '</div>' + 
                (invalidPhrase === null ? '' : '<div class="invalid-feedback" data-phrase="' + e(invalidPhrase) + '"></div>') +
            '</div>';
        
        this.input = this.getElementsByTagName('input')[0];
        this.button = this.getElementsByTagName('button')[0];
    }
    
    connectedCallback() {
        this.button.addEventListener('click', function(event) {
            resetToDefault(event.target);
        }, false);
        //populate input value from elements value attribute
        this.input.value = this.getAttribute('value');
    }

    get value() {
        return this.input.value
    }
    
    set value(newValue) {
        this.input.value = newValue;
    }
};

class inputClear extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreate('button', {"class": ["mi", "mi-small", "clear-button", "btn-secondary"]}, 'clear');
        this.parentNode.insertBefore(button, this.nextSibling);
        this.button = button;
    }
    connectedCallback() {
        this.button.addEventListener('click', function(event) {
            event.target.previousSibling.value = '';
            const dataClearEvent = event.target.previousSibling.getAttribute('data-clear-event');
            if (dataClearEvent !== null) {
                const clearEvent = new Event(dataClearEvent);
                event.target.previousSibling.dispatchEvent(clearEvent);
            }
        }, false);
    }
};

customElements.define('mympd-input-clear', inputClear, {extends: 'input'});
customElements.define('mympd-input-reset', inputReset);
