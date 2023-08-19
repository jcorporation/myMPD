"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module home_js */

/**
 * Adds a link to a view to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addViewToHome() {
    _addHomeIcon('appGoto', '', 'preview', '', [app.current.card, app.current.tab, app.current.view,
        app.current.offset, app.current.limit, app.current.filter, app.current.sort, app.current.tag, app.current.search]);
}

/**
 * Adds a modal shortcut to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addOpenModalToHome() {
    _addHomeIcon('openModal', '', 'web_asset', '', []);
}

/**
 * Adds a external link to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addExternalLinkToHome() {
    _addHomeIcon('openExternalLink', '', 'link', '', []);
}

/**
 * Adds a script to the homescreen
 * @param {string} name name for the home icon
 * @param {object} script script object
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addScriptToHome(name, script) {
    const options = [script.script, script.arguments.join(',')];
    _addHomeIcon('execScriptFromOptions', name, 'code', '', options);
}

/**
 * Adds a playlist to the homescreen
 * @param {string} uri uri of the playlist
 * @param {string} type one of plist, smartpls
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addPlistToHome(uri, type, name) {
    _addHomeIcon('replaceQueue', name, 'list', '', [type, uri]);
}

/**
 * Adds a webradio favorite to the homescreen
 * @param {string} uri uri of the webradio favorite
 * @param {string} type must be webradio
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addRadioFavoriteToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

/**
 * Adds a webradioDB entry to the homescreen
 * @param {string} uri uri of the stream
 * @param {string} type must be stream
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addWebRadiodbToHome(uri, type, name, image) {
    _addHomeIcon('replaceQueue', name, '', image, [type, uri]);
}

/**
 * Adds a directory to the homescreen
 * @param {string} uri directory uri
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addDirToHome(uri, name) {
    if(uri === undefined) {
        uri = app.current.filter;
        name = basename(app.current.filter, false);
    }
    _addHomeIcon('replaceQueue', name, 'folder_open', '', ['dir', uri]);
}

/**
 * Adds a song or stream to the homescreen
 * @param {string} uri song or stream uri
 * @param {string} type one of song, stream
 * @param {string} name name for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSongToHome(uri, type, name) {
    const ligature = type === 'song' ? 'music_note' : 'stream';
    _addHomeIcon('replaceQueue', name, ligature, '', [type, uri]);
}

/**
 * Adds the current search to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addSearchToHome() {
    _addHomeIcon('replaceQueue', tn('Current search'), 'saved_search', '', ['search', app.current.search]);
}

/**
 * Adds an album to the homescreen
 * @param {string} albumId the albumid
 * @param {string} name name for the home icon
 * @param {string} image image for the home icon
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addAlbumToHome(albumId, name, image) {
    if (image === '') {
        _addHomeIcon('replaceQueue', name, 'album', '', ['album', albumId]);
    }
    else {
        _addHomeIcon('replaceQueue', name, '', image, ['album', albumId]);
    }
}

/**
 * Adds a new stream to the homescreen
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function addStreamToHome() {
    const mode = getRadioBoxValueId('modalPlaylistAddToPos');
    const uri = elGetById('modalPlaylistAddToUrisInput').value;
    let action;
    switch(mode) {
        case 'append': action = 'appendQueue'; break;
        case 'appendPlay': action = 'appendPlayQueue'; break;
        case 'insertAfterCurrent': action = 'insertAfterCurrentQueue'; break;
        case 'insertPlayAfterCurrent': action = 'insertPlayAfterCurrentQueue'; break;
        case 'replace': action = 'replaceQueue'; break;
        case 'replacePlay': action = 'replacePlayQueue'; break;
        default: logError('Invalid mode: ' + mode);
    }
    _addHomeIcon(action, '', 'stream', '', ['stream', uri]);
}

/**
 * Opens the add to homescreen modal, this function is called by the add*ToHome functions above.
 * @param {string} cmd action
 * @param {string} name name for the home icon
 * @param {string} ligature ligature for the home icon
 * @param {string} image picture for the home icon
 * @param {object} options options array
 * @returns {void}
 */
