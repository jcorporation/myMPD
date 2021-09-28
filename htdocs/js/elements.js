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
        let inputType = this.getAttribute('type');
        inputType = inputType === null ? 'text' : inputType;

        const inputGroup = elCreate('div', {"class": ["input-group"]}, '');
        const input = elCreate('input', {
            "id": this.getAttribute('id'),
            "placeholder": placeholder === null ? '' : placeholder,
            "type": inputType,
            "class": ["form-control", "border-secondary"]
        }, '');
        if (this.classList.contains('alwaysEnabled')) {
            input.classList.add('alwaysEnabled');
        }
        inputGroup.appendChild(input);

        const resetButton = elCreate('button', {
            "class": ["btn", "btn-secondary", "rounded-end", "resetBtn"],
            "data-title-phrase": "Reset to default"
        }, '');
        resetButton.appendChild(elCreate('span', {"class": ["mi"]}, 'settings_backup_restore'));
        inputGroup.appendChild(resetButton);
        if (unitPhrase !== null) {
            const unitText = elCreate('div', {
                "class": ["input-group-text-nobg"],
                "data-phrase": unitPhrase
            }, '');
            inputGroup.appendChild(unitText);
        }
        if (invalidPhrase !== null) {
            const invalidText = elCreate('div', {
                "class": ["invalid-feedback"],
                "data-phrase": invalidPhrase
            }, '');
            inputGroup.appendChild(invalidText);
        }

        this.appendChild(inputGroup);

        this.input = input;
        this.resetButton = resetButton;
    }

    connectedCallback() {
        this.resetButton.addEventListener('click', function(event) {
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

class inputClear extends HTMLInputElement {
    constructor() {
        super();
        const button = elCreate('button', {"class": ["mi", "mi-small", "clear-button", "btn-secondary"]}, 'clear');
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

customElements.define('mympd-input-reset', inputReset);
customElements.define('mympd-input-clear', inputClear, {extends: 'input'});
