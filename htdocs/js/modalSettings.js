"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module modalSettings_js */

/**
 * Initialization function for the settings elements
 * @returns {void}
 */
function initModalSettings() {
    // cache for the form field containers
    const forms = {};
    // create the fields
    createForm(settingsFields, 'modalSettings', forms);
    createForm(settingsWebuiFields, 'modalSettings', forms);
    createForm(settingsPartitionFields, 'modalSettings', forms);
    createForm(settingsLocalFields, 'modalSettings', forms);
    // initialize myMPD custom elements
    initElements(document.getElementById('modalSettings'));

    //set featWhence feature detection for default actions
    for (const sel of ['modalSettingsClickQuickPlayInput', 'modalSettingsClickFilesystemPlaylistInput',
        'modalSettingsClickPlaylistInput', 'modalSettingsClickSongInput',
        'modalSettingsClickRadioFavoritesInput', 'modalSettingsClickRadiobrowserInput'])
    {
        const options = document.querySelectorAll('#' + sel + ' > option');
        for (const opt of options) {
            if (opt.value === 'insert' ||
                opt.value === 'play')
            {
                opt.classList.add('featWhence');
            }
        }
    }

    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettings');
        getSettings();
    });
}

/**
 * Change eventhandler for the locale select
 * @param {Event} event change event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function eventChangeLocale(event) {
    const value = getSelectValue(event.target);
    warnLocale(value);
}

/**
 * Change eventhandler for the theme select
 * @param {Event} event change event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function eventChangeTheme(event) {
    const value = getSelectValue(event.target);
    const bgImageEl = document.getElementById('modalSettingsBgImageInput');
    const bgImageValue = getData(bgImageEl, 'value');
    if (value === 'light') {
        document.getElementById('modalSettingsBgColorInput').value = '#ffffff';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = getBgImageText('/assets/mympd-background-light.svg');
            setData(bgImageEl, 'value', '/assets/mympd-background-light.svg');
        }
    }
    else {
        //dark is the default
        document.getElementById('modalSettingsBgColorInput').value = '#060708';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = getBgImageText('/assets/mympd-background-dark.svg');
            setData(bgImageEl, 'value', '/assets/mympd-background-dark.svg');
        }
    }
    toggleThemeInputs(value);
}

/**
 * Shows or hides the background color and image inputs
 * @param {string} theme theme name
 * @returns {void}
 */
function toggleThemeInputs(theme) {
    if (theme === 'auto') {
        document.getElementById('modalSettingsBgColorInput').parentNode.parentNode.classList.add('d-none');
        document.getElementById('modalSettingsBgImageInput').parentNode.parentNode.classList.add('d-none');
    }
    else {
        document.getElementById('modalSettingsBgColorInput').parentNode.parentNode.classList.remove('d-none');
        document.getElementById('modalSettingsBgImageInput').parentNode.parentNode.classList.remove('d-none');
    }
}

/**
 * Sets the text for the bgImage input
 * @param {string} value bgImage value
 * @returns {string} bgImage text
 */
function getBgImageText(value) {
    if (value === '') {
        return tn('None');
    }
    for (const key of bgImageValues) {
        if (key.value === value) {
            return key.text;
        }
    }
    return value;
}

/**
 * Gets the background images list and populates the select element
 * @returns {void}
 */
function getBgImageList() {
    const list = document.getElementById('modalSettingsBgImageInput');
    getImageList(list, bgImageValues, 'backgrounds');
}

/**
 * Populates the settings modal
 * @returns {void}
 */
