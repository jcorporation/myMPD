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
    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalSettings');
        getSettings();
    });

    createSettingsFrm();
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
    const bgImageEl = document.getElementById('SettingBgImageInput');
    const bgImageValue = getData(bgImageEl, 'value');
    if (value === 'light') {
        document.getElementById('SettingBgColorInput').value = '#ffffff';
        if (bgImageValue.indexOf('/assets/') === 0) {
            bgImageEl.value = getBgImageText('/assets/mympd-background-light.svg');
            setData(bgImageEl, 'value', '/assets/mympd-background-light.svg');
        }
    }
    else {
        //dark is the default
        document.getElementById('SettingBgColorInput').value = '#060708';
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
        document.getElementById('SettingBgColorInput').parentNode.parentNode.classList.add('d-none');
        document.getElementById('SettingBgImageInput').parentNode.parentNode.classList.add('d-none');
    }
    else {
        document.getElementById('SettingBgColorInput').parentNode.parentNode.classList.remove('d-none');
        document.getElementById('SettingBgImageInput').parentNode.parentNode.classList.remove('d-none');
    }
}

/**
 * Sets the text for the bgImage input
 * @param {string} value bgImage value
 * @returns {string} bgImage text
 */
function getBgImageText(value) {
    if (value === '') {
        return 'None';
    }
    for (const key of bgImageValues) {
        if (key.value === value) {
            return key.text;
        }
    }
    return value;
}

/**
 * Populates the settings modal
 * @returns {void}
 */
function populateSettingsFrm() {
    getBgImageList();
    const bgImageInput = document.getElementById('SettingBgImageInput');
    setData(bgImageInput, 'value', settings.webuiSettings.uiBgImage);
    bgImageInput.value = getBgImageText(settings.webuiSettings.uiBgImage);

    toggleThemeInputs(settings.webuiSettings.uiTheme);

    //partition specific settings
    document.getElementById('SettingHighlightColorInput').value = settings.partition.highlightColor;
    document.getElementById('SettingHighlightColorContrastInput').value = settings.partition.highlightColorContrast;
    document.getElementById('SettingMpdStreamPortInput').value = settings.partition.mpdStreamPort;
    document.getElementById('SettingStreamUriInput').value = settings.partition.streamUri;

    //locales
    const localeList = document.getElementById('SettingLocaleInput');
    elClear(localeList);
    for (const l in i18n) {
        localeList.appendChild(
            elCreateTextTn('option', {"value": l}, i18n[l].desc)
        );
        if (l === settings.webuiSettings.uiLocale) {
            localeList.lastChild.setAttribute('selected', 'selected');
        }
    }
    warnLocale(settings.webuiSettings.uiLocale);

    //web notifications - check permission
    const btnNotifyWeb = document.getElementById('SettingNotifyWebInput');
    elHideId('SettingNotifyWebWarn');
    if (notificationsSupported()) {
        if (Notification.permission !== 'granted') {
            if (settings.webuiSettings.notifyWeb === true) {
                elShowId('SettingNotifyWebWarn');
            }
            settings.webuiSettings.notifyWeb = false;
        }
        if (Notification.permission === 'denied') {
            elShowId('SettingNotifyWebWarn');
        }
        toggleBtnChk(btnNotifyWeb, settings.webuiSettings.notifyWeb);
        elEnable(btnNotifyWeb);
    }
    else {
        elDisable(btnNotifyWeb);
        toggleBtnChk(btnNotifyWeb, false);
    }

    document.getElementById('SettingScaleRatioInput').value = localSettings.scaleRatio;
    document.getElementById('SettingViewModeInput').value = localSettings.viewMode;

    //media session support
    const btnMediaSession = document.getElementById('SettingMediaSessionInput');
    if (features.featMediaSession === false) {
        elShowId('SettingMediaSessionInputWarn');
        elDisable(btnMediaSession);
        toggleBtnChk(btnMediaSession, false);
    }
    else {
        elHideId('SettingMediaSessionInputWarn');
        elEnable(btnMediaSession);
    }

    document.getElementById('SettingBookletNameInput').value = settings.bookletName;
    document.getElementById('SettingCoverimageNamesInput').value = settings.coverimageNames;
    document.getElementById('SettingThumbnailNamesInput').value = settings.thumbnailNames;
    document.getElementById('SettingListenBrainzTokenInput').value = settings.listenbrainzToken;

    //smart playlists
    if (settings.features.featPlaylists === true) {
        elEnableId('SettingSmartplsEnableBtn');
        toggleBtnChkCollapseId('SettingSmartplsEnableBtn', 'SettingSmartplsCollapse', settings.smartpls);
        elHideId('SettingSmartplsWarn');
    }
    else {
        elDisableId('SettingSmartplsEnableBtn');
        toggleBtnChkCollapseId('SettingSmartplsEnableBtn', 'SettingSmartplsCollapse', false);
        elShowId('SettingSmartplsWarn');
    }
    document.getElementById('SettingSmartplsPrefixInput').value = settings.smartplsPrefix;
    document.getElementById('SettingSmartplsIntervalInput').value = settings.smartplsInterval / 60 / 60;
    addTagListSelect('SettingSmartplsSortInput', 'tagList');
    document.getElementById('SettingSmartplsSortInput').value = settings.smartplsSort;
    //lyrics
    if (features.featLibrary === false) {
        //lyrics need access to library
        settings.webuiSettings.enableLyrics = false;
    }
    toggleBtnChkCollapseId('SettingLyricsEnableBtn', 'SettingLyricsCollapse', settings.webuiSettings.enableLyrics);

    //local playback
    toggleBtnChkCollapseId('SettingLocalPlaybackEnableBtn', 'SettingLocalPlaybackCollapse', settings.webuiSettings.enableLocalPlayback);
    toggleBtnChkId('SettingEnableLocalPlaybackAutoplayInput', localSettings.localPlaybackAutoplay);

    //albumart background css filter
    toggleBtnChkId('SettingBgCoverInput', settings.webuiSettings.uiBgCover);

    //tag multiselects
    initTagMultiSelect('SettingEnabledTagsInput', 'SettingEnabledTagsList', settings.tagListMpd, settings.tagList);
    initTagMultiSelect('SettingSearchTagsInput', 'SettingSearchTagsList', settings.tagList, settings.tagListSearch);
    initTagMultiSelect('SettingBrowseTagsInput', 'SettingBrowseTagsList', settings.tagList, settings.tagListBrowse);
    initTagMultiSelect('SettingGeneratePlsTagsInput', 'SettingGeneratePlsTagsList', settings.tagListBrowse, settings.smartplsGenerateTagList);

    //features - show or hide warnings - use settings object
    setFeatureBtnId('SettingLyricsEnableBtn', settings.features.featLibrary);
    setFeatureBtnId('SettingEnableScriptingInput', settings.features.featScripting);
    setFeatureBtnId('SettingEnableMountsInput', settings.features.featMounts);
    setFeatureBtnId('SettingEnablePartitionsInput', settings.features.featPartitions);
}

