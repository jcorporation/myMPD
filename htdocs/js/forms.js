"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module forms_js */

/**
 * Populates the settings json from input elements
 * @param {string} prefix for element ids
 * @param {object} settingsParams settings object to populate
 * @param {object} defaultFields settingsFields object
 * @returns {boolean} true on success, else false
 */
function formToJson(prefix, settingsParams, defaultFields) {
    for (const key in defaultFields) {
        if (defaultFields[key].inputType === 'none') {
            continue;
        }
        const id = prefix + ucFirst(key) + 'Input';
        const el = document.getElementById(id);
        if (el) {
            const value = defaultFields[key].inputType === 'select'
                ? getSelectValue(el)
                : defaultFields[key].inputType === 'mympd-select-search'
                    ? getData(el, 'value')
                    : defaultFields[key].inputType === 'checkbox'
                        ? getBtnChkValue(el)
                        : el.value;
            if (defaultFields[key].validate !== undefined) {
                const func = getFunctionByName(defaultFields[key].validate.cmd);
                if (func(el, ... defaultFields[key].validate.options) === false) {
                    return false;
                }
            }
            settingsParams[key] = defaultFields[key].contentType === 'number'
                ? parseNumber(value)
                : value;
        }
        else {
            logError('Element not found: ' + id);
            return false;
        }
    }
    return true;
}

/**
 * Creates form fields
 * @param {object} defaultFields object with elements to create and the default values
 * @param {string} prefix prefix for element ids
 * @param {object} forms cache for the form field containers
 * @returns {void}
 */
