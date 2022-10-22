"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module Browse_js */

/**
 * Initialization function for the browse view
 */
function initBrowse() {
    for (const nav of ['BrowseDatabaseByTagDropdown', 'BrowseNavPlaylistsDropdown', 'BrowseNavFilesystemDropdown',
        'BrowseNavWebradiodbDropdown','BrowseNavRadiobrowserDropdown', 'BrowseNavRadioFavoritesDropdown'])
    {
        document.getElementById(nav).addEventListener('click', function(event) {
            navBrowseHandler(event);
        }, false);
    }
}

/**
 * Event handler for the navigation dropdown in the browse views
 * @param {event} event triggering event
 */
function navBrowseHandler(event) {
    if (event.target.nodeName === 'BUTTON') {
        const tag = getData(event.target, 'tag');
        if (tag === 'Playlists' ||
            tag === 'Filesystem' ||
            tag === 'Radio')
        {
            appGoto('Browse', tag, undefined);
            return;
        }

        if (app.current.card === 'Browse' &&
            app.current.tab !== 'Database')
        {
            appGoto('Browse', 'Database', app.cards.Browse.tabs.Database.active);
            return;
        }
        if (tag !== 'Album') {
            app.current.filter = tag;
            app.current.sort.tag = tag;
            app.current.sort.desc = false;
        }
        else {
            app.current.sort = {
                "tag": tagAlbumArtist,
                "desc": false
            };
        }
        app.current.search = '';
        document.getElementById('searchDatabaseMatch').value = 'contains';
        appGoto(app.current.card, app.current.tab, app.current.view,
            0, app.current.limit, app.current.filter, app.current.sort, tag, app.current.search);
    }
}

/**
 * Event handler for links to browse views
 * @param {event} event triggering event
 */
function gotoBrowse(event) {
    let target = event.target;
    let tag = getData(target, 'tag');
    let name = getData(target, 'name');
    let i = 0;
    while (tag === undefined) {
        i++;
        target = target.parentNode;
        tag = getData(target, 'tag');
        name = getData(target, 'name');
        if (i > 2) {
            break;
        }
    }
    if (tag !== '' &&
        name !== '' &&
        name !== '-' &&
        settings.tagListBrowse.includes(tag))
    {
        if (tag === 'Album') {
            let artist = getData(target, 'AlbumArtist');
            if (artist === undefined) {
                artist = getData(target.parentNode, 'AlbumArtist');
            }
            if (artist !== null) {
                //Show album details
                appGoto('Browse', 'Database', 'Detail', 0, undefined, tag, tagAlbumArtist, name, artist);
            }
            else {
                //show filtered album list
                gotoAlbumList(tag, name);
            }
        }
        else {
            //show filtered album list
            gotoAlbumList(tag, name);
        }
    }
}

/**
 * Go's to the album detail view
 * @param {Array} artist albumartist names
 * @param {string} album album name
 */
//eslint-disable-next-line no-unused-vars
function gotoAlbum(artist, album) {
    appGoto('Browse', 'Database', 'Detail', 0, undefined, 'Album', tagAlbumArtist, album, artist);
}

/**
 * Go's to a filtered album list
 * @param {string} tag tag to search
 * @param {Array} value array of values to match
 */
//eslint-disable-next-line no-unused-vars
function gotoAlbumList(tag, value) {
    if (typeof value === 'string') {
        //convert string to array
        value = [value];
    }
    document.getElementById('searchDatabaseStr').value = '';
    let expression = '(';
    for (let i = 0, j = value.length; i < j; i++) {
        if (i > 0) {
            expression += ' AND '
        }
        expression += '(' + tag + ' == \'' + escapeMPD(value[i]) + '\')';
    }
    expression += ')';
    appGoto('Browse', 'Database', 'List', 0, undefined, tag, {"tag": tagAlbumArtist, "desc": false}, 'Album', expression);
}

/**
 * Go's to the filesystem view
 * @param {string} uri uri to list
 * @param {*} type "dir" or "plist"
 */
//eslint-disable-next-line no-unused-vars
function gotoFilesystem(uri, type) {
    document.getElementById('searchFilesystemStr').value = '';
    appGoto('Browse', 'Filesystem', undefined, 0, undefined, '-', '-', type, uri);
}

/**
 * Callback function for intersection observer to lazy load cover images
 * @param {object} changes IntersectionObserverEntry objects
 * @param {object} observer IntersectionObserver
 */
 function setGridImage(changes, observer) {
    changes.forEach(change => {
        if (change.intersectionRatio > 0) {
            observer.unobserve(change.target);
            const uri = getData(change.target.firstChild, 'image');
            const body = change.target.firstChild.querySelector('.card-body');
            if (body) {
                body.style.backgroundImage = getCssImageUri(uri);
            }
        }
    });
}