function populateSettingsFrm() {
    jsonToForm(settings, settingsFields, 'modalSettings');
    jsonToForm(settings.webuiSettings, settingsWebuiFields, 'modalSettings');
    jsonToForm(settings.partition, settingsPartitionFields, 'modalSettings');
    jsonToForm(localSettings, settingsLocalFields, 'modalSettings');
    // background image select
    getBgImageList();
    const bgImageInput = document.getElementById('modalSettingsBgImageInput');
    setData(bgImageInput, 'value', settings.webuiSettings.bgImage);
    bgImageInput.value = getBgImageText(settings.webuiSettings.bgImage);

    // theme
    toggleThemeInputs(settings.webuiSettings.theme);

    //locales
    const localeList = document.getElementById('modalSettingsLocaleInput');
    elClear(localeList);
    for (const l in i18n) {
        localeList.appendChild(
            elCreateTextTn('option', {"value": l}, i18n[l].desc)
        );
        if (l === settings.webuiSettings.locale) {
            localeList.lastChild.setAttribute('selected', 'selected');
        }
    }
    warnLocale(settings.webuiSettings.locale);

    // web notifications - check permission
    const btnNotifyWeb = document.getElementById('modalSettingsNotifyWebInput');
    elHideId('modalSettingsNotifyWebWarn');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.webuiSettings.notifyWeb === true) {
                elShowId('modalSettingsNotifyWebWarn');
            }
            settings.webuiSettings.notifyWeb = false;
        }
        if (Notification.permission === 'denied') {
            elShowId('modalSettingsNotifyWebWarn');
        }
        toggleBtnChk(btnNotifyWeb, settings.webuiSettings.notifyWeb);
        elEnable(btnNotifyWeb);
    }
    else {
        elDisable(btnNotifyWeb);
        toggleBtnChk(btnNotifyWeb, false);
    }

    // media session support
    const btnMediaSession = document.getElementById('modalSettingsMediaSessionInput');
    if (features.featMediaSession === false) {
        elShowId('modalSettingsMediaSessionInputWarn');
        elDisable(btnMediaSession);
        toggleBtnChk(btnMediaSession, false);
    }
    else {
        elHideId('modalSettingsMediaSessionInputWarn');
        elEnable(btnMediaSession);
    }

    // smart playlists
    if (settings.features.featPlaylists === true) {
        elEnableId('modalSettingsSmartplsInput');
        toggleBtnChkCollapseId('modalSettingsSmartplsInput', 'modalSettingsSmartplsCollapse', settings.smartpls);
        elHideId('modalSettingsSmartplsWarn');
    }
    else {
        elDisableId('modalSettingsSmartplsInput');
        toggleBtnChkCollapseId('modalSettingsSmartplsInput', 'modalSettingsSmartplsCollapse', false);
        elShowId('modalSettingsSmartplsWarn');
    }
    addTagListSelect('modalSettingsSmartplsSortInput', 'tagList');
    document.getElementById('modalSettingsSmartplsSortInput').value = settings.smartplsSort;
    // seconds to hours
    document.getElementById('modalSettingsSmartplsIntervalInput').value = settings.smartplsInterval / 60 / 60;

    // lyrics
    if (features.featLibrary === false) {
        //lyrics need access to library
        settings.webuiSettings.enableLyrics = false;
    }
    toggleBtnChkCollapseId('modalSettingsEnableLyricsInput', 'modalSettingsLyricsCollapse', settings.webuiSettings.enableLyrics);

    // local playback
    toggleBtnChkCollapseId('modalSettingsEnableLocalPlaybackInput', 'modalSettingsLocalPlaybackCollapse', settings.webuiSettings.enableLocalPlayback);

    // tag multiselects
    initTagMultiSelect('modalSettingsEnabledTagsInput', 'modalSettingsEnabledTagsList', settings.tagListMpd, settings.tagList);
    initTagMultiSelect('modalSettingsSearchTagsInput', 'modalSettingsSearchTagsList', settings.tagList, settings.tagListSearch);
    initTagMultiSelect('modalSettingsBrowseTagsInput', 'modalSettingsBrowseTagsList', settings.tagList, settings.tagListBrowse);
    initTagMultiSelect('modalSettingsGeneratePlsTagsInput', 'modalSettingsGeneratePlsTagsList', settings.tagListBrowse, settings.smartplsGenerateTagList);

    // handle features: show or hide warnings - use the settings object
    setFeatureBtnId('modalSettingsEnableLyricsInput', settings.features.featLibrary);
    setFeatureBtnId('modalSettingsEnableScriptingInput', settings.features.featScripting);
    setFeatureBtnId('modalSettingsEnableMountsInput', settings.features.featMounts);
    setFeatureBtnId('modalSettingsEnablePartitionsInput', settings.features.featPartitions);
}

