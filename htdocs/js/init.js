"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module init_js */

/**
 * Initializes / starts the myMPD app
 */

/**
 * Shows an error message on the splashscreen
 * @param {string} text message to display (untranslated)
 * @returns {void}
 */
 function showAppInitAlert(text) {
    const spa = elGetById('splashScreenAlert');
    elClear(spa);
    spa.appendChild(
        elCreateTextTn('p', {"class": ["text-light"]}, text)
    );
    const reloadBtn = elCreateTextTn('button', {"class": ["btn", "btn-light", "me-2", "alwaysEnabled"]}, 'Reload');
    reloadBtn.addEventListener('click', function() {
        clearAndReload();
    }, false);
    const resetBtn = elCreateTextTn('button', {"class": ["btn", "btn-light", "alwaysEnabled"]}, 'Reset');
    resetBtn.addEventListener('click', function() {
        resetLocalSettings();
        clearAndReload();
    }, false);
    spa.appendChild(
        elCreateNodes('p', {}, [
            reloadBtn,
            resetBtn
        ])
    );
}

/**
 * Clears the service worker caches
 * @returns {void}
 */
function clearCache() {
    if ('serviceWorker' in navigator) {
        caches.keys().then(function(cacheNames) {
            cacheNames.forEach(function(cacheName) {
                caches.delete(cacheName);
            });
        });
    }
}

/**
 * Clears the service worker caches and reloads the app
 * @returns {void}
 */
function clearAndReload() {
    clearCache();
    location.reload();
}

/**
 * Starts the app
 * @returns {void}
 */
function appInitStart() {
    getAssets();
    //add app routing event handler
    window.addEventListener('hashchange', function() {
        if (app.goto === false) {
            appRoute();
        }
        else {
            app.goto = false;
        }
    }, false);

    // create pre-generated elements
    createPreGeneratedElements();

    //webapp manifest shortcuts
    const params = new URLSearchParams(window.location.search);
    const action = params.get('action');
    switch(action) {
        case 'clickPlay':
            clickPlay();
            break;
        case 'clickStop':
            clickStop();
            break;
        case 'clickNext':
            clickNext();
            break;
        // No Default
    }

    //update table height on window resize
    window.addEventListener('resize', function() {
        if (resizeTimer !== null) {
            clearTimeout(resizeTimer);
        }
        resizeTimer = setTimeout(function() {
            const list = elGetById(app.id + 'List');
            if (list) {
                setScrollViewHeight(list);
            }
            resizeTimer = null;
        }, 100);
    }, false);

    setMobileView();

    i18nHtml(elGetById('splashScreenAlert'));

    //set loglevel
    if (debugMode === true) {
        settings.loglevel = 4;
    }

    //serviceworker handling
    if ('serviceWorker' in navigator) {
        //add serviceworker
        if (debugMode === false &&
            window.location.protocol === 'https:')
        {
            window.addEventListener('load', function() {
                navigator.serviceWorker.register('sw.js', {scope: subdir + '/'}).then(function(registration) {
                    //Registration was successful
                    logDebug('ServiceWorker registration successful.');
                    registration.update();
                }, function(err) {
                    //Registration failed
                    logError('ServiceWorker registration failed: ' + err);
                });
            });
        }
        //debugMode - delete serviceworker
        if (debugMode === true) {
            let serviceWorkerExists = false;
            navigator.serviceWorker.getRegistrations().then(function(registrations) {
                for (const registration of registrations) {
                    registration.unregister();
                    serviceWorkerExists = true;
                }
                if (serviceWorkerExists === true) {
                    clearAndReload();
                }
                else {
                    clearCache();
                }
            }).catch(function(err) {
                logError('Service Worker unregistration failed: ' + err);
            });
        }
    }

    //show splash screen
    elShowId('splashScreen');
    domCache.body.classList.add('overflow-hidden');
    elGetById('splashScreenAlert').textContent = tn('Fetch myMPD settings');

    //initialize app
    appInited = false;
    settingsParsed = 'no';
    sendAPI("MYMPD_API_SETTINGS_GET", {}, function(obj) {
        parseSettings(obj);
        if (settingsParsed === 'parsed') {
            //connect to websocket in background
            setTimeout(function() {
                webSocketConnect();
            }, 0);
            //app initialized
            elGetById('splashScreenAlert').textContent = tn('Applying settings');
            elGetById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                elHideId('splashScreen');
                elGetById('splashScreen').classList.remove('hide-fade');
                domCache.body.classList.remove('overflow-hidden');
            }, 500);
            appInit();
            appInited = true;
            appRoute();
            logDebug('Startup duration: ' + (Date.now() - startTime) + 'ms');
        }
    }, true);
}

/**
 * Initializes the html elements
 * @returns {void}
 */