function createForm(defaultFields, prefix, forms) {
    // iterate through sorted keys
    const settingsKeys = Object.keys(defaultFields);
    settingsKeys.sort(function(a, b) {
        return defaultFields[a].sort - defaultFields[b].sort;
    });
    for (let i = 0, j = settingsKeys.length; i < j; i++) {
        const key = settingsKeys[i];
        // check if we should add a field
        if (defaultFields[key].form === undefined ||
            defaultFields[key].inputType === 'none')
        {
            continue;
        }
        // calculate a camelCase id
        const id = prefix + ucFirst(key) + 'Input';
        // get the container
        const form = defaultFields[key].form;
        if (forms[form] === undefined) {
            forms[form] = document.getElementById(form);
            // clear the container if it was not cached
            elClear(forms[form]);
        }
        // create the form field
        const col = elCreateNode('div', {"class": ["col-sm-8", "position-relative"]},
            elCreateNode('div', {"class": ["input-group"]},
                elCreateEmpty('div', {"class": ["flex-grow-1", "position-relative"]})
            )
        );
        if (defaultFields[key].inputType === 'select') {
            // simple select
            const select = elCreateEmpty('select', {"class": ["form-select"], "id": id});
            for (const value in defaultFields[key].validValues) {
                select.appendChild(
                    elCreateTextTn('option', {"value": value}, defaultFields[key].validValues[value])
                );
            }
            col.firstChild.firstChild.appendChild(select);
        }
        else if (defaultFields[key].inputType === 'mympd-select-search') {
            // searchable select
            const input = elCreateEmpty('input', {"class": ["form-select"], "id": id});
            setData(input, 'cb-filter', defaultFields[key].cbCallback);
            setData(input, 'cb-filter-options', [id + 'Input']);
            input.setAttribute('data-is', 'mympd-select-search');
            col.firstChild.firstChild.appendChild(
                elCreateNode('div', {"class": ["btn-group", "d-flex"]}, input)
            );
        }
        else if (defaultFields[key].inputType === 'checkbox') {
            // checkbox
            const btn = elCreateEmpty('button', {"type": "button", "id": id, "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]});
            if (defaultFields[key].onClick !== undefined) {
                // custom click handler
                btn.addEventListener('click', function(event) {
                    // @ts-ignore
                    window[defaultFields[key].onClick](event);
                }, false);
            }
            else {
                // default click handler
                btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target, undefined);
                }, false);
            }
            col.firstChild.firstChild.appendChild(btn);
        }
        else if (defaultFields[key].inputType === 'password') {
            // password field
            col.firstChild.firstChild.appendChild(
                elCreateEmpty('input', {"is": "mympd-input-password", "id": id,
                    "value": settingsFields[key], "class": ["form-control"], "type": "password"})
            );
        }
        else {
            // text and color inputs with reset to default button
            const inputType = defaultFields[key].inputType === 'color'
                ? 'color'
                : 'text';
            const placeholder = defaultFields[key].placeholder !== undefined
                ? defaultFields[key].placeholder
                : defaultFields[key].defaultValue;
            col.firstChild.firstChild.appendChild(
                elCreateEmpty('input', {"is": "mympd-input-reset", "id": id, "data-default": defaultFields[key].defaultValue,
                    "placeholder": placeholder, "value": "", "class": ["form-control"], "type": inputType})
            );
        }
        // unit
        if (defaultFields[key].unit !== undefined) {
            col.firstChild.appendChild(
                elCreateTextTn('span', {"class": ["input-group-text-nobg"]}, defaultFields[key].unit)
            );
        }
        // invalid feedback element for local validation
        if (defaultFields[key].invalid !== undefined) {
            col.firstChild.appendChild(
                elCreateTextTn('div', {"class": ["invalid-feedback"]}, defaultFields[key].invalid)
            );
        }
        // warning element
        if (defaultFields[key].warn !== undefined) {
            col.appendChild(
                elCreateTextTn('div', {"id": id + 'Warn', "class": ["mt-2", "mb-1", "alert", "alert-warning", "d-none"]}, defaultFields[key].warn)
            );
        }
        // help text
        if (defaultFields[key].help !== undefined) {
            col.appendChild(
                elCreateTextTn('small', {"class": ["help"]}, defaultFields[key].help)
            );
        }
        // create the label
        const label = defaultFields[key].hintIcon === undefined
            ? elCreateTextTn('label', {"class": ["col-sm-4", "col-form-label"], "for": id}, defaultFields[key].title)
            : elCreateNodes('label', {"class": ["col-sm-4", "col-form-label"], "for": id}, [
                    elCreateTextTn('span', {}, defaultFields[key].title),
                    elCreateText('small', {"class": ["mi", "mi-sm", "ms-1"], "title": tn(defaultFields[key].hintText),
                        "data-title-phrase": defaultFields[key].hintText}, defaultFields[key].hintIcon)
              ]);
        // create the row and append it to the form field container
        let rowClasses = ["mb-3", "row"];
        if (defaultFields[key].cssClass !== undefined) {
            rowClasses = rowClasses.concat(defaultFields[key].cssClass);
        }
        forms[form].appendChild(
            elCreateNodes('div', {"class": rowClasses}, [
                label,
                col
            ])
        );
    }

    // add event handler
    for (const key in defaultFields) {
        if (defaultFields[key].onChange !== undefined) {
            const id = prefix + ucFirst(key) + 'Input';
            document.getElementById(id).addEventListener('change', function(event) {
                // @ts-ignore
                window[defaultFields[key].onChange](event);
            }, false);
        }
    }
}

/**
 * Populates the form with values
 * @param {object} settingsFields object with the values for the elements
 * @param {object} defaultFields object with elements to populate
 * @param {string} prefix prefix for element ids
 * @returns {void}
 */
function jsonToForm(settingsFields, defaultFields, prefix) {
    // iterate through keys
    const settingsKeys = Object.keys(defaultFields);
    for (let i = 0, j = settingsKeys.length; i < j; i++) {
        const key = settingsKeys[i];
        // check if we should add a field
        if (defaultFields[key].form === undefined ||
            defaultFields[key].inputType === 'none')
        {
            continue;
        }
        // calculate a camelCase id
        const id = prefix + ucFirst(key) + 'Input';
        const field = document.getElementById(id);
        if (field) {
            switch(defaultFields[key].inputType) {
                case 'checkbox':
                    toggleBtnChk(field, settingsFields[key]);
                    break;
                case 'color':
                case 'password':
                case 'select':
                case 'text':
                    field.value = settingsFields[key];
                    break;
                case 'mympd-select-search':
                    setData(field, 'value', settingsFields[key]);
                    field.value = tn(settingsFields[key]);
                    break;
                default:
                    logError('Unhandled field type ' + defaultFields[key].inputType + ' for id ' + id + ' not found.');
            }
        }
        else {
            logError('Form field with id ' + id + 'not found.');
        }
    }
}
