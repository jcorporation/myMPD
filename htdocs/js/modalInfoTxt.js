"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalInfoTxt_js */

/**
 * Shows the song details modal
 * @param {EventTarget} target triggering event target
 * @returns {void}
 */
function showInfoTxt(target) {
    let uri = getData(target, 'uri');
    if (uri === undefined) {
        uri = getData(target.parentNode, 'uri');
    }
    httpGet(uri, parseInfoTxt, false);
}

/**
 * Parses the info txt
 * @param {string} text info text
 * @returns {void}
 */
function parseInfoTxt(text) {
    const infoTxtEl = elGetById('modalInfoTxtText');
    elClear(infoTxtEl);
    // simple markdown parser
    try {
        const lines = text.replace(/\r/g, '').split('\n');
        for (let i = 0, j = lines.length; i < j; i++) {
            const line = lines[i];
            // headings
            const title = line.match(/^(#+)\s+(.*)/);
            if (title) {
                const h = title[1].length;
                infoTxtEl.appendChild(
                    elCreateText('h' + h.toString(), {}, title[2])
                );
                continue;
            }
            // unordered list
            const ul = line.match(/^- (.*)$/);
            if (ul) {
                const ulEl = elCreateEmpty('ul', {});
                while (i < j && lines[i].match(/^- (.*)$/) !== null) {
                    const li = lines[i].match(/^- (.*)$/);
                    ulEl.appendChild(
                        elCreateText('li', {}, li[1])
                    );
                    i++;
                }
                infoTxtEl.appendChild(ulEl);
                continue;
            }
            // ordered list
            const ol = line.match(/^\d+\. (.*)$/);
            if (ol) {
                const ulEl = elCreateEmpty('ol', {});
                while (i < j && lines[i].match(/^\d+\. (.*)$/) !== null) {
                    const li = lines[i].match(/^\d+\. (.*)$/);
                    ulEl.appendChild(
                        elCreateText('li', {}, li[1])
                    );
                    i++;
                }
                infoTxtEl.appendChild(ulEl);
                continue;
            }
            // empty lines
            if (line.match(/^\s*$/)) {
                continue;
            }
            // text lines
            const l = [];
            while (i < j && lines[i].match(/^\s*$/) === null) {
                l.push(lines[i]);
                i++;
            }
            infoTxtEl.appendChild(
                elCreateText('p', {}, l.join(' '))
            );
        }
    }
    catch(error) {
        infoTxtEl.appendChild(
            elCreateTextTn('div', {"class": ["alert", "alert-warning"]}, 'Markdown could not be parsed.')
        );
    }
    uiElements.modalInfoTxt.show();
}