/**
 * Shows the warning for a disabled feature button (feature is not supported by the backend)
 * @param {string} id button id
 * @param {boolean} value true = enable button and hide warning
 *                        false = disable button and show warning
 * @returns {void}
 */
function setFeatureBtnId(id, value) {
    if (value === true) {
        elEnableId(id);
        elHideId(id + 'Warn');
    }
    else {
        elDisableId(id);
        toggleBtnChkId(id, false);
        elShowId(id + 'Warn');
    }
}

/**
 * Save localSettings in browsers localStorage
 * @returns {boolean} true on success, else false
 */
function saveLocalSettings() {
    try {
        for (const key in localSettings) {
            localStorage.setItem(key, localSettings[key]);
        }
    }
    catch(err) {
        logError('Can not save settings to localStorage: ' + err.message);
        return false;
    }
    return true;
}

/**
 * Saves the settings
 * @param {Element} target triggering element
 * @param {boolean} closeModal true = close modal, else not
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSettings(target, closeModal) {
    cleanupModalId('modalSettings');

    const settingsParams = {};
    settingsParams.webuiSettings = {};
    if (formToJson('modalSettings', settingsParams, settingsFields) === true &&
        formToJson('modalSettings', settingsParams.webuiSettings, settingsWebuiFields) === true &&
        formToJson('modalSettings', localSettings, settingsLocalFields) === true)
    {
        if (saveLocalSettings() === false) {
            modalClose({"error": {"message": "Can not save browser specific settings to localStorage"}});
            return;
        }
        // from hours to seconds
        settingsParams.smartplsInterval = settingsParams.smartplsInterval * 60 * 60;
        // manual fields
        settingsParams.smartplsGenerateTagList = getTagMultiSelectValues(document.getElementById('modalSettingsGeneratePlsTagsList'), false);
        settingsParams.tagList = getTagMultiSelectValues(document.getElementById('modalSettingsEnabledTagsList'), false);
        settingsParams.tagListSearch = getTagMultiSelectValues(document.getElementById('modalSettingsSearchTagsList'), false);
        settingsParams.tagListBrowse = getTagMultiSelectValues(document.getElementById('modalSettingsBrowseTagsList'), false);

        btnWaiting(target, true);
        if (closeModal === true) {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', settingsParams, saveSettingsClose, true);
        }
        else {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', settingsParams, saveSettingsApply, true);
        }
    }
}

/**
 * Response handler for MYMPD_API_SETTINGS_SET that closes the modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsClose(obj) {
    // modal is closed in the next close handler
    if (modalApply(obj) === true) {
        savePartitionSettings(true);
    }
}

/**
 * Response handler for MYMPD_API_SETTINGS_SET
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsApply(obj) {
    if (modalApply(obj) === true) {
        savePartitionSettings(false);
    }
}

/**
 * Saves the partition specific settings
 * @param {boolean} closeModal true = close modal, else not
 * @returns {void}
 */
function savePartitionSettings(closeModal) {
    const settingsParams = {};
    if (formToJson('modalSettings', settingsParams, settingsPartitionFields) === true) {
        if (closeModal === true) {
            sendAPI('MYMPD_API_PARTITION_SAVE', settingsParams, savePartitionSettingsClose, true);
        }
        else {
            sendAPI('MYMPD_API_PARTITION_SAVE', settingsParams, savePartitionSettingsApply, true);
        }
    }
}

/**
 * Response handler for MYMPD_API_PARTITION_SAVE that closes the modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionSettingsApply(obj) {
    if (modalApply(obj) === true) {
        getSettings();
    }
}

/**
 * Response handler for MYMPD_API_PARTITION_SAVE
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionSettingsClose(obj) {
    if (modalClose(obj) === true) {
        getSettings();
    }
}

/**
 * Gets all selected tags from a tag multiselect
 * @param {HTMLElement} taglist container of the checkboxes
 * @param {boolean} translate true = translate the name, else not
 * @returns {string} comma separated list of selected tags
 */
function getTagMultiSelectValues(taglist, translate) {
    const values = [];
    const chkBoxes = taglist.querySelectorAll('button');
    for (let i = 0, j = chkBoxes.length; i < j; i++) {
        if (chkBoxes[i].classList.contains('active')) {
            if (translate === true) {
                values.push(tn(chkBoxes[i].name));
            }
            else {
                values.push(chkBoxes[i].name);
            }
        }
    }
    if (translate === true) {
        return values.join(', ');
    }
    return values.join(',');
}

