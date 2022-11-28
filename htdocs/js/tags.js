"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module tags_js */

/**
 * Marks a tag from a tag dropdown as active and sets the element with descId to its phrase
 * @param {string} containerId container id (dropdown)
 * @param {string} descId id of the descriptive element
 * @param {string} setTo tag to select
 */
 function selectTag(containerId, descId, setTo) {
    const btns = document.getElementById(containerId);
    let aBtn = btns.querySelector('.active');
    if (aBtn !== null) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn !== null) {
        aBtn.classList.add('active');
        if (descId !== undefined) {
            const descEl = document.getElementById(descId);
            if (descEl !== null) {
                descEl.textContent = aBtn.textContent;
                descEl.setAttribute('data-phrase', aBtn.getAttribute('data-phrase'));
            }
        }
    }
}

/**
 * Populates a container with buttons for tags
 * @param {string} elId id of the element to populate
 * @param {string} list name of the taglist
 */
function addTagList(elId, list) {
    const stack = elCreateEmpty('div', {"class": ["d-grid", "gap-2"]});
    if (list === 'tagListSearch') {
        if (features.featTags === true) {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, 'Any Tag')
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "filename"}, 'Filename')
        );
    }
    if (elId === 'searchDatabaseAlbumListTags') {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, 'Any Tag')
        );
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": settings[list][i]}, settings[list][i])
        );
    }
    if (elId === 'BrowseNavFilesystemDropdown' ||
        elId === 'BrowseNavPlaylistDropdown' ||
        elId === 'BrowseNavRadioFavoritesDropdown' ||
        elId === 'BrowseNavWebradiodbDropdown' ||
        elId === 'BrowseNavRadiobrowserDropdown')
    {
        if (features.featTags === true) {
            elClear(stack);
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Database"}, 'Database')
            );
        }
    }
    if (elId === 'BrowseDatabaseAlbumListTagDropdown' ||
        elId === 'BrowseDatabaseTagListTagDropdown' ||
        elId === 'BrowseNavFilesystemDropdown' ||
        elId === 'BrowseNavPlaylistDropdown' ||
        elId === 'BrowseNavRadioFavoritesDropdown' ||
        elId === 'BrowseNavWebradiodbDropdown' ||
        elId === 'BrowseNavRadiobrowserDropdown')
    {
        if (elId === 'BrowseDatabaseAlbumListTagDropdown' ||
            elId === 'BrowseDatabaseTagListTagDropdown')
        {
            stack.appendChild(
                elCreateEmpty('div', {"class": ["dropdown-divider"]})
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Playlist"}, 'Playlists')
        );
        if (elId === 'BrowseNavPlaylistDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Filesystem"}, 'Filesystem')
        );
        if (elId === 'BrowseNavFilesystemDropdown') {
            stack.lastChild.classList.add('active');
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Radio"}, 'Webradios')
        );
        if (elId === 'BrowseNavRadioFavoritesDropdown' ||
            elId === 'BrowseNavWebradiodbDropdown' ||
            elId === 'BrowseNavRadiobrowserDropdown')
        {
            stack.lastChild.classList.add('active');
        }
    }
    else if (elId === 'databaseAlbumListSortTagsList') {
        if (settings.tagList.includes('Date') === true &&
            settings[list].includes('Date') === false)
        {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Date"}, 'Date')
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "LastModified"}, 'Last modified')
        );
    }
    else if (elId === 'searchQueueTags') {
        if (features.featAdvqueue === true)
        {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "prio"}, 'Priority')
            );
        }
    }
    const el = document.getElementById(elId);
    elReplaceChild(el, stack);
}

/**
 * Populates a select element with options for tags
 * @param {string} elId id of the select to populate
 * @param {string} list name of the taglist
 */