function appInit() {
    //init links
    const hrefs = document.querySelectorAll('[data-href]');
    for (const href of hrefs) {
        if (href.nodeName !== 'A' &&
            href.nodeName !== 'BUTTON' &&
            href.classList.contains('not-clickable') === false)
        {
            href.classList.add('clickable');
        }
        if (href.parentNode.classList.contains('noInitChilds') ||
            href.parentNode.parentNode.classList.contains('noInitChilds'))
        {
            //handler on parentnode
            continue;
        }
        href.addEventListener('click', function(event) {
            parseCmdFromJSON(event, getData(this, 'href'));
        }, false);
    }
    //hide popover
    domCache.body.addEventListener('click', function() {
        hidePopover();
    }, false);
    //init modules
    initBrowse();
    initContextMenuOffcanvas();
    initLocalPlayback();
    initModalAbout();
    initModalEnterPin();
    initModalHomeIcon();
    initModalMaintenance();
    initModalMounts();
    initModalNotifications();
    initModalPartitionOutputs();
    initModalPartitions();
    initModalPlaylistAddTo();
    initModalQueueAddTo();
    initModalQueueSave();
    initModalRadioFavoriteEdit();
    initModalSettings();
    initModalSettingsConnection();
    initModalSettingsPlayback();
    initModalScripts();
    initModalSongDetails();
    initModalTimer();
    initModalTrigger();
    initNavs();
    initOutputs();
    initViewBrowseDatabase();
    initViewBrowseFilesystem();
    initViewBrowseRadioFavorites();
    initViewBrowseRadioRadiobrowser();
    initViewBrowseRadioWebradiodb();
    initViewHome();
    initViewPlayback();
    initViewPlaylists();
    initPresets();
    initSelectActions();
    initViewQueueCurrent();
    initViewQueueJukebox('QueueJukeboxSong');
    initViewQueueJukebox('QueueJukeboxAlbum');
    initViewQueueLastPlayed();
    initViewSearch();
    //init drag and drop
    for (const table of ['QueueCurrentList', 'BrowsePlaylistDetailList']) {
        dragAndDropTable(table);
    }
    const dndTableHeader = [
        'QueueCurrent',
        'QueueLastPlayed',
        'QueueJukeboxSong',
        'QueueJukeboxAlbum',
        'Search',
        'BrowseFilesystem',
        'BrowsePlaylistDetail',
        'BrowseDatabaseAlbumDetail',
        'BrowseRadioWebradiodb',
        'BrowseRadioRadiobrowser'
    ];
    for (const table of dndTableHeader) {
        dragAndDropTableHeader(table);
    }
    //init custom elements
    initElements(domCache.body);
    //add bootstrap native updated event listeners for dropdowns
    const dropdowns = document.querySelectorAll('[data-bs-toggle="dropdown"]');
    for (const dropdown of dropdowns) {
        const positionClass = dropdown.parentNode.classList.contains('dropup')
            ? 'dropup'
            : 'dropdown';
        if (positionClass === 'dropdown') {
            dropdown.parentNode.addEventListener('updated.bs.dropdown', function(event) {
                const menu = event.target.querySelector('.dropdown-menu');
                // reset styles
                menu.style.removeProperty('overflow-y');
                menu.style.removeProperty('overflow-x');
                menu.style.removeProperty('max-height');
                // prevent vertical overflow
                const menuHeight = menu.offsetHeight;
                const offsetY = getYpos(menu);
                const scrollY = getScrollPosY(dropdown);
                const bottomPos = window.innerHeight + scrollY - menuHeight - offsetY;
                if (bottomPos < 0) {
                    menu.style.overflowY = 'auto';
                    menu.style.overflowX = 'hidden';
                    const maxHeight = menuHeight + bottomPos - 10;
                    menu.style.maxHeight = `${maxHeight}px`;
                }
                // prevent horizontal overflow
                const offsetX = getXpos(menu);
                if (offsetX < 0) {
                    menu.style.left = 0;
                }
            }, false);
        }
    }
    //update state on window focus - browser pauses javascript
    window.addEventListener('focus', function() {
        logDebug('Browser tab gots the focus -> update player state');
        sendAPI("MYMPD_API_PLAYER_STATE", {}, parseState, false);
        websocketKeepAlive();
    }, false);
    //global keymap
    document.addEventListener('keydown', function(event) {
        if (event.target.tagName === 'INPUT' ||
            event.target.tagName === 'SELECT' ||
            event.target.tagName === 'TEXTAREA' ||
            event.ctrlKey ||
            event.altKey ||
            event.metaKey)
        {
            return;
        }
        const cmd = keymap[event.key];
        if (cmd &&
            typeof window[cmd.cmd] === 'function')
        {
            if (keymap[event.key].feature === undefined ||
                features[keymap[event.key].feature] === true)
            {
                parseCmd(event, cmd);
            }
            event.stopPropagation();
        }
    }, false);
    //contextmenu for tables
    const tables = ['BrowseFilesystemList', 'BrowseDatabaseAlbumDetailList', 'QueueCurrentList', 'QueueLastPlayedList',
        'QueueJukeboxSongList', 'QueueJukeboxAlbumList', 'SearchList', 'BrowsePlaylistListList', 'BrowsePlaylistDetailList',
        'BrowseRadioRadiobrowserList', 'BrowseRadioWebradiodbList'];
    for (const tableId of tables) {
        const tbody = document.querySelector('#' + tableId + ' > tbody');
        tbody.addEventListener('long-press', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') ||
                event.target.parentNode.parentNode.classList.contains('not-clickable') ||
                getData(event.target.parentNode, 'type') === 'parentDir')
            {
                return;
            }
            showContextMenu(event);
        }, false);

        tbody.addEventListener('contextmenu', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') ||
                event.target.parentNode.parentNode.classList.contains('not-clickable') ||
                getData(event.target.parentNode, 'type') === 'parentDir')
            {
                return;
            }
            showContextMenu(event);
        }, false);
    }
    //contextmenu for grids
    const grids = ['HomeList', 'BrowseDatabaseAlbumListList', 'BrowseRadioFavoritesList'];
    for (const gridId of grids) {
        const gridEl = document.querySelector('#' + gridId);
        gridEl.addEventListener('contextmenu', function(event) {
            if (event.target.classList.contains('card-body') ||
                event.target.classList.contains('card-footer'))
            {
                showContextMenu(event);
            }
        }, false);
    
        gridEl.addEventListener('long-press', function(event) {
            if (event.target.classList.contains('card-body') ||
                event.target.classList.contains('card-footer'))
            {
                showContextMenu(event);
            }
        }, false);
    }

    //websocket
    window.addEventListener('beforeunload', function() {
        webSocketClose();
    });
}