/**
 * Initializes a tag multiselect
 * @param {string} inputId input element id to initialize
 * @param {string} listId list container element id to initialize
 * @param {object} allTags all tags to list
 * @param {object} enabledTags already enabled tags
 * @returns {void}
 */
function initTagMultiSelect(inputId, listId, allTags, enabledTags) {
    const values = [];
    const list = document.getElementById(listId);
    elClear(list);
    for (let i = 0, j = allTags.length; i < j; i++) {
        if (enabledTags.includes(allTags[i])) {
            values.push(tn(allTags[i]));
        }
        const btn = elCreateEmpty('button', {"class": ["btn", "btn-secondary", "btn-xs", "mi", "mi-sm", "me-2"], "name": allTags[i]});
        if (enabledTags.includes(allTags[i])) {
            btn.classList.add('active');
            btn.textContent = 'check';
        }
        else {
            btn.textContent = 'radio_button_unchecked';
        }
        list.appendChild(
            elCreateNodes('div', {"class": ["form-check"]}, [
                btn,
                elCreateTextTn('label', {"class": ["form-check-label"], "for": allTags[i]}, allTags[i])
            ])
        );
    }

    const inputEl = document.getElementById(inputId);
    inputEl.value = values.join(', ');
    if (getData(inputEl, 'init') === true) {
        return;
    }
    setData(inputEl, 'init', true);
    document.getElementById(listId).addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        if (event.target.nodeName === 'BUTTON') {
            toggleBtnChk(event.target, undefined);
            event.target.parentNode.parentNode.parentNode.previousElementSibling.value =
                getTagMultiSelectValues(event.target.parentNode.parentNode, true);
        }
    });
}

/**
 * Filters the selected column by available tags
 * @param {string} tableName the table name
 * @returns {void}
 */
function filterCols(tableName) {
    //set available tags
    const tags = setColTags(tableName);
    //column name
    const set = "cols" + tableName;
    settings[set] = settings[set].filter(function(value) {
        return tags.includes(value);
    });
    logDebug('Columns for ' + set + ': ' + settings[set]);
}

/**
 * Event handler for the enable web notification button that requests the permission from the user
 * @param {Event} event change event
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function toggleBtnNotifyWeb(event) {
    const btnNotifyWeb = event.target;
    const notifyWebState = getBtnChkValue(btnNotifyWeb);
    if (notificationsSupported() === false) {
        toggleBtnChk(btnNotifyWeb, false);
        settings.webuiSettings.notifyWeb = false;
        return;
    }
    if (notifyWebState === true) {
        toggleBtnChk(btnNotifyWeb, false);
        settings.webuiSettings.notifyWeb = false;
        elHideId('modalSettingsNotifyWebWarn');
        return;
    }
    Notification.requestPermission(function (permission) {
        if (permission === 'granted') {
            toggleBtnChk(btnNotifyWeb, true);
            settings.webuiSettings.notifyWeb = true;
            elHideId('modalSettingsNotifyWebWarn');
        }
        else {
            toggleBtnChk(btnNotifyWeb, false);
            settings.webuiSettings.notifyWeb = false;
            elShowId('modalSettingsNotifyWebWarn');
        }
    });
}

/**
 * Shows the missing translations warning
 * @param {string} value locale name
 * @returns {void}
 */
function warnLocale(value) {
    const warnEl = document.getElementById('modalSettingsMissingPhrasesWarn');
    elClear(warnEl);
    if (i18n[value].missingPhrases > 0) {
        warnEl.appendChild(
            elCreateTextTnNr('p', {}, 'Missing translations', i18n[value].missingPhrases)
        );
        warnEl.appendChild(
            elCreateTextTn('a', {"class": ["alert-link", "external"], "target": "_blank",
                "href": "https://github.com/jcorporation/myMPD/discussions/167"}, 'Help to improve myMPD')
        );
        elShow(warnEl);
    }
    else {
        elHide(warnEl);
    }
}
