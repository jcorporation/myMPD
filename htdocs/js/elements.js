"use strict";
// SPDX-License-Identifier: GPL-2.0-or-later
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
                '<input id="' + this.getAttribute('id') + '" placeholder="' + (placeholder === null ? '' : placeholder) + '" type="' + 
                    (inputType === null ? 'text' : inputType) + '" class="form-control border-secondary ' + alwaysEnabled + '"/>' +
                '<div class="input-group-append">' +
                    '<button data-title-phrase="Reset to default" class="btn btn-secondary resetBtn rounded-right ' + alwaysEnabled + '">' +
                        '<span class="mi">settings_backup_restore</span>' +
                    '</button>' +
                    (unitPhrase === null ? '' : '<div class="input-group-text-nobg" data-phrase="' + unitPhrase + '"></div>') +
                '</div>' + 
                (invalidPhrase === null ? '' : '<div class="invalid-feedback" data-phrase="' + invalidPhrase + '"></div>') +
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
}

customElements.define('mympd-input-reset', inputReset);
