"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module features_js */

/**
 * Sets the features object accordingly to the backend features and settings
 * @returns {void}
 */
function setFeatures() {
    //web ui features
    features.featAdvAlbum = settings.albumMode === 'adv';
    features.featCacert = settings.features.featCacert;
    features.featHome = settings.webuiSettings.enableHome;
    features.featVolumeLevel = settings.webuiSettings.footerVolumeLevel;
    features.featScripting = settings.webuiSettings.enableScripting
        ? settings.features.featScripting
        : false;
    features.featTimer = settings.webuiSettings.enableTimer;
    features.featMediaSession = checkMediaSessionSupport();
    features.featFooterNotifications = settings.webuiSettings.footerNotifications;
    features.featSession = settings.pin;
    features.featFooterAudioFormat = settings.webuiSettings.footerAudioFormat;
    features.featMygpiod = settings.features.featMygpiod;
    features.featWebradioDB = settings.features.featWebradioDB;
    features.viewTitle = settings.webuiSettings.viewTitles;
    //stickers config value
    features.featStickersEnabled = settings.features.featStickersEnabled;

    features.featPagination = settings.webuiSettings.endlessScroll === false
        ? true
        : false;

    //Local playback features
    features.featLocalPlayback = settings.webuiSettings.enableLocalPlayback
        ? settings.partition.mpdStreamPort > 0 || settings.partition.streamUri.length > 0
            ? true
            : false
        : false;
    features.featLocalPlaybackOutput = detectFeatureLocalPlaybackOutput();
    features.featLocalPlaybackVolume = userAgentData.isSafari === true && userAgentData.isMobile === true
        ? false
        : true;

    //mpd features
    if (settings.partition.mpdConnected === true) {
        features.featLibrary = settings.features.featLibrary;
        features.featLyrics = settings.webuiSettings.enableLyrics
            ? settings.features.featLibrary
            : false;
        features.featMounts = settings.webuiSettings.enableMounts
            ? settings.features.featMounts
            : false;
        features.featNeighbors = settings.webuiSettings.enableMounts
            ? settings.features.featNeighbors
            : false;
        features.featPartitions = settings.webuiSettings.enablePartitions;
        features.featPlaylists = settings.features.featPlaylists;
        features.featSmartplsAvailable = settings.features.featPlaylists && settings.features.featTags;
        features.featSmartpls = settings.features.featPlaylists && settings.features.featTags
            ? settings.smartpls
            : false;
        features.featStickers = settings.features.featStickers;
        features.featTags = settings.features.featTags;
        features.featFingerprint = settings.features.featFingerprint;
        features.featPlaylistRmRange = settings.features.featPlaylistRmRange;
        features.featWhence = settings.features.featWhence;
        features.featAdvqueue = settings.features.featAdvqueue;
        features.featConsumeOneshot = settings.features.featConsumeOneshot;
        features.featPlaylistDirAuto = settings.features.featPlaylistDirAuto;
        features.featStartsWith = settings.features.featStartsWith;
        features.featPcre = settings.features.featPcre;
        features.featPcreOrStartsWith = settings.features.featPcre || settings.features.featStartsWith;
        features.featLike = features.featStickers && settings.webuiSettings.feedback === 'like';
        features.featRating = features.featStickers && settings.webuiSettings.feedback === 'rating';
        features.featDbAdded = settings.features.featDbAdded;
        features.featStickerAdv  = features.featStickers && settings.features.featStickerAdv;
        features.featStringnormalization = settings.features.featStringnormalization;
    }
}

/**
 * Shows or hides feature related elements
 * @returns {void}
 */
function applyFeatures() {
    //show or hide elements
    for (const feature in features) {
        const featureEls = document.querySelectorAll('.' + feature);
        let displayValue = features[feature] === true
            ? ''
            : 'none';
        for (const el of featureEls) {
            el.style.display = displayValue;
        }
        const notfeatureEls = document.querySelectorAll('.not' + feature);
        displayValue = features[feature] === true
            ? 'none'
            : '';
        for (const el of notfeatureEls) {
            el.style.display = displayValue;
        }
    }
}

/**
 * Detects support for local playback device selection
 * @returns {boolean} true if supported, else false
 */
function detectFeatureLocalPlaybackOutput() {
    if (navigator.mediaDevices !== undefined &&
        'setSinkId' in HTMLAudioElement.prototype)
    {
        logDebug('Enabling local playback output selection');
        return true;
    }
    logDebug('Disabling local playback output selection');
    return false;
}