/**
 * Initializes the navigation html elements
 * @returns {void}
 */
function initNavs() {
    domCache.progress.addEventListener('click', function(event) {
        if (currentState.currentSongId >= 0 &&
            currentState.totalTime > 0)
        {
            const seekVal = Math.ceil((currentState.totalTime * event.clientX) / domCache.progress.offsetWidth);
            sendAPI("MYMPD_API_PLAYER_SEEK_CURRENT", {
                "seek": seekVal,
                "relative": false
            }, null, false);
        }
    }, false);

    domCache.progress.addEventListener('mousemove', function(event) {
        if ((currentState.state === 'pause' || currentState.state === 'play') &&
            currentState.totalTime > 0)
        {
            domCache.progressPos.textContent = fmtSongDuration(Math.ceil((currentState.totalTime / event.target.offsetWidth) * event.clientX));
            domCache.progressPos.style.display = 'block';
            const w = domCache.progressPos.offsetWidth / 2;
            const posX = event.clientX < w ? event.clientX : (event.clientX < window.innerWidth - w ? event.clientX - w : event.clientX - (w * 2));
            domCache.progressPos.style.left = posX + 'px';
        }
    }, false);

    domCache.progress.addEventListener('mouseout', function() {
        domCache.progressPos.style.display = 'none';
    }, false);

    const navbarMain = elGetById('navbar-main');
    navbarMain.addEventListener('click', function(event) {
        event.preventDefault();
        if (event.target.nodeName === 'DIV') {
            return;
        }
        const target = event.target.nodeName === 'A' ? event.target : event.target.parentNode;
        const href = getData(target, 'href');
        if (href === undefined) {
            return;
        }
        parseCmd(event, href);
    }, false);

    navbarMain.addEventListener('contextmenu', function(event) {
        if (event.target.getAttribute('data-contextmenu') === null &&
            event.target.parentNode.getAttribute('data-contextmenu') === null)
        {
            return;
        }
        showContextMenu(event);
    }, false);
    navbarMain.addEventListener('long-press', function(event) {
        if (event.target.getAttribute('data-contextmenu') === null &&
            event.target.parentNode.getAttribute('data-contextmenu') === null)
        {
            return;
        }
        showContextMenu(event);
    }, false);

    elGetById('scripts').addEventListener('click', function(event) {
        event.preventDefault();
        const target = event.target.nodeName === 'SPAN' ? event.target.parentNode : event.target;
        if (target.nodeName === 'A') {
            target.firstElementChild.textContent = 'start';
            setTimeout(function() {
                target.firstElementChild.textContent = 'code';
            }, 400);
            execScript(getData(target, 'href'));
        }
    }, false);
}

/**
 * Gets the initial assets
 * @returns {void}
 */
function getAssets() {
    httpGet(subdir + '/assets/i18n/en-US.json', function(obj) {
        phrasesDefault = obj;
    }, true);
    httpGet(subdir + '/assets/ligatures.json', function(obj) {
        materialIcons = obj;
    }, true);
}

/**
 * Handle javascript errors
 * @param {string} msg error message
 * @param {string} url url of the error
 * @param {number} line line of the error
 * @param {number} col column of the error
 * @returns {boolean} false
 */
window.onerror = function(msg, url, line, col) {
    if (settings.loglevel >= 4) {
        showNotification(tn('JavaScript error') + ': ' + msg + ' (' + url + ': ' + line + ':' + col + ')', 'general', 'error');
    }
    //show error also in the console
    return false;
};

/**
 * Configure trusted types to allow service worker registration
 */
if (window.trustedTypes &&
    window.trustedTypes.createPolicy)
{
    window.trustedTypes.createPolicy('default', {
        createScriptURL(dirty) {
            if (dirty === 'sw.js') {
                return 'sw.js';
            }
            throw new Error('Script not allowed: ' + dirty);
       }
    });
}

//Start app
appInitStart();
