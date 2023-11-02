"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
    features.featLocalPlayback = settings.webuiSettings.enableLocalPlayback
        ? settings.partition.mpdStreamPort > 0 || settings.partition.streamUri.length > 0
            ? true
            : false
        : false;
    features.featScripting = settings.webuiSettings.enableScripting
        ? settings.features.featScripting
        : false;
    features.featTimer = settings.features.featTimer && settings.webuiSettings.enableTimer;
    features.featTrigger = settings.webuiSettings.enableTrigger;
    features.featMediaSession = checkMediaSessionSupport();
    features.featFooterNotifications = settings.webuiSettings.footerNotifications;
    features.featSession = settings.pin;
    features.featStickersEnabled = settings.features.featStickersEnabled;

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
        features.featPartitions = settings.webuiSettings.enablePartitions
            ? settings.features.featPartitions
            : false;
        features.featPlaylists = settings.features.featPlaylists;
        features.featSmartplsAvailable = settings.features.featPlaylists && settings.features.featTags;
        features.featSmartpls = settings.features.featPlaylists && settings.features.featTags
            ? settings.smartpls
            : false;
        features.featStickers = settings.features.featStickers;
        features.featTags = settings.features.featTags;
        features.featBinarylimit = settings.features.featBinarylimit;
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

