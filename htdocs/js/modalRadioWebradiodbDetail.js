"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalRadioWebradiodbDetail_js */

/**
 * Shows the details of a webradioDB entry
 * @param {string} uri webradio uri
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function showWebradiodbDetails(uri) {
    //reuse the radiobrowser modal
    const tbody = elGetById('modalRadiobrowserDetailsList');
    elClearId('modalRadiobrowserDetailsList');
    const m3u = isStreamUri(uri)
        ? streamUriToName(uri) + '.m3u'
        : uri;
    const result = webradioDb.webradios[m3u];
    if (result.Image !== '') {
        elGetById('RadiobrowserDetailsImage').style.backgroundImage = getCssImageUri(webradioDbPicsUri + result.Image);
    }
    else {
        elGetById('RadiobrowserDetailsImage').style.backgroundImage =
            'url("' + subdir + '/assets/coverimage-notavailable")';
    }
    elGetById('RadiobrowserDetailsTitle').textContent = result.Name;
    setDataId('RadiobrowserDetailsTitle', 'webradio', result);
    const showFields = [
        'StreamUri',
        'Homepage',
        'Genre',
        'Country',
        'Language',
        'Codec',
        'Bitrate',
        'Description'
    ];
    for (const field of showFields) {
        const value = printValue(field, result[field]);
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, field),
                elCreateNode('td', {}, value)
            ])
        );
    }
    const alternateStreams = Object.keys(result.alternativeStreams);
    if (alternateStreams.length > 0) {
        const td = elCreateEmpty('td', {});
        for (const name of alternateStreams) {
            const p = elCreateTextTn('p', {"class": ["pb-0"]}, 'Webradioformat',
                {"codec": result.alternativeStreams[name].Codec, "bitrate": result.alternativeStreams[name].Bitrate});
            const btn = elCreateText('button', {"class": ["btn", "btn-sm", "btn-secondary", "mi", "mi-sm", "ms-2"]}, 'favorite');
            p.appendChild(btn);
            td.appendChild(p);
            btn.addEventListener('click', function(event) {
                event.preventDefault();
                showEditRadioFavorite({
                    "Name": result.Name,
                    "StreamUri": result.alternativeStreams[name].StreamUri,
                    "Genre": result.Genre,
                    "Homepage": result.Homepage,
                    "Country": result.Country,
                    "Language": result.Languages,
                    "Codec": result.alternativeStreams[name].Codec,
                    "Bitrate": result.alternativeStreams[name].Bitrate,
                    "Description": result.Description,
                    "Image": result.Image
                });
            }, false);
        }
        tbody.appendChild(
            elCreateNodes('tr', {}, [
                elCreateTextTn('th', {}, 'Alternative streams'),
                td
            ])
        );
    }
    uiElements.modalRadiobrowserDetails.show();
}