function addTagListSelect(elId, list) {
    const select = document.getElementById(elId);
    elClear(select);
    if (elId === 'saveSmartPlaylistSort' ||
        elId === 'selectSmartplsSort')
    {
        select.appendChild(
            elCreateTextTn('option', {"value": ""}, 'Disabled')
        );
        select.appendChild(
            elCreateTextTn('option', {"value": "shuffle"}, 'Shuffle')
        );
        const optGroup = elCreateEmpty('optgroup', {"label": tn('Sort by tag'), "data-label-phrase": "Sort by tag"});
        optGroup.appendChild(
            elCreateTextTn('option', {"value": "filename"}, 'Filename')
        );
        for (let i = 0, j = settings[list].length; i < j; i++) {
            optGroup.appendChild(
                elCreateTextTn('option', {"value": settings[list][i]}, settings[list][i])
            );
        }
        select.appendChild(optGroup);
    }
    else if (elId === 'selectJukeboxUniqueTag' &&
        settings.tagListBrowse.includes('Title') === false)
    {
        //Title tag should be always in the list
        select.appendChild(
            elCreateTextTn('option', {"value": "Title"}, 'Song')
        );
        for (let i = 0, j = settings[list].length; i < j; i++) {
            select.appendChild(
                elCreateTextTn('option', {"value": settings[list][i]}, settings[list][i])
            );
        }
    }
}

/**
 * Parses the bits to the bitrate
 * @param {number} bits bits to parse
 * @returns {string} bitrate as string
 */
function parseBits(bits) {
    switch(bits) {
        case 224: return tn('32 bit floating');
        case 225: return tn('DSD');
        default:  return bits + ' ' + tn('bit');
    }
}

/**
 * Returns a tag value as dom element
 * @param {string | object} key the tag type
 * @param {*} value the tag value
 * @returns {Node} the created node
 */
function printValue(key, value) {
    if (value === undefined || value === null || value === '') {
        return document.createTextNode('-');
    }
    switch(key) {
        case 'Type':
            switch(value) {
                case 'song':     return elCreateText('span', {"class": ["mi"]}, 'music_note');
                case 'smartpls': return elCreateText('span', {"class": ["mi"]}, 'queue_music');
                case 'plist':    return elCreateText('span', {"class": ["mi"]}, 'list');
                case 'dir':      return elCreateText('span', {"class": ["mi"]}, 'folder_open');
                case 'stream':	 return elCreateText('span', {"class": ["mi"]}, 'stream');
                case 'webradio': return elCreateText('span', {"class": ["mi"]}, 'radio');
                default:         return elCreateText('span', {"class": ["mi"]}, 'radio_button_unchecked');
            }
        case 'Duration':
            return document.createTextNode(fmtSongDuration(value));
        case 'AudioFormat':
            return document.createTextNode(parseBits(value.bits) + smallSpace + nDash + smallSpace + value.sampleRate / 1000 + tn('kHz'));
        case 'Pos':
            //mpd is 0-indexed but humans wants 1-indexed lists
            return document.createTextNode(value + 1);
        case 'LastModified':
        case 'LastPlayed':
        case 'stickerLastPlayed':
        case 'stickerLastSkipped':
            return document.createTextNode(value === 0 ? tn('never') : fmtDate(value));
        case 'stickerLike':
            return elCreateText('span', {"class": ["mi"]},
                value === 0 ? 'thumb_down' : value === 1 ? 'radio_button_unchecked' : 'thumb_up');
        case 'stickerElapsed':
            return document.createTextNode(fmtSongDuration(value));
        case 'Artist':
        case 'ArtistSort':
        case 'AlbumArtist':
        case 'AlbumArtistSort':
        case 'Composer':
        case 'ComposerSort':
        case 'Performer':
        case 'Conductor':
        case 'Ensemble':
        case 'MUSICBRAINZ_ARTISTID':
        case 'MUSICBRAINZ_ALBUMARTISTID': {
            //multi value tags - print one line per value
            const span = elCreateEmpty('span', {});
            for (let i = 0, j = value.length; i < j; i++) {
                if (i > 0) {
                    span.appendChild(
                        elCreateEmpty('br', {})
                    );
                }
                if (key.indexOf('MUSICBRAINZ') === 0) {
                    span.appendChild(
                        getMBtagLink(key, value[i])
                    );
                }
                else {
                    span.appendChild(
                        document.createTextNode(value[i])
                    );
                }
            }
            return span;
        }
        case 'Genre':
            //multi value tags - print comma separated
            if (typeof value === 'string') {
                return document.createTextNode(value);
            }
            return document.createTextNode(
                value.join(', ')
            );
        case 'tags':
            //radiobrowser.info
            return document.createTextNode(
                value.replace(/,(\S)/g, ', $1')
            );
        case 'homepage':
        case 'Homepage':
            //webradios
            if (value === '') {
                return document.createTextNode(value);
            }
            return elCreateText('a', {"class": ["text-success", "external"],
                "href": value, "rel": "noreferrer", "target": "_blank"}, value);
        case 'lastcheckok':
            //radiobrowser.info
            return elCreateText('span', {"class": ["mi"]},
                (value === 1 ? 'check_circle' : 'error')
            );
        case 'Bitrate':
            return document.createTextNode(value + ' ' + tn('kbit'));
        case 'SongCount':
            return document.createTextNode(tn('Num songs', {"smartCount": value}));
        case 'Discs':
            return document.createTextNode(tn('Num discs', {"smartCount": value}));
        default:
            if (key.indexOf('MUSICBRAINZ') === 0) {
                return getMBtagLink(key, value);
            }
            else {
                return document.createTextNode(value);
            }
    }
}

