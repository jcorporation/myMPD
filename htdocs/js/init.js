"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module init_js */

/**
 * Initializes / stars the myMPD app
 */

/**
 * Shows an error message on the splashscreen
 * @param {string} text message to display (untranslated)
 */
 function showAppInitAlert(text) {
    const spa = document.getElementById('splashScreenAlert');
    elClear(spa);
    spa.appendChild(
        elCreateTextTn('p', {"class": ["text-light"]}, text)
    );
    const reloadBtn = elCreateTextTn('button', {"class": ["btn", "btn-light", "me-2"]}, 'Reload');
    reloadBtn.addEventListener('click', function() {
        clearAndReload();
    }, false);
    const resetBtn = elCreateTextTn('button', {"class": ["btn", "btn-light"]}, 'Reset');
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
 */
function clearAndReload() {
    clearCache();
    location.reload();
}

/**
 * Initializes the add to homescreen function
 */
function a2hsInit() {
    window.addEventListener('beforeinstallprompt', function(event) {
        logDebug('Event: beforeinstallprompt');
        // Prevent Chrome 67 and earlier from automatically showing the prompt
        event.preventDefault();
        // Stash the event so it can be triggered later
        deferredA2HSprompt = event;
        // Update UI notify the user they can add to home screen
        elShowId('nav-add2homescreen');
    });

    document.getElementById('nav-add2homescreen').addEventListener('click', function(event) {
        // Hide our user interface that shows our A2HS button
        elHide(event.target);
        // Show the prompt
        deferredA2HSprompt.prompt();
        // Wait for the user to respond to the prompt
        deferredA2HSprompt.userChoice.then(() => {
            deferredA2HSprompt = null;
        });
    });

    window.addEventListener('appinstalled', function() {
        showNotification(tn('myMPD installed as app'), '', 'general', 'info');
    });
}

/**
 * Starts the app
 */
function appInitStart() {
    //add app routing event handler
    window.addEventListener('hashchange', function() {
        if (app.goto === false) {
            appRoute();
        }
        else {
            app.goto = false;
        }
    }, false);

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
    }

    //update table height on window resize
    window.addEventListener('resize', function() {
        if (resizeTimer !== null) {
            clearTimeout(resizeTimer);
        }
        resizeTimer = setTimeout(function() {
            const list = document.getElementById(app.id + 'List');
            if (list) {
                setScrollViewHeight(list);
            }
            resizeTimer = null;
        }, 100);
    }, false);

    setMobileView();

    subdir = window.location.pathname.replace('/index.html', '').replace(/\/$/, '');
    i18nHtml(document.getElementById('splashScreenAlert'));

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
    document.getElementById('splashScreenAlert').textContent = tn('Fetch myMPD settings');

    //init add to home screen feature
    a2hsInit();

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
            document.getElementById('splashScreenAlert').textContent = tn('Applying settings');
            document.getElementById('splashScreen').classList.add('hide-fade');
            setTimeout(function() {
                elHideId('splashScreen');
                document.getElementById('splashScreen').classList.remove('hide-fade');
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
 */
function appInit() {
    getAssets();
    //collapse arrows for submenus
    const collapseArrows = document.querySelectorAll('.subMenu');
    for (const collapseArrow of collapseArrows) {
        collapseArrow.addEventListener('click', function(event) {
            event.stopPropagation();
            event.preventDefault();
            toggleCollapseArrow(this);
        }, false);
    }
    //init links
    const hrefs = document.querySelectorAll('[data-href]');
    for (const href of hrefs) {
        if (href.classList.contains('not-clickable') === false) {
            href.classList.add('clickable');
        }
        let parentInit = href.parentNode.classList.contains('noInitChilds') ? true : false;
        if (parentInit === false) {
            parentInit = href.parentNode.parentNode.classList.contains('noInitChilds') ? true : false;
        }
        if (parentInit === true) {
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
    initGlobalModals();
    initSong();
    initHome();
    initBrowse();
    initBrowseDatabase();
    initBrowseFilesystem();
    initBrowseRadioFavorites();
    initBrowseRadioRadiobrowser();
    initBrowseRadioWebradiodb();
    initQueueCurrent();
    initQueueJukebox();
    initQueueLastPlayed();
    initSearch();
    initScripts();
    initTrigger();
    initTimer();
    initPartitions();
    initMounts();
    initSettings();
    initSettingsConnection();
    initSettingsPlayback();
    initMaintenance();
    initPlayback();
    initNavs();
    initPlaylists();
    initOutputs();
    initLocalPlayback();
    initSession();
    //init drag and drop
    for (const table of ['QueueCurrentList', 'BrowsePlaylistDetailList']) {
        dragAndDropTable(table);
    }
    const dndTableHeader = [
        'QueueCurrent',
        'QueueLastPlayed',
        'QueueJukebox',
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
    //update state on window focus - browser pauses javascript
    window.addEventListener('focus', function() {
        logDebug('Browser tab gots the focus -> update player state');
        sendAPI("MYMPD_API_PLAYER_STATE", {}, parseState, false);
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
        if (cmd && typeof window[cmd.cmd] === 'function') {
            if (keymap[event.key].req === undefined ||
                settings[keymap[event.key].req] === true)
            {
                parseCmd(event, cmd);
            }
            event.stopPropagation();
        }
    }, false);
    //contextmenu for tables
    const tables = ['BrowseFilesystemList', 'BrowseDatabaseAlbumDetailList', 'QueueCurrentList', 'QueueLastPlayedList',
        'QueueJukeboxList', 'SearchList', 'BrowsePlaylistListList', 'BrowsePlaylistDetailList',
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
            showPopover(event);
        }, false);

        tbody.addEventListener('contextmenu', function(event) {
            if (event.target.parentNode.classList.contains('not-clickable') ||
                event.target.parentNode.parentNode.classList.contains('not-clickable') ||
                getData(event.target.parentNode, 'type') === 'parentDir')
            {
                return;
            }
            showPopover(event);
        }, false);
    }

    //websocket
    window.addEventListener('beforeunload', function() {
        webSocketClose();
    });
}

/**
 * Initializes the html elements
 */
function initGlobalModals() {
    const tab = document.getElementById('tabShortcuts');
    elClear(tab);
    const keys = Object.keys(keymap).sort((a, b) => {
        return keymap[a].order - keymap[b].order
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
        const k = elCreateText('div', {"class": ["key", "float-start"]}, (keymap[key].key !== undefined ? keymap[key].key : key));
        if (keymap[key].key && keymap[key].key.length > 1) {
            k.classList.add('mi', 'mi-small');
        }
        col.appendChild(k);
        col.appendChild(
            elCreateTextTn('div', {}, keymap[key].desc)
        );
        tab.lastChild.appendChild(col);
    }

    document.getElementById('modalAbout').addEventListener('show.bs.modal', function () {
        sendAPI("MYMPD_API_DATABASE_STATS", {}, parseStats, false);
        getServerinfo();
    }, false);
}

/**
 * Initializes the navigation html elements
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

    domCache.progressBar.style.transition = progressBarTransition;

    const navbarMain = document.getElementById('navbar-main');
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
        if (event.target.getAttribute('data-popover') === null &&
            event.target.parentNode.getAttribute('data-popover') === null)
        {
            return;
        }
        showPopover(event);
    }, false);
    navbarMain.addEventListener('long-press', function(event) {
        if (event.target.getAttribute('data-popover') === null &&
            event.target.parentNode.getAttribute('data-popover') === null)
        {
            return;
        }
        showPopover(event);
    }, false);

    document.getElementById('scripts').addEventListener('click', function(event) {
        event.preventDefault();
        if (event.target.nodeName === 'A') {
            execScript(getData(event.target, 'href'));
        }
    }, false);

    //update list of notifications
    document.getElementById('menu-notifications').addEventListener('show.bs.collapse', function() {
        showMessages();
    }, false);
    document.getElementById('offcanvasMenu').addEventListener('show.bs.offcanvas', function() {
        showMessages();
    }, false);
}

/**
 * Gets the initial assets
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
 */
if (debugMode === false) {
    window.onerror = function(msg, url, line) {
        logError('JavaScript error: ' + msg + ' (' + url + ': ' + line + ')');
        if (settings.loglevel >= 4) {
            showNotification(tn('JavaScript error'), msg + ' (' + url + ': ' + line + ')', 'general', 'error');
        }
        return true;
    };
}

/**
 * Configure trusted types to allow service worker registration
 */
if (window.trustedTypes &&
    window.trustedTypes.createPolicy)
{
    window.trustedTypes.createPolicy('default', {
        createScriptURL(dirty) {
            if (dirty === 'sw.js') {
                return 'sw.js'
            }
            throw new Error('Script not allowed: ' + dirty);
       }
    });
}

//Start app
appInitStart();
