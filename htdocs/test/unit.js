"use strict";
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

const results = document.getElementById('results');

results.appendChild(
    elCreateText('h4', {}, 'Locale')
);
let locale = '';
const ts = getTimestamp();
for (const key in i18n) {
    locale = key;
    if (locale !== 'default') {
        results.appendChild(
            elCreateText('p', {}, key + ': ' + fmtDate(ts))
        );
    }
}