/**
 * Checks if tag matches the value
 * @param {string | object} tag tag to check
 * @param {string} value value to check
 * @returns {boolean} true if tag matches value, else false
 */
function checkTagValue(tag, value) {
    if (typeof tag === 'string') {
        return tag === value;
    }
    return tag[0] === value;
}

/**
 * Returns a link to MusicBrainz
 * @param {string} tag tag name
 * @param {string} value tag value
 * @returns {HTMLElement} a link or the value as text
 */
function getMBtagLink(tag, value) {
    let MBentity = '';
    switch (tag) {
        case 'MUSICBRAINZ_ALBUMARTISTID':
        case 'MUSICBRAINZ_ARTISTID':
            MBentity = 'artist';
            break;
        case 'MUSICBRAINZ_ALBUMID':
            MBentity = 'release';
            break;
        case 'MUSICBRAINZ_RELEASETRACKID':
            MBentity = 'track';
            break;
        case 'MUSICBRAINZ_TRACKID':
            MBentity = 'recording';
            break;
    }
    if (MBentity === '' ||
        value === '-')
    {
        return elCreateText('span', {}, value);
    }
    else {
        return elCreateText('a', {"data-title-phrase": "Lookup at musicbrainz",
            "class": ["text-success", "external"], "target": "_musicbrainz",
            "href": "https://musicbrainz.org/" + MBentity + "/" + myEncodeURI(value)}, value);
    }
}

/**
 * 
 * @param {object} songObj mpd song object
 * @param {boolean} showArtists true=show artists, false=show albumartists
 * @returns {HTMLElement} dom node with musicbrainz links
 */
function addMusicbrainzFields(songObj, showArtists) {
    if (settings.webuiSettings.cloudMusicbrainz === false) {
        return null;
    }

    const artist = showArtists === false ? 'MUSICBRAINZ_ALBUMARTISTID' : 'MUSICBRAINZ_ARTISTID';
    if (songObj.MUSICBRAINZ_ALBUMID !== '-' ||
        (songObj[artist] !== undefined && checkTagValue(songObj[artist], '-') === false))
    {
        const mbField = elCreateNode('div', {"class": ["col-xl-6"]},
            elCreateTextTn('small', {}, 'MusicBrainz')
        );
        if (songObj.MUSICBRAINZ_ALBUMID !== '-') {
            const albumLink = getMBtagLink('MUSICBRAINZ_ALBUMID', songObj.MUSICBRAINZ_ALBUMID);
            albumLink.textContent = tn('Goto album');
            mbField.appendChild(
                elCreateNode('p', {"class": ["mb-1"]}, albumLink)
            );
        }
        if (songObj[artist] !== undefined &&
            checkTagValue(songObj[artist], '-') === false)
        {
            for (let i = 0, j = songObj[artist].length; i < j; i++) {
                const artistLink = getMBtagLink(artist, songObj[artist][i]);
                artistLink.textContent = songObj.AlbumArtist[i];
                mbField.appendChild(
                    elCreateNode('p', {"class": ["mb-1"]}, artistLink)
                );
            }
        }
        return mbField;
    }
    return null;
}
