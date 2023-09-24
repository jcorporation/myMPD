"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module tags_js */

/**
 * Marks a tag from a tag dropdown as active and sets the element with descId to its phrase
 * @param {string} containerId container id (dropdown)
 * @param {string} descId id of the descriptive element
 * @param {string} setTo tag to select
 * @returns {void}
 */
 function selectTag(containerId, descId, setTo) {
    const btns = elGetById(containerId);
    let aBtn = btns.querySelector('.active');
    if (aBtn !== null) {
        aBtn.classList.remove('active');
    }
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn !== null) {
        aBtn.classList.add('active');
        if (descId !== undefined) {
            const descEl = elGetById(descId);
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
 * @returns {void}
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
    if (elId === 'BrowseDatabaseAlbumListSearchTags') {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "any"}, 'Any Tag')
        );
    }
    for (let i = 0, j = settings[list].length; i < j; i++) {
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": settings[list][i]}, settings[list][i])
        );
    }
    if (elId === 'BrowseFilesystemNavDropdown' ||
        elId === 'BrowsePlaylistListNavDropdown' ||
        elId === 'BrowseRadioFavoritesNavDropdown' ||
        elId === 'BrowseRadioWebradiodbNavDropdown' ||
        elId === 'BrowseRadioRadiobrowserNavDropdown')
    {
        elClear(stack);
        if (features.featAlbums === true) {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Database"}, 'Database')
            );
        }
    }
    if (elId === 'BrowseDatabaseAlbumListTagDropdown' ||
        elId === 'BrowseDatabaseTagListTagDropdown' ||
        elId === 'BrowseFilesystemNavDropdown' ||
        elId === 'BrowsePlaylistListNavDropdown' ||
        elId === 'BrowseRadioFavoritesNavDropdown' ||
        elId === 'BrowseRadioWebradiodbNavDropdown' ||
        elId === 'BrowseRadioRadiobrowserNavDropdown')
    {
        if (elId === 'BrowseDatabaseAlbumListTagDropdown' ||
            elId === 'BrowseDatabaseTagListTagDropdown')
        {
            stack.appendChild(
                elCreateEmpty('div', {"class": ["dropdown-divider"]})
            );
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Filesystem"}, 'Filesystem')
        );
        if (elId === 'BrowseFilesystemNavDropdown') {
            stack.lastChild.classList.add('active');
        }
        if (features.featPlaylists === true) {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Playlist"}, 'Playlists')
            );
            if (elId === 'BrowsePlaylistListNavDropdown') {
                stack.lastChild.classList.add('active');
            }
        }
        stack.appendChild(
            elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "Radio"}, 'Webradios')
        );
        if (elId === 'BrowseRadioFavoritesNavDropdown' ||
            elId === 'BrowseRadioWebradiodbNavDropdown' ||
            elId === 'BrowseRadioRadiobrowserNavDropdown')
        {
            stack.lastChild.classList.add('active');
        }
    }
    else if (elId === 'BrowseDatabaseAlbumListSortTagsList') {
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
    else if (elId === 'QueueCurrentSearchTags') {
        if (features.featAdvqueue === true)
        {
            stack.appendChild(
                elCreateTextTn('button', {"class": ["btn", "btn-secondary", "btn-sm"], "data-tag": "prio"}, 'Priority')
            );
        }
    }
    const el = elGetById(elId);
    elReplaceChild(el, stack);
}

/**
 * Populates a select element with options for tags
 * @param {string} elId id of the select to populate
 * @param {string} list name of the taglist
 * @returns {void}
 */
