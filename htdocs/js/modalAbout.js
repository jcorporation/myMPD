"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalAbout_js */

/**
 * Initialization function for the about modal
 * @returns {void}
 */
function initModalAbout() {
    const tab = document.getElementById('modalAboutTabShortcuts');
    elClear(tab);
    const keys = Object.keys(keymap).sort((a, b) => {
        return keymap[a].order - keymap[b].order;
    });
    for (const key of keys) {
        if (keymap[key].cmd === undefined) {
            tab.appendChild(
                elCreateNode('div', {"class": ["row", "mb-2", "mt-3"]},
                    elCreateNode('div', {"class": ["col-12"]},
                        elCreateTextTn('h5', {}, keymap[key].desc)
                    )
                )
            );
            tab.appendChild(
                elCreateEmpty('div', {"class": ["row"]})
            );
            continue;
        }
        const col = elCreateEmpty('div', {"class": ["col", "col-6", "mb-3", "align-items-center"]});
        if (keymap[key].feature !== undefined) {
            col.classList.add(keymap[key].feature);
        }
        const k = elCreateText('div', {"class": ["key", "float-start"]}, (keymap[key].key !== undefined ? keymap[key].key : key));
        if (keymap[key].key && keymap[key].key.length > 1) {
            k.classList.add('mi', 'mi-sm');
        }
        col.appendChild(k);
        col.appendChild(
            elCreateTextTn('div', {}, keymap[key].desc)
        );
        tab.lastChild.appendChild(col);
    }

    document.getElementById('modalAbout').addEventListener('show.bs.modal', function () {
        sendAPI("MYMPD_API_STATS", {}, parseStats, false);
        getServerinfo();
    }, false);
}

/**
 * Parses the MYMPD_API_STATS jsonrpc response
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function parseStats(obj) {
    document.getElementById('modalAboutArtistCount').textContent = obj.result.artists;
    document.getElementById('modalAboutAlbumCount').textContent = obj.result.albums;
    document.getElementById('modalAboutSongCount').textContent = obj.result.songs;
    document.getElementById('modalAboutDbPlaytime').textContent = fmtDuration(obj.result.dbPlaytime);
    document.getElementById('modalAboutPlaytime').textContent = fmtDuration(obj.result.playtime);
    document.getElementById('modalAboutUptime').textContent = fmtDuration(obj.result.uptime);
    document.getElementById('modalAboutMympdUptime').textContent = fmtDuration(obj.result.myMPDuptime);
    document.getElementById('modalAboutDbUpdated').textContent = fmtDate(obj.result.dbUpdated);
    document.getElementById('modalAboutMympdVersion').textContent = obj.result.mympdVersion;
    document.getElementById('modalAboutMympdUri').textContent = obj.result.myMPDuri;

    const mpdInfoVersionEl = document.getElementById('modalAboutProtocolVersion');
    elClear(mpdInfoVersionEl);
    mpdInfoVersionEl.appendChild(document.createTextNode(obj.result.mpdProtocolVersion));

    const mpdProtocolVersion = obj.result.mpdProtocolVersion.match(/(\d+)\.(\d+)\.(\d+)/);
    if ((mpdProtocolVersion[1] < mpdVersion.major) ||
        (mpdProtocolVersion[1] <= mpdVersion.major && mpdProtocolVersion[2] < mpdVersion.minor) ||
        (mpdProtocolVersion[1] <= mpdVersion.major && mpdProtocolVersion[2] <= mpdVersion.minor && mpdProtocolVersion[3] < mpdVersion.patch)
       )
    {
        mpdInfoVersionEl.appendChild(
            elCreateTextTn('div', {"class": ["alert", "alert-warning", "mt-2", "mb-1"]}, 'MPD version is outdated')
        );
    }
}

/**
 * Gets the serverinfo (ip address)
 * @returns {void}
 */
function getServerinfo() {
    httpGet(subdir + '/serverinfo', function(obj) {
        document.getElementById('modalAboutWebserverIP').textContent = obj.result.ip;
    }, true);
}