function _addHomeIcon(cmd, name, ligature, image, options) {
    elGetById('modalHomeIconTitle').textContent = tn('Add to homescreen');
    elGetById('inputHomeIconReplace').value = 'false';
    elGetById('inputHomeIconOldpos').value = '0';
    elGetById('inputHomeIconName').value = name;
    elGetById('inputHomeIconBgcolor').value = '#28a745';
    elGetById('inputHomeIconColor').value = '#ffffff';

    populateHomeIconCmdSelect(cmd, options[0]);
    elGetById('selectHomeIconCmd').value = cmd;
    elClearId('divHomeIconOptions');
    showHomeIconCmdOptions(options);
    getHomeIconPictureList();
    const homeIconPreviewEl = elGetById('homeIconPreview');
    const homeIconImageInput = elGetById('inputHomeIconImage');
    if (image !== '') {
        homeIconImageInput.value = image;
        setData(homeIconImageInput, 'value', image);
        elGetById('inputHomeIconLigature').value = '';
        elClear(homeIconPreviewEl);
        homeIconPreviewEl.style.backgroundImage = getCssImageUri(image);
        elHideId('divHomeIconLigature');
    }
    else {
        //use ligature
        homeIconImageInput.value = tn('Use ligature');
        setData(homeIconImageInput, 'value', '');
        elGetById('inputHomeIconLigature').value = ligature;
        homeIconPreviewEl.textContent = ligature;
        homeIconPreviewEl.style.backgroundImage = '';
        elShowId('divHomeIconLigature');
    }

    homeIconPreviewEl.style.backgroundColor = '#28a745';
    homeIconPreviewEl.style.color = '#ffffff';
    uiElements.modalHomeIcon.show();
}

/**
 * Duplicates a home icon
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function duplicateHomeIcon(pos) {
    _editHomeIcon(pos, false, "Duplicate home icon");
}

/**
 * Opens the edit home icon dialog
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function editHomeIcon(pos) {
    _editHomeIcon(pos, true, "Edit home icon");
}

/**
 * The real edit home icon function
 * @param {number} pos home icon position
 * @param {boolean} replace true = replace existing home icon, false = duplicate home icon
 * @param {string} title title for the modal
 * @returns {void}
 */
function _editHomeIcon(pos, replace, title) {
    elGetById('modalHomeIconTitle').textContent = tn(title);
    sendAPI("MYMPD_API_HOME_ICON_GET", {"pos": pos}, function(obj) {
        elGetById('inputHomeIconReplace').value = replace;
        elGetById('inputHomeIconOldpos').value = pos;
        elGetById('inputHomeIconName').value = obj.result.data.name;
        elGetById('inputHomeIconLigature').value = obj.result.data.ligature;
        elGetById('inputHomeIconBgcolor').value = obj.result.data.bgcolor;
        elGetById('inputHomeIconColor').value = obj.result.data.color;

        populateHomeIconCmdSelect(obj.result.data.cmd, obj.result.data.options[0]);
        elGetById('selectHomeIconCmd').value = obj.result.data.cmd;
        showHomeIconCmdOptions(obj.result.data.options);
        getHomeIconPictureList();
        elGetById('inputHomeIconImage').value = obj.result.data.image === '' ? tn('Use ligature') : obj.result.data.image;
        setData(elGetById('inputHomeIconImage'),'value', obj.result.data.image);

        elGetById('homeIconPreview').textContent = obj.result.data.ligature;
        elGetById('homeIconPreview').style.backgroundColor = obj.result.data.bgcolor;
        elGetById('homeIconPreview').style.color = obj.result.data.color;

        if (obj.result.data.image === '') {
            elShowId('divHomeIconLigature');
            elGetById('homeIconPreview').style.backgroundImage = '';
        }
        else {
            elHideId('divHomeIconLigature');
            elGetById('homeIconPreview').style.backgroundImage = getCssImageUri(obj.result.data.image);
        }
        //reset ligature selection
        elGetById('searchHomeIconLigature').value = '';
        elGetById('searchHomeIconCat').value = 'all';
        filterHomeIconLigatures();
        //show modal
        cleanupModalId('modalHomeIcon');
        uiElements.modalHomeIcon.show();
    }, false);
}

/**
 * Deletes a home icon
 * @param {number} pos home icon position
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function deleteHomeIcon(pos) {
    sendAPI("MYMPD_API_HOME_ICON_RM", {"pos": pos}, null, false);
}

/**
 * Opens the link in a new window
 * @param {string} link uri to open
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function openExternalLink(link) {
    window.open(link);
}

/**
 * Goto handler for home icons
 * @param {string} type one of dir, search, album, plist, smartpls
 * @param {string} uri type = search: search expression,
 *                     type = album: album id,
 *                     else uri of directory or playlist
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function homeIconGoto(type, uri) {
    switch(type) {
        case 'dir':
            gotoFilesystem(uri[0], type);
            break;
        case 'search':
            appGoto('Search', undefined, undefined, 0, undefined, 'any', {'tag': 'Title', 'desc': false}, '', uri[0]);
            break;
        case 'album':
            //uri = AlbumId
            gotoAlbum(uri[0]);
            break;
        case 'plist':
        case 'smartpls':
            playlistDetails(uri[0]);
            break;
        default:
            logError('Invalid type: ' + type);
    }
}