function addTagListSelect(elId, list) {
    const select = elGetById(elId);
    elClear(select);
    if (elId === 'modalSmartPlaylistEditSortInput' ||
        elId === 'modalSettingsSmartplsSortInput')
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
    else if (elId === 'modalPlaybackJukeboxUniqueTagInput' &&
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
 * Parses the bits to the bitrate from mpd audioformat
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
 * Parses the channels information from mpd audioformat
 * @param {number} channels number of channels
 * @returns {string} the parses number of channels as text
 */
function parseChannels(channels) {
    switch(channels) {
        case 0:  return '';
        case 1:  return tn('Mono');
        case 2:  return tn('Stereo');
        default: return channels.toString();
    }
}

/**
 * Combines name and title to display extm3u name
 * @param {string} name name tag
 * @param {string} title title tag
 * @returns {string} the title to display
 */
function getDisplayTitle(name, title) {
    if (title === name) {
        return title;
    }
    return isEmptyTag(name) === false
        ? name + ': ' + title
        : title;
}

/**
 * Returns a tag value as dom element
 * @param {string} key the tag type
 * @param {string | number | object} value the tag value
 * @returns {Node} the created node
 */
function printValue(key, value) {
    if (isEmptyTag(value) === true) {
        return document.createTextNode('');
    }
    switch(key) {
        case 'Type':
            switch(value) {
                case 'song':     return elCreateText('span', {"class": ["mi"]}, 'music_note');
                case 'smartpls': return elCreateText('span', {"class": ["mi"]}, 'queue_music');
                case 'plist':    return elCreateText('span', {"class": ["mi"]}, 'list');
                case 'dir':      return elCreateText('span', {"class": ["mi"]}, 'folder_open');
                case 'stream':   return elCreateText('span', {"class": ["mi"]}, 'stream');
                case 'webradio': return elCreateText('span', {"class": ["mi"]}, 'radio');
                default:         return elCreateText('span', {"class": ["mi"]}, 'radio_button_unchecked');
            }
        case 'Duration':
            return document.createTextNode(fmtSongDuration(value));
        case 'AudioFormat': {
            const text = [];
            text.push(parseBits(value.bits));
            text.push(value.sampleRate / 1000 + tn('kHz'));
            const channels = parseChannels(value.channels);
            if (channels !== '') {
                text.push(channels);
            }
            return document.createTextNode(text.join(smallSpace + nDash + smallSpace));
        }
        case 'Pos':
            //mpd is 0-indexed but humans wants 1-indexed lists
            return document.createTextNode(value + 1);
        case 'LastModified':
        case 'LastPlayed':
        case 'lastPlayed':
        case 'lastSkipped':
            return document.createTextNode(value === 0 ? tn('never') : fmtDate(value));
        case 'like':
            return elCreateText('span', {"class": ["mi"]},
                value === 0
                    ? 'thumb_down'
                    : value === 1 
                        ? 'horizontal_rule'
                        : 'thumb_up'
            );
        case 'elapsed':
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
            if (typeof value === 'string') {
                return document.createTextNode(value);
            }
            //multi value tags - return comma separated
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
        case 'Languages':
            return document.createTextNode(
                value.join(', ')
            );
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
            if (value === 0) {
                return document.createTextNode('-');
            }
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
 * Checks if tag is empty
 * @param {string | Array} tagValue tag value to check
 * @returns {boolean} true if tag matches value, else false
 */
function isEmptyTag(tagValue) {
    return tagValue === undefined || tagValue === null || tagValue.length === 0;
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
        case 'MUSICBRAINZ_RELEASEGROUPID':
            MBentity = 'release-group';
            break;
        // No Default
    }
    if (isEmptyTag(value) === true) {
        return elCreateText('span', {}, '');
    }
    else if (MBentity === '') {
        return elCreateText('span', {}, value);
    }
    else {
        return elCreateText('a', {"data-title-phrase": "Lookup at musicbrainz",
            "class": ["text-success", "external"], "target": "_musicbrainz",
            "href": "https://musicbrainz.org/" + MBentity + "/" + myEncodeURI(value)}, value);
    }
}

/**
 * Returns Links to MusicBrainz artist und album
 * @param {object} songObj mpd song object
 * @param {boolean} showArtists true=show artists, false=show albumartists
 * @returns {HTMLElement} dom node with musicbrainz links
 */
function addMusicbrainzFields(songObj, showArtists) {
    if (settings.webuiSettings.cloudMusicbrainz === false) {
        return null;
    }

    const artist = showArtists === false
        ? 'MUSICBRAINZ_ALBUMARTISTID'
        : 'MUSICBRAINZ_ARTISTID';

    const mbField = elCreateNode('div', {"class": ["col-xl-6"]},
        elCreateTextTn('small', {}, 'MusicBrainz')
    );

    if (isEmptyTag(songObj.MUSICBRAINZ_RELEASEGROUPID) === false) {
        //use releasegroupid
        const albumLink = getMBtagLink('MUSICBRAINZ_RELEASEGROUPID', songObj.MUSICBRAINZ_RELEASEGROUPID);
        albumLink.textContent = tn('Goto album');
        mbField.appendChild(
            elCreateNode('p', {"class": ["mb-1"]}, albumLink)
        );
    }
    else if (isEmptyTag(songObj.MUSICBRAINZ_ALBUMID) === false) {
        //fallback to albumid
        const albumLink = getMBtagLink('MUSICBRAINZ_ALBUMID', songObj.MUSICBRAINZ_ALBUMID);
        albumLink.textContent = tn('Goto album');
        mbField.appendChild(
            elCreateNode('p', {"class": ["mb-1"]}, albumLink)
        );
    }
    if (isEmptyTag(songObj[artist]) === false) {
        //show albumartists or artists
        for (let i = 0, j = songObj[artist].length; i < j; i++) {
            const artistLink = getMBtagLink(artist, songObj[artist][i]);
            artistLink.textContent = artist === 'MUSICBRAINZ_ALBUMARTISTID'
                ? songObj.AlbumArtist[i]
                : songObj.Artist[i];
            if (artistLink.textContent === '') {
                // skip empty tag values
                // count of mbids and artists are not equal
                continue;
            }
            mbField.appendChild(
                elCreateNode('p', {"class": ["mb-1"]}, artistLink)
            );
        }
    }
    return mbField.childNodes.length > 1
        ? mbField
        : null;
}