/**
 * Creates the settings modal and initializes the elements
 * @returns {void}
 */
function createSettingsFrm() {
    _createSettingsFrm(settings.webuiSettings, webuiSettingsDefault, 'Setting');
    _createSettingsFrm(settings, settingFields, 'Setting');
    initElements(document.getElementById('modalSettings'));
}

/**
 * Creates the settings modal
 * @param {object} fields object with the values for the elements to create
 * @param {object} defaults object with elements to create and the default values
 * @param {string} prefix prefix for element ids
 * @returns {void}
 */
function _createSettingsFrm(fields, defaults, prefix) {
    //TODO: key in defaults is UCFirst, in defaults LCFirst
    const advFrm = {};
    const advSettingsKeys = Object.keys(defaults);
    advSettingsKeys.sort();
    for (let i = 0, j = advSettingsKeys.length; i < j; i++) {
        const key = advSettingsKeys[i];
        if (defaults[key] === undefined || defaults[key].form === undefined) {
            continue;
        }
        const form = defaults[key].form;
        if (advFrm[form] === undefined) {
            advFrm[form] = document.getElementById(form);
            elClear(advFrm[form]);
        }

        if (defaults[key].inputType === 'section') {
            if (defaults[key].title !== undefined) {
                advFrm[form].appendChild(
                    elCreateEmpty('hr', {})
                );
                advFrm[form].appendChild(
                    elCreateTextTn('h4', {}, defaults[key].title)
                );
            }
            else if (defaults[key].subtitle !== undefined) {
                advFrm[form].appendChild(
                    elCreateTextTn('h4', {}, defaults[key].subtitle)
                );
            }
            continue;
        }

        const col = elCreateEmpty('div', {"class": ["col-sm-8", "position-relative"]});
        if (defaults[key].inputType === 'select') {
            const select = elCreateEmpty('select', {"class": ["form-select"], "id": prefix + r(key) + 'Input'});
            for (const value in defaults[key].validValues) {
                select.appendChild(
                    elCreateTextTn('option', {"value": value}, defaults[key].validValues[value])
                );
                if ((defaults[key].contentType === 'integer' && fields[key] === Number(value)) ||
                    fields[key] === value)
                {
                    select.lastChild.setAttribute('selected', 'selected');
                }
            }
            col.appendChild(select);
        }
        else if (defaults[key].inputType === 'mympd-select-search') {
            const input = elCreateEmpty('input', {"class": ["form-select"], "id": prefix + r(key) + 'Input'});
            setData(input, 'cb-filter', defaults[key].cbCallback);
            setData(input, 'cb-filter-options', [input.getAttribute('id')]);
            input.setAttribute('data-is', 'mympd-select-search');
            col.classList.add('position-relative');
            const btnGrp = elCreateNode('div', {"class": ["btn-group", "d-flex"]}, input);
            col.appendChild(btnGrp);
        }
        else if (defaults[key].inputType === 'checkbox') {
            const btn = elCreateEmpty('button', {"type": "button", "id": prefix + r(key) + 'Input', "class": ["btn", "btn-sm", "btn-secondary", "mi", "chkBtn"]});
            if (fields[key] === true) {
                btn.classList.add('active');
                btn.textContent = 'check';
            }
            else {
                btn.textContent = 'radio_button_unchecked';
            }
            if (defaults[key].onClick !== undefined) {
                btn.addEventListener('click', function(event) {
                    // @ts-ignore
                    window[defaults[key].onClick](event);
                }, false);
            }
            else {
                btn.addEventListener('click', function(event) {
                    toggleBtnChk(event.target, undefined);
                }, false);
            }
            col.appendChild(btn);
        }
        else if (defaults[key].inputType === 'password') {
            const input = elCreateEmpty('input', {"is": "mympd-input-password", "id": prefix + r(key) + 'Input',
                "value": fields[key], "class": ["form-control"], "type": "password"});
            col.appendChild(input);
        }
        else {
            const it = defaults[key].inputType === 'color' ? 'color' : 'text';
            const placeholder = defaults[key].placeholder !== undefined
                ? defaults[key].placeholder
                : defaults[key].defaultValue;
            const input = elCreateEmpty('input', {"is": "mympd-input-reset", "id": prefix + r(key) + 'Input', "data-default": defaults[key].defaultValue,
                "placeholder": placeholder, "value": fields[key], "class": ["form-control"], "type": it});
            col.appendChild(input);
        }
        if (defaults[key].invalid !== undefined) {
            col.appendChild(
                elCreateTextTn('div', {"class": ["invalid-feedback"]}, defaults[key].invalid)
            );
        }
        if (defaults[key].warn !== undefined) {
            col.appendChild(
                elCreateTextTn('div', {"id": prefix + r(key) + 'InputWarn', "class": ["mt-2", "mb-1", "alert", "alert-warning", "d-none"]}, defaults[key].warn)
            );
        }
        if (defaults[key].help !== undefined) {
            col.appendChild(
                elCreateTextTn('small', {"class": ["help"]}, defaults[key].help)
            );
        }

        const label = defaults[key].hint === undefined
            ? elCreateTextTn('label', {"class": ["col-sm-4", "col-form-label"], "for": prefix + r(key) + 'Input'}, defaults[key].title)
            : elCreateNodes('label', {"class": ["col-sm-4", "col-form-label"], "for": prefix + r(key) + 'Input'}, [
                    elCreateTextTn('span', {}, defaults[key].title),
                    elCreateText('small', {"class": ["mi", "mi-sm", "ms-1"], "title": tn("Browser specific setting"), "data-title-phrase": "Browser specific setting"}, defaults[key].hint)
              ]);

        let rowClasses = ["mb-3", "row"];
        if (defaults[key].cssClass !== undefined) {
            rowClasses = rowClasses.concat(defaults[key].cssClass);
        }
        advFrm[form].appendChild(
            elCreateNodes('div', {"class": rowClasses}, [
                label,
                col
            ])
        );
    }

    for (const key in defaults) {
        if (defaults[key].onChange !== undefined) {
            document.getElementById(prefix + r(key) + 'Input').addEventListener('change', function(event) {
                // @ts-ignore
                window[defaults[key].onChange](event);
            }, false);
        }
    }

    //set featWhence feature detection for default actions
    for (const sel of ['SettingClickQuickPlayInput', 'SettingClickFilesystemPlaylistInput',
        'SettingClickPlaylistInput', 'SettingClickSongInput',
        'SettingClickRadioFavoritesInput', 'SettingClickRadiobrowserInput'])
    {
        const options = document.querySelectorAll('#' + sel + ' > option');
        for (const opt of options) {
            if (opt.value === 'insert' || opt.value === 'play') {
                opt.classList.add('featWhence');
            }
        }
    }
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
 * Saves the settings
 * @param {boolean} closeModal true = close modal, else not
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function saveSettings(closeModal) {
    cleanupModalId('modalSettings');
    let formOK = true;

    for (const inputId of ['SettingThumbnailSizeInput', 'SettingLastPlayedCountInput',
        'SettingSmartplsIntervalInput', 'SettingVolumeMaxInput', 'SettingVolumeMinInput',
        'SettingVolumeStepInput'])
    {
        const inputEl = document.getElementById(inputId);
        if (validateUintEl(inputEl) === false) {
            formOK = false;
        }
    }

    const inputCoverimageNames = document.getElementById('SettingCoverimageNamesInput');
    if (validateFilenameListEl(inputCoverimageNames) === false) {
        formOK = false;
    }
    const inputThumbnailNames = document.getElementById('SettingThumbnailNamesInput');
    if (validateFilenameListEl(inputThumbnailNames) === false) {
        formOK = false;
    }

    const inputBookletName = document.getElementById('SettingBookletNameInput');
    if (validateFilenameEl(inputBookletName) === false) {
        formOK = false;
    }

    const inputScaleRatio = document.getElementById('SettingScaleRatioInput');
    //handle scaleRatio only for mobile browsers
    if (userAgentData.isMobile === true) {
        if (validateFloatEl(inputScaleRatio) === false) {
            formOK = false;
        }
    }

    if (formOK === true) {
        //browser specific settings
        localSettings.localPlaybackAutoplay = getBtnChkValueId('SettingEnableLocalPlaybackAutoplayInput');
        localSettings.viewMode = getSelectValueId('SettingViewModeInput');
        setUserAgentData();

        //use scaleRatio only for mobile browsers
        localSettings.scaleRatio = userAgentData.isMobile === true ?
            inputScaleRatio.value : '1.0';
        setViewport();

        //save localSettings in browsers localStorage
        try {
            for (const key in localSettings) {
                localStorage.setItem(key, localSettings[key]);
            }
        }
        catch(err) {
            logError('Can not save settings to localStorage: ' + err.message);
        }

        //from hours to seconds
        const smartplsInterval = Number(document.getElementById('SettingSmartplsIntervalInput').value) * 60 * 60;

        const webuiSettings = {};
        for (const key in webuiSettingsDefault) {
            const el = document.getElementById('Setting' + r(key) + 'Input');
            if (el) {
                if (webuiSettingsDefault[key].inputType === 'select') {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(getSelectValue(el)) : getSelectValue(el);
                }
                else if (webuiSettingsDefault[key].inputType === 'mympd-select-search') {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(getData(el, 'value')) : getData(el, 'value');
                }
                else if (webuiSettingsDefault[key].inputType === 'checkbox') {
                    webuiSettings[key] = getBtnChkValue(el);
                }
                else {
                    webuiSettings[key] = webuiSettingsDefault[key].contentType === 'integer' ? Number(el.value) : el.value;
                }
            }
            else if (webuiSettingsDefault[key].defaultValue !== undefined) {
                webuiSettings[key] = webuiSettingsDefault[key].defaultValue;
            }
        }

        webuiSettings.enableLyrics = getBtnChkValueId('SettingLyricsEnableBtn');
        webuiSettings.enableLocalPlayback = getBtnChkValueId('SettingEnableLocalPlaybackBtn');

        const params = {
            "coverimageNames": inputCoverimageNames.value,
            "thumbnailNames": inputThumbnailNames.value,
            "lastPlayedCount": Number(document.getElementById('SettingLastPlayedCountInput').value),
            "smartpls": getBtnChkValueId('SettingSmartplsEnableBtn'),
            "smartplsPrefix": document.getElementById('SettingSmartplsPrefixInput').value,
            "smartplsInterval": smartplsInterval,
            "smartplsSort": document.getElementById('SettingSmartplsSortInput').value,
            "smartplsGenerateTagList": getTagMultiSelectValues(document.getElementById('SettingGeneratePlsTagsList'), false),
            "tagList": getTagMultiSelectValues(document.getElementById('SettingEnabledTagsList'), false),
            "tagListSearch": getTagMultiSelectValues(document.getElementById('SettingSearchTagsList'), false),
            "tagListBrowse": getTagMultiSelectValues(document.getElementById('SettingBrowseTagsList'), false),
            "bookletName": inputBookletName.value,
            "volumeMin": Number(document.getElementById('SettingVolumeMinInput').value),
            "volumeMax": Number(document.getElementById('SettingVolumeMaxInput').value),
            "volumeStep": Number(document.getElementById('SettingVolumeStepInput').value),
            "lyricsUsltExt": document.getElementById('SettingLyricsUsltExtInput').value,
            "lyricsSyltExt": document.getElementById('SettingLyricsSyltExtInput').value,
            "lyricsVorbisUslt": document.getElementById('SettingLyricsVorbisUsltInput').value,
            "lyricsVorbisSylt": document.getElementById('SettingLyricsVorbisSyltInput').value,
            "listenbrainzToken": document.getElementById('SettingCloudListenBrainzTokenInput').value,
            "webuiSettings": webuiSettings
        };

        if (closeModal === true) {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', params, saveSettingsClose, true);
        }
        else {
            sendAPIpartition('default', 'MYMPD_API_SETTINGS_SET', params, saveSettingsApply, true);
        }
    }
}

/**
 * Response handler for MYMPD_API_SETTINGS_SET that closes the modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        savePartitionSettings(true);
    }
}

/**
 * Response handler for MYMPD_API_SETTINGS_SET
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function saveSettingsApply(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        savePartitionSettings(false);
    }
}

/**
 * Saves the partition specific settings
 * @param {boolean} closeModal true = close modal, else not
 * @returns {void}
 */
//eslint-disable-next-line no-unused-vars
function savePartitionSettings(closeModal) {
    let formOK = true;
    const mpdStreamPortEl = document.getElementById('SettingMpdStreamPortInput');
    const streamUriEl = document.getElementById('SettingStreamUriInput');
    if (validateIntRangeEl(mpdStreamPortEl, 0, 65535) === false) {
        formOK = false;
    }
    if (streamUriEl.value.length > 0 &&
        validateStreamEl(streamUriEl) === false)
    {
        formOK = false;
    }

    if (formOK === true) {
        const params = {
            "highlightColor": document.getElementById('SettingHighlightColorInput').value,
            "highlightColorContrast": document.getElementById('SettingHighlightColorContrastInput').value,
            "mpdStreamPort": Number(mpdStreamPortEl.value),
            "streamUri": streamUriEl.value
        };
        if (closeModal === true) {
            sendAPI('MYMPD_API_PARTITION_SAVE', params, savePartitionSettingsClose, true);
        }
        else {
            sendAPI('MYMPD_API_PARTITION_SAVE', params, savePartitionSettingsApply, true);
        }
    }
}

/**
 * Response handler for MYMPD_API_PARTITION_SAVE that closes the modal
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionSettingsApply(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        btnWaiting(document.getElementById('btnApplySettings'), true);
        getSettings();
    }
}

/**
 * Response handler for MYMPD_API_PARTITION_SAVE
 * @param {object} obj jsonrpc response
 * @returns {void}
 */
function savePartitionSettingsClose(obj) {
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings();
        uiElements.modalSettings.hide();
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
        elHideId('SettingNotifyWebWarn');
        return;
    }
    Notification.requestPermission(function (permission) {
        if (permission === 'granted') {
            toggleBtnChk(btnNotifyWeb, true);
            settings.webuiSettings.notifyWeb = true;
            elHideId('SettingNotifyWebWarn');
        }
        else {
            toggleBtnChk(btnNotifyWeb, false);
            settings.webuiSettings.notifyWeb = false;
            elShowId('SettingNotifyWebWarn');
        }
    });
}

/**
 * Shows the missing translations warning
 * @param {string} value locale name
 * @returns {void}
 */
function warnLocale(value) {
    const warnEl = document.getElementById('SettingMissingPhrasesWarn');
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
