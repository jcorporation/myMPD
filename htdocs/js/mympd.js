"use strict";
/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:

   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: https://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

var socket;
var lastSong = '';
var lastState;
var currentSong = new Object();
var playstate = '';
var settings = {};
var alertTimeout;
var progressTimer;
let deferredPrompt;
var dragEl;

var app = {};
app.apps = { "Playback": { "state": "0/-/", "scrollPos": 0 },
             "Queue": 	 { "state": "0/any/", "scrollPos": 0 },
             "Browse":   { 
                  "active": "Database", 
                  "tabs":  { "Filesystem": { "state": "0/-/", "scrollPos": 0 },
                             "Playlists":  { 
                                    "active": "All",
                                    "views": { "All":    { "state": "0/-/", "scrollPos": 0 },
                                               "Detail": { "state": "0/-/", "scrollPos": 0 }
                                    }
                             },
                             "Database":   { 
                                    "active": "AlbumArtist",
                                    "views": { "AlbumArtist": { "state": "0/-/", "scrollPos": 0 },
                                               "Genre": { "state": "0/-/", "scrollPos": 0 },
                                               "Artist": { "state": "0/-/", "scrollPos": 0 },
                                               "Composer": { "state": "0/-/", "scrollPos": 0 },
                                               "Performer": { "state": "0/-/", "scrollPos": 0 },
                                               "Date": { "state": "0/-/", "scrollPos": 0 },
                                               "Album":  { "state": "0/-/", "scrollPos": 0 },
                                     }
                             }
                  }
             },
             "Search": { "state": "0/any/", "scrollPos": 0 }
           };
           
app.current = { "app": "Playback", "tab": undefined, "view": undefined, "page": 0, "filter": "", "search": "", "scrollPos": 0 };
app.last = { "app": undefined, "tab": undefined, "view": undefined, "filter": "", "search": "",  "scrollPos": 0 };

var domCache = {};
domCache.navbarBottomBtns = document.getElementById('navbar-bottom').getElementsByTagName('div');
domCache.navbarBottomBtnsLen = domCache.navbarBottomBtns.length;
domCache.panelHeadingBrowse = document.getElementById('panel-heading-browse').getElementsByTagName('a');
domCache.panelHeadingBrowseLen = domCache.panelHeadingBrowse.length;
domCache.counter = document.getElementById('counter');
domCache.volumePrct = document.getElementById('volumePrct');
domCache.volumeControl = document.getElementById('volumeControl');
domCache.volumeIcon = document.getElementById('volumeIcon');
domCache.btnsPlay = document.getElementsByClassName('btnPlay');
domCache.btnsPlayLen = domCache.btnsPlay.length;
domCache.btnPrev = document.getElementById('btnPrev');
domCache.btnNext = document.getElementById('btnNext');
domCache.progressBar = document.getElementById('progressBar');
domCache.volumeBar = document.getElementById('volumeBar');
domCache.outputs = document.getElementById('outputs');
domCache.btnAdd = document.getElementById('nav-add2homescreen');
domCache.currentTrack = document.getElementById('currentTrack');
domCache.currentArtist = document.getElementById('currentArtist');
domCache.currentAlbum = document.getElementById('currentAlbum');
domCache.currentCover = document.getElementById('currentCover');
domCache.btnVoteUp = document.getElementById('btnVoteUp');
domCache.btnVoteDown = document.getElementById('btnVoteDown');

var modalConnectionError = new Modal(document.getElementById('modalConnectionError'));
var modalSettings = new Modal(document.getElementById('modalSettings'));
var modalSavequeue = new Modal(document.getElementById('modalSaveQueue'));
var modalSongDetails = new Modal(document.getElementById('modalSongDetails'));
var modalAddToPlaylist = new Modal(document.getElementById('modalAddToPlaylist'));
var modalRenamePlaylist = new Modal(document.getElementById('modalRenamePlaylist'));
var modalUpdateDB = new Modal(document.getElementById('modalUpdateDB'));
//var mainMenu = new Dropdown(document.getElementById('mainMenu'));
//var volumeMenu = new Dropdown(document.getElementById('volumeIcon'));

function appPrepare(scrollPos) {
    if (app.current.app != app.last.app || app.current.tab != app.last.tab || app.current.view != app.last.view) {
        //Hide all cards + nav
        for (var i = 0; i < domCache.navbarBottomBtnsLen; i++) {
            domCache.navbarBottomBtns[i].classList.remove('active');
        }
        document.getElementById('cardPlayback').classList.add('hide');
        document.getElementById('cardQueue').classList.add('hide');
        document.getElementById('cardBrowse').classList.add('hide');
        document.getElementById('cardSearch').classList.add('hide');
        for (var i = 0; i < domCache.panelHeadingBrowseLen; i++) {
            domCache.panelHeadingBrowse[i].classList.remove('active');
        }
        document.getElementById('cardBrowsePlaylists').classList.add('hide');
        document.getElementById('cardBrowseDatabase').classList.add('hide');        
        document.getElementById('cardBrowseFilesystem').classList.add('hide');        
        //show active card + nav
        document.getElementById('card' + app.current.app).classList.remove('hide');
        document.getElementById('nav' + app.current.app).classList.add('active');
        if (app.current.tab != undefined) {
            document.getElementById('card' + app.current.app + app.current.tab).classList.remove('hide');
            document.getElementById('card' + app.current.app + 'Nav' + app.current.tab).classList.add('active');    
        }
        scrollTo(scrollPos);
    }
    var list = document.getElementById(app.current.app + 
        (app.current.tab == undefined ? '' : app.current.tab) + 
        (app.current.view == undefined ? '' : app.current.view) + 'List');
    if (list)
        list.classList.add('opacity05');
}

function appGoto(a,t,v,s) {
    var scrollPos = 0;
    if (document.body.scrollTop)
        scrollPos = document.body.scrollTop
    else 
        scrollPos = document.documentElement.scrollTop;
        
    if (app.apps[app.current.app].scrollPos != undefined)
        app.apps[app.current.app].scrollPos = scrollPos
    else if (app.apps[app.current.app].tabs[app.current.tab].scrollPos != undefined)
        app.apps[app.current.app].tabs[app.current.tab].scrollPos = scrollPos
    else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos != undefined)
        app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos = scrollPos;

    var hash = '';
    if (app.apps[a].tabs) {
        if (t == undefined) 
            t = app.apps[a].active;
        if (app.apps[a].tabs[t].views) {
            if (v == undefined) 
                v = app.apps[a].tabs[t].active;
            hash = '/' + a + '/' + t +'/'+v + '!' + (s == undefined ? app.apps[a].tabs[t].views[v].state : s);
        } else {
            hash = '/'+a+'/'+t+'!'+ (s == undefined ? app.apps[a].tabs[t].state : s);
        }
    } else {
        hash = '/' + a + '!'+ (s == undefined ? app.apps[a].state : s);
    }
    location.hash = hash;
}

function appRoute() {
    var hash = decodeURI(location.hash);
    var params;
    if (params = hash.match(/^\#\/(\w+)\/?(\w+)?\/?(\w+)?\!((\d+)\/([^\/]+)\/(.*))$/)) {
        app.current.app = params[1];
        app.current.tab = params[2];
        app.current.view = params[3];
        if (app.apps[app.current.app].state) {
            app.apps[app.current.app].state = params[4];
            app.current.scrollPos = app.apps[app.current.app].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].state) {
            app.apps[app.current.app].tabs[app.current.tab].state = params[4];
            app.apps[app.current.app].active = app.current.tab;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].scrollPos;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state) {
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state = params[4];
            app.apps[app.current.app].active = app.current.tab;
            app.apps[app.current.app].tabs[app.current.tab].active = app.current.view;
            app.current.scrollPos = app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].scrollPos;
        }
        app.current.page = parseInt(params[5]);
        app.current.filter = params[6];
        app.current.search = params[7];
    } else {
        appGoto('Playback');
        return;
    }

    appPrepare(app.current.scrollPos);

    if (app.current.app == 'Playback') {
        sendAPI({"cmd": "MPD_API_PLAYER_CURRENT_SONG"}, songChange);
    }    
    else if (app.current.app == 'Queue' ) {
        selectTag('searchqueuetag', 'searchqueuetagdesc', app.current.filter);
        getQueue();
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists' && app.current.view == 'All') {
        sendAPI({"cmd": "MPD_API_PLAYLIST_LIST", "data": {"offset": app.current.page, "filter": app.current.filter}}, parsePlaylists);
        doSetFilterLetter('BrowsePlaylistsFilter');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists' && app.current.view == 'Detail') {
        sendAPI({"cmd": "MPD_API_PLAYLIST_CONTENT_LIST", "data": {"offset": app.current.page, "filter": app.current.filter, "uri": app.current.search}}, parsePlaylists);
        doSetFilterLetter('BrowsePlaylistsFilter');
    }    

    else if (app.current.app == 'Browse' && app.current.tab == 'Database') {
        if (app.current.search != '') {
            sendAPI({"cmd": "MPD_API_DATABASE_TAG_ALBUM_LIST", "data": {"offset": app.current.page, "filter": app.current.filter, "search": app.current.search, "tag": app.current.view}}, parseListDBtags);
            doSetFilterLetter('BrowseDatabaseFilter');
        }
        else {
            sendAPI({"cmd": "MPD_API_DATABASE_TAG_LIST","data": {"offset": app.current.page, "filter": app.current.filter, "tag": app.current.view}}, parseListDBtags);
            doSetFilterLetter('BrowseDatabaseFilter');
            selectTag('BrowseDatabaseByTagDropdown', 'btnBrowseDatabaseByTag', app.current.view);
        }
    }    
    else if (app.current.app == 'Browse' && app.current.tab == 'Filesystem') {
        sendAPI({"cmd": "MPD_API_DATABASE_FILESYSTEM_LIST", "data": {"offset": app.current.page, "path": (app.current.search ? app.current.search : "/"), "filter": app.current.filter}}, parseFilesystem);
        // Don't add all songs from root
        if (app.current.search) {
            document.getElementById('BrowseFilesystemAddAllSongs').removeAttribute('disabled');
            document.getElementById('BrowseFilesystemAddAllSongsBtn').removeAttribute('disabled');
        }
        else {
            document.getElementById('BrowseFilesystemAddAllSongs').setAttribute('disabled', 'disabled');
            document.getElementById('BrowseFilesystemAddAllSongsBtn').setAttribute('disabled', 'disabled');
        }
        // Create breadcrumb
        var breadcrumbs='<li class="breadcrumb-item"><a data-uri="">root</a></li>';
        var pathArray = app.current.search.split('/');
        var pathArrayLen = pathArray.length;
        var fullPath = '';
        for (var i = 0; i < pathArrayLen; i++) {
            if (pathArrayLen -1 == i) {
                breadcrumbs += '<li class="breadcrumb-item active">' + pathArray[i] + '</li>';
                break;
            }
            fullPath += pathArray[i];
            breadcrumbs += '<li class="breadcrumb-item"><a data-uri="' + fullPath + '">' + pathArray[i] + '</a></li>';
            fullPath += '/';
        }
        var elBrowseBreadcrumb=document.getElementById('BrowseBreadcrumb');
        elBrowseBreadcrumb.innerHTML = breadcrumbs;
        var breadcrumbItems = elBrowseBreadcrumb.getElementsByTagName('a');
        var breadcrumbItemsLen = breadcrumbItems.length;
        for (var i = 0; i < breadcrumbItemsLen; i++) {
            breadcrumbItems[i].addEventListener('click', function() {
	        appGoto('Browse', 'Filesystem', undefined, '0/' + app.current.filter + '/' + this.getAttribute('data-uri'));
            }, false);
        }
        doSetFilterLetter('BrowseFilesystemFilter');
    }
    else if (app.current.app == 'Search') {
        document.getElementById('searchstr').focus();
        if (app.last.app != app.current.app) {
            if (app.current.search != '')
                document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                    '<tr><td><span class="material-icons">search</span></td>' +
                    '<td colspan="5">Searching...</td></tr>';
        }

        if (app.current.search.length >= 2) {
            sendAPI({"cmd": "MPD_API_DATABASE_SEARCH", "data": { "plist": "", "offset": app.current.page, "filter": app.current.filter, "searchstr": app.current.search}}, parseSearch);
        } else {
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML = '';
            document.getElementById('searchAddAllSongs').setAttribute('disabled', 'disabled');
            document.getElementById('searchAddAllSongsBtn').setAttribute('disabled', 'disabled');
            document.getElementById('panel-heading-search').innerText = '';
            document.getElementById('SearchList').classList.remove('opacity05');
            setPagination(0);
        }
        selectTag('searchtags', 'searchtagsdesc', app.current.filter);
    }
    else {
        appGoto("Playback");
    }

    app.last.app = app.current.app;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
};

function appInit() {
    getSettings();
    sendAPI({"cmd": "MPD_API_PLAYER_STATE"}, parseState);

    webSocketConnect();

    domCache.volumeBar.value = 0;
    domCache.volumeBar.addEventListener('click', function(event) {
        event.stopPropagation();
    }, false);
    domCache.volumeBar.addEventListener('change', function(event) {
        sendAPI({"cmd": "MPD_API_PLAYER_VOLUME", "data": {"volume": domCache.volumeBar.value}});
    }, false);

    domCache.progressBar.value = 0;
    domCache.progressBar.addEventListener('change', function(event) {
        if (currentSong && currentSong.currentSongId >= 0) {
            var seekVal = Math.ceil(currentSong.totalTime * (domCache.progressBar.value / 100));
            sendAPI({"cmd": "MPD_API_PLAYER_SEEK", "data": {"songid": currentSong.currentSongId, "seek": seekVal}});
        }
    }, false);
  
    document.getElementById('volumeIcon').parentNode.addEventListener('show.bs.dropdown', function () {
        sendAPI({"cmd": "MPD_API_PLAYER_OUTPUT_LIST"}, parseOutputs);
    });    
    
    document.getElementById('modalAbout').addEventListener('shown.bs.modal', function () {
        sendAPI({"cmd": "MPD_API_DATABASE_STATS"}, parseStats);
    });

    document.getElementById('modalUpdateDB').addEventListener('hidden.bs.modal', function () {
        document.getElementById('updateDBprogress').classList.remove('updateDBprogressAnimate');
    });
    
    document.getElementById('modalSaveQueue').addEventListener('shown.bs.modal', function () {
        var plName = document.getElementById('saveQueueName');
        plName.focus();
        plName.value = '';
        plName.classList.remove('is-invalid');
        document.getElementById('saveQueueFrm').classList.remove('was-validated');
    });
        
    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        getSettings();
        document.getElementById('settingsFrm').classList.remove('was-validated');
        document.getElementById('inputCrossfade').classList.remove('is-invalid');
        document.getElementById('inputMixrampdb').classList.remove('is-invalid');
        document.getElementById('inputMixrampdelay').classList.remove('is-invalid');
    });


    document.getElementById('addToPlaylistPlaylist').addEventListener('change',function(event) {
        if (this.options[this.selectedIndex].text == 'New Playlist') {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.remove('hide');
            document.getElementById('addToPlaylistNewPlaylist').focus();
        }
        else {
            document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
        }
    }, false);
    
    addFilterLetter('BrowseFilesystemFilterLetters');
    addFilterLetter('BrowseDatabaseFilterLetters');
    addFilterLetter('BrowsePlaylistsFilterLetters');

    var hrefs = document.querySelectorAll('[data-href]');
    var hrefsLen = hrefs.length;
    for (var i = 0; i < hrefsLen; i++) {
        hrefs[i].classList.add('clickable');
        hrefs[i].addEventListener('click', function(event) {
            event.preventDefault();
            //event.stopPropagation();
            var cmd = JSON.parse(this.getAttribute('data-href'));
            if (typeof window[cmd.cmd] === 'function') {
                switch(cmd.cmd) {
                    case 'sendAPI':
                        sendAPI(... cmd.options); 
                    break;
                    default:
                    window[cmd.cmd](... cmd.options);                    
                }
            }
        }, false);
    }

    var pd = document.getElementsByClassName('pages');
    var pdLen = pd.length;
    for (var i = 0; i < pdLen; i++) {
        pd[i].addEventListener('click', function(event) {
            if (event.target.nodeName == 'BUTTON') {
                gotoPage(event.target.getAttribute('data-page'));
            }
        }, false);
    }

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON') 
            event.stopPropagation();
            sendAPI({"cmd": "MPD_API_PLAYER_TOGGLE_OUTPUT", "data": {"output": event.target.getAttribute('data-output-id'), "state": (event.target.classList.contains('active') ? 0 : 1)}});
            toggleBtn(event.target.id);
    }, false);
    
    document.getElementById('QueueList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') 
            sendAPI({"cmd": "MPD_API_PLAYER_PLAY_TRACK","data": {"track": event.target.parentNode.getAttribute('data-trackid')}});
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            switch(event.target.parentNode.getAttribute('data-type')) {
                case 'dir':
                    appGoto('Browse', 'Filesystem', undefined, '0/' + app.current.filter +'/' + decodeURI(event.target.parentNode.getAttribute("data-uri")));
                    break;
                case 'song':
                    appendQueue('song', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
                    break;
                case 'plist':
                    appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
                    break;
            }
        }
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);
    
    document.getElementById('BrowsePlaylistsAllList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowsePlaylistsDetailList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);    
    
    document.getElementById('BrowseDatabaseTagList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appGoto('Browse', 'Database', app.current.view, '0/-/' + event.target.parentNode.getAttribute('data-uri'));
        }
    }, false);
    
    document.getElementById('SearchList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);

    document.getElementById('BrowseFilesystemAddAllSongsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON') {
            if (event.target.innerText == 'Add all to queue') {
                addAllFromBrowse();
            }
            else if (event.target.innerText == 'Add all to playlist') {
                showAddToPlaylist(app.current.search);                
            }
        }
    }, false);

    document.getElementById('searchAddAllSongsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON') {
            if (event.target.innerText == 'Add all to queue') {
                addAllFromSearchPlist('queue');
            }
            else if (event.target.innerText == 'Add all to playlist') {
                showAddToPlaylist('SEARCH');                
            }
        }
    }, false);
    
    document.getElementById('BrowseDatabaseAddAllSongsDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON') {
            if (event.target.innerText == 'Add all to queue') {
                addAllFromBrowseDatabasePlist('queue');
            }
            else if (event.target.innerText == 'Add all to playlist') {
                showAddToPlaylist('DATABASE');
            }
        }
    }, false);

    document.getElementById('searchtags').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON')
            appGoto(app.current.app, app.current.tab, app.current.view, '0/' + event.target.getAttribute('data-tag') + '/' + app.current.search);            
    }, false);

    document.getElementById('searchqueuestr').addEventListener('keyup', function(event) {
        appGoto(app.current.app, app.current.tab, app.current.view, '0/' + app.current.filter + '/' + this.value);
    }, false);

    document.getElementById('searchqueuetag').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON')
            appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + event.target.getAttribute('data-tag') + '/' + app.current.search);
    }, false);

    document.getElementById('search').addEventListener('submit', function() {
        return false;
    }, false);

    document.getElementById('searchqueue').addEventListener('submit', function() {
        return false;
    }, false);

    document.getElementById('searchstr').addEventListener('keyup', function(event) {
        appGoto('Search', undefined, undefined, '0/' + app.current.filter + '/' + this.value);
    }, false);

    document.getElementById('BrowseDatabaseByTagDropdown').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON')
            appGoto(app.current.app, app.current.tab, event.target.getAttribute('data-tag') , '0/' + app.current.filter  + '/' + app.current.search);
    }, false);

    document.getElementsByTagName('body')[0].addEventListener('click', function(event) {
        var oldPopover = document.getElementsByClassName('popover');
        for (var i = 0; i < oldPopover.length; i++)
            oldPopover[i].remove();
    }, false);

    dragAndDropTable('QueueList');
    dragAndDropTable('BrowsePlaylistsDetailList');

    window.addEventListener('hashchange', appRoute, false);

    window.addEventListener('focus', function() {
        sendAPI({"cmd": "MPD_API_PLAYER_STATE"}, parseState);
    }, false);
    
    document.addEventListener('keydown', function(event) {
        if (event.target.tagName == 'INPUT')
            return;
        switch (event.which) {
            case 37: //left
                clickPrev();
                break;
            case 39: //right
                clickNext();
                break;
            case 32: //space
                clickPlay();
                break;
            default:
                return;
        }
        event.preventDefault();
    }, false);
    
    if ('serviceWorker' in navigator && document.URL.substring(0, 5) == 'https') {
        window.addEventListener('load', function() {
            navigator.serviceWorker.register('/sw.js', {scope: '/'}).then(function(registration) {
                // Registration was successful
                console.log('ServiceWorker registration successful with scope: ', registration.scope);
                registration.update();
            }, function(err) {
                // Registration failed
                console.log('ServiceWorker registration failed: ', err);
            });
        });
    }
    
    window.addEventListener('beforeinstallprompt', function(event) {
        // Prevent Chrome 67 and earlier from automatically showing the prompt
        event.preventDefault();
        // Stash the event so it can be triggered later.
        deferredPrompt = event;
    });
    
    window.addEventListener('beforeinstallprompt', function(event) {
        event.preventDefault();
        deferredPrompt = event;
        // Update UI notify the user they can add to home screen
        domCache.btnAdd.classList.remove('hide');
    });
    
    domCache.btnAdd.addEventListener('click', function(event) {
        // Hide our user interface that shows our A2HS button
        domCache.btnAdd.classList.add('hide');
        // Show the prompt
        deferredPrompt.prompt();
        // Wait for the user to respond to the prompt
        deferredPrompt.userChoice.then((choiceResult) => {
            if (choiceResult.outcome === 'accepted')
                console.log('User accepted the A2HS prompt');
            else
                console.log('User dismissed the A2HS prompt');
            deferredPrompt = null;
        });
    });
    
    window.addEventListener('appinstalled', function(event) {
        console.log('myMPD installed as app');
    });
}

function dragAndDropTable(table) {
    var tableBody=document.getElementById(table).getElementsByTagName('tbody')[0];
    tableBody.addEventListener('dragstart', function(event) {
        if (event.target.nodeName == 'TR') {
            event.target.classList.add('opacity05');
            event.dataTransfer.setDragImage(event.target, 0, 0);
            event.dataTransfer.effectAllowed = 'move';
            event.dataTransfer.setData('Text', event.target.getAttribute('id'));
            dragEl = event.target.cloneNode(true);
        }
    }, false);
    tableBody.addEventListener('dragleave', function(event) {
        event.preventDefault();        
        var target = event.target;
        if (event.target.nodeName == 'TD')
            target = event.target.parentNode;
        if (target.nodeName == 'TR')
            target.classList.remove('dragover');
    }, false);
    tableBody.addEventListener('dragover', function(event) {
        event.preventDefault();
        var tr = tableBody.getElementsByClassName('dragover');
        var trLen = tr.length;
        for (var i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        var target = event.target;
        if (event.target.nodeName == 'TD')
            target = event.target.parentNode;
        if (target.nodeName == 'TR')
            target.classList.add('dragover');
        event.dataTransfer.dropEffect = 'move';
    }, false);
    tableBody.addEventListener('dragend', function(event) {
        var tr = tableBody.getElementsByClassName('dragover');
        var trLen = tr.length;
        for (var i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        if (document.getElementById(event.dataTransfer.getData('Text')))
            document.getElementById(event.dataTransfer.getData('Text')).classList.remove('opacity05');
    }, false);
    tableBody.addEventListener('drop', function(event) {
        event.stopPropagation();
        event.preventDefault();
        var target = event.target;
        if (event.target.nodeName == 'TD')
            target = event.target.parentNode;
        var oldSongpos = document.getElementById(event.dataTransfer.getData('Text')).getAttribute('data-songpos');
        var newSongpos = target.getAttribute('data-songpos');
        document.getElementById(event.dataTransfer.getData('Text')).remove();
        dragEl.classList.remove('opacity05');
        tableBody.insertBefore(dragEl, target);
        var tr = tableBody.getElementsByClassName('dragover');
        var trLen = tr.length;
        for (var i = 0; i < trLen; i++) {
            tr[i].classList.remove('dragover');
        }
        document.getElementById(table).classList.add('opacity05');
        if (app.current.app == 'Queue')
            sendAPI({"cmd": "MPD_API_QUEUE_MOVE_TRACK","data": {"from": oldSongpos, "to": newSongpos}});
        else if (app.current.app == 'Browse' && app.current.tab == 'Playlists' && app.current.view == 'Detail')
            playlistMoveTrack(oldSongpos, newSongpos);        
    }, false);
}

function playlistMoveTrack(from, to) {
    sendAPI({"cmd": "MPD_API_PLAYLIST_MOVE_TRACK","data": { "plist": app.current.search, "from": from, "to": to}});
    sendAPI({"cmd": "MPD_API_PLAYLIST_CONTENT_LIST","data": {"offset": app.current.page, "filter": app.current.filter, "uri": app.current.search}}, parsePlaylists);
}

function webSocketConnect() {
    socket = new WebSocket(getWsUrl());

    try {
        socket.onopen = function() {
            console.log('connected');
            showNotification('Connected to myMPD', '', '', 'success');
            modalConnectionError.hide();
            appRoute();
            sendAPI({"cmd": "MPD_API_PLAYER_STATE"}, parseState);
        }

        socket.onmessage = function got_packet(msg) {
            if (msg.data === lastState || msg.data.length == 0)
                return;
                
            try {
                var obj = JSON.parse(msg.data);
            } catch(e) {
                console.log('Invalid JSON data received: ' + msg.data);
            }

            switch (obj.type) {
                case 'update_state':
                    parseState(obj);
                    break;
                case 'disconnected':
                    showNotification('myMPD lost connection to MPD', '', '', 'danger');
                    break;
                case 'update_queue':
                    if (app.current.app === 'Queue')
                        getQueue();
                    sendAPI({"cmd": "MPD_API_PLAYER_STATE"}, parseState);
                    break;
                case 'update_options':
                    getSettings();
                    break;
                case 'update_outputs':
                    sendAPI({"cmd": "MPD_API_PLAYER_OUTPUT_LIST"}, parseOutputs);
                    break;
                case 'update_started':
                    updateDBstarted(false);
                    break;
                case 'update_database':
                case 'update_finished':
                    updateDBfinished(obj.type);
                    break;
                case 'error':
                    showNotification(obj.data, '', '', 'danger');
                    break;
                default:
                    break;
            }
        }

        socket.onclose = function(){
            console.log('disconnected');
            modalConnectionError.show();
            setTimeout(function() {
                console.log('reconnect');
                webSocketConnect();
            }, 3000);
        }

    } catch(exception) {
        alert('Error: ' + exception);
    }
}

function getWsUrl() {
    var pcol;
    var u = document.URL;
    var separator;

    if (u.substring(0, 5) == 'https') {
        pcol = 'wss://';
        u = u.substr(8);
    } else {
        pcol = 'ws://';
        if (u.substring(0, 4) == 'http')
            u = u.substr(7);
    }

    u = u.split('#');
    if (/\/$/.test(u[0]))
        separator = '';
    else
        separator = '/';

    return pcol + u[0] + separator + 'ws';
}

function parseStats(obj) {
    document.getElementById('mpdstats_artists').innerText =  obj.data.artists;
    document.getElementById('mpdstats_albums').innerText = obj.data.albums;
    document.getElementById('mpdstats_songs').innerText = obj.data.songs;
    document.getElementById('mpdstats_dbPlaytime').innerText = beautifyDuration(obj.data.dbPlaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.data.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.data.uptime);
    var d = new Date(obj.data.dbUpdated * 1000);
    document.getElementById('mpdstats_dbUpdated').innerText = d.toUTCString();
    document.getElementById('mympdVersion').innerText = obj.data.mympdVersion;
    document.getElementById('mpdVersion').innerText = obj.data.mpdVersion;
}

function toggleBtn(btn, state) {
    var b = document.getElementById(btn);
    if (!b)
        return;
    if (state == undefined)
        state = b.classList.contains('active') ? 0 : 1;

    if (state == 1 || state == true)
        b.classList.add('active');
    else
        b.classList.remove('active');
}

function parseSettings(obj) {
    toggleBtn('btnRandom', obj.data.random);
    toggleBtn('btnConsume', obj.data.consume);
    toggleBtn('btnSingle', obj.data.single);
    toggleBtn('btnRepeat', obj.data.repeat);
    toggleBtn('btnJukebox', obj.data.jukeboxMode);

    if (obj.data.crossfade != undefined) {
        document.getElementById('inputCrossfade').removeAttribute('disabled');
        document.getElementById('inputCrossfade').value = obj.data.crossfade;
    } else {
        document.getElementById('inputCrossfade').setAttribute('disabled', 'disabled');
    }
    if (obj.data.mixrampdb != undefined) {
        document.getElementById('inputMixrampdb').removeAttribute('disabled');
        document.getElementById('inputMixrampdb').value = obj.data.mixrampdb;
    } else {
        document.getElementById('inputMixrampdb').setAttribute('disabled', 'disabled');
    }
    if (obj.data.mixrampdelay != undefined) {
        document.getElementById('inputMixrampdelay').removeAttribute('disabled');
        document.getElementById('inputMixrampdelay').value = obj.data.mixrampdelay;
    } else {
        document.getElementById('inputMixrampdelay').setAttribute('disabled', 'disabled');
    }

    document.getElementById('selectReplaygain').value = obj.data.replaygain;

    var btnnotifyWeb = document.getElementById('btnnotifyWeb');
    if (notificationsSupported()) {
        if (obj.data.notificationWeb) {
            toggleBtn('btnnotifyWeb', obj.data.notificationWeb);
            Notification.requestPermission(function (permission) {
                if (!('permission' in Notification))
                    Notification.permission = permission;
                if (permission === 'granted') {
                    toggleBtn('btnnotifyWeb', 1);
                } else {
                    toggleBtn('btnnotifyWeb', 0);
                    obj.data.notificationWeb = true;
                }
            });         
        }
        else {
            toggleBtn('btnnotifyWeb', 0);
        }
    } else {
        btnnotifyWeb.setAttribute('disabled', 'disabled');
        toggleBtn('btnnotifyWeb', 0);
    }
    
    toggleBtn('btnnotifyPage', obj.data.notificationPage);

    var stickerEls = document.getElementsByClassName('stickers');
    var stickerElsLen = stickerEls.length;
    var displayStickers = obj.data.stickers == true ? '' : 'none';
    for (var i = 0; i < stickerElsLen; i++) {
        stickerEls[i].style.display = displayStickers;
    }
    
    if (obj.data.mixramp == true) {
        document.getElementsByClassName('mixramp')[0].style.display = '';
    } else {
        document.getElementsByClassName('mixramp')[0].style.display = 'none';
    }

    settings = obj.data;
    settings.mpdstream = 'http://';
    if (settings.mpdhost == '127.0.0.1' || settings.mpdhost == 'localhost')
        settings.mpdstream += window.location.hostname;
    else
        settings.mpdstream += settings.mpdhost;
    settings.mpdstream += ':' + settings.streamport + '/';
    
    addTagList('BrowseDatabaseByTagDropdown', false);
    addTagList('searchqueuetag', true);
    addTagList('searchtags', true);
}

function getSettings() {
    sendAPI({"cmd": "MPD_API_SETTINGS_GET"}, parseSettings);
}

function parseOutputs(obj) {
    var btns = '';
    var outputsLen = obj.data.outputs.length;
    for (var i = 0; i < outputsLen; i++) {
        btns += '<button id="btnOutput' + obj.data.outputs[i].id +'" data-output-id="' + obj.data.outputs[i].id + '" class="btn btn-secondary btn-block';
        if (obj.data.outputs[i].state == 1)
            btns += ' active';
        btns += '"><span class="material-icons float-left">volume_up</span> ' + obj.data.outputs[i].name + '</button>';
    }
    domCache.outputs.innerHTML = btns;
}

function setCounter(currentSongId, totalTime, elapsedTime) {
    currentSong.totalTime = totalTime;
    currentSong.elapsedTime = elapsedTime;
    currentSong.currentSongId = currentSongId;
    var total_minutes = Math.floor(totalTime / 60);
    var total_seconds = totalTime - total_minutes * 60;
    var elapsed_minutes = Math.floor(elapsedTime / 60);
    var elapsed_seconds = elapsedTime - elapsed_minutes * 60;

    domCache.progressBar.value = Math.floor(100 * elapsedTime / totalTime);

    var counterText = elapsed_minutes + ":" + 
        (elapsed_seconds < 10 ? '0' : '') + elapsed_seconds + " / " +
        total_minutes + ":" + (total_seconds < 10 ? '0' : '') + total_seconds;
    domCache.counter.innerText = counterText;
    
    //Set playing track in queue view
    if (lastState) {
        var tr = document.getElementById('queueTrackId' + lastState.data.currentSongId);
        if (tr) {
            var trtds = tr.getElementsByTagName('td');
            trtds[4].innerText = tr.getAttribute('data-duration');
            trtds[0].classList.remove('material-icons');
            trtds[0].innerText = tr.getAttribute('data-songpos');
            tr.classList.remove('font-weight-bold');
        }
    }
    var tr = document.getElementById('queueTrackId' + currentSongId);
    if (tr) {
        var trtds = tr.getElementsByTagName('td');
        trtds[4].innerText = counterText;
        trtds[0].classList.add('material-icons');
        trtds[0].innerText = 'play_arrow';
        tr.classList.add('font-weight-bold');
    }
    
    if (progressTimer)
            clearTimeout(progressTimer);
    if (playstate == 'play') {
        progressTimer = setTimeout(function() {
            currentSong.elapsedTime ++;
            setCounter(currentSong.currentSongId, currentSong.totalTime, currentSong.elapsedTime);    
        }, 1000);
    }
}

function parseState(obj) {
    if (JSON.stringify(obj) === JSON.stringify(lastState))
        return;

    //Set playstate
    if (obj.data.state == 1) {
        for (var i = 0; i < domCache.btnsPlayLen; i++)
            domCache.btnsPlay[i].innerText = 'play_arrow';
        playstate = 'stop';
    } else if (obj.data.state == 2) {
        for (var i = 0; i < domCache.btnsPlayLen; i++)
            domCache.btnsPlay[i].innerText = 'pause';
        playstate = 'play';
    } else {
        for (var i = 0; i < domCache.btnsPlayLen; i++)
            domCache.btnsPlay[i].innerText = 'play_arrow';
	playstate = 'pause';
    }

    if (obj.data.nextSongPos == -1 && settings.jukeboxMode == false)
        domCache.btnNext.setAttribute('disabled','disabled');
    else
        domCache.btnNext.removeAttribute('disabled');
    
    if (obj.data.songPos <= 0)
        domCache.btnPrev.setAttribute('disabled','disabled');
    else
        domCache.btnPrev.removeAttribute('disabled');
    
    if (obj.data.queueLength == 0)
        for (var i = 0; i < domCache.btnsPlayLen; i++)
            domCache.btnsPlay[i].setAttribute('disabled','disabled');
    else
        for (var i = 0; i < domCache.btnsPlayLen; i++)
            domCache.btnsPlay[i].removeAttribute('disabled');

    //Set volume
    if (obj.data.volume == -1) {
      domCache.volumePrct.innerText = 'Volumecontrol disabled';
      domCache.volumeControl.classList.add('hide');
    } else {
        domCache.volumeControl.classList.remove('hide');
        domCache.volumePrct.innerText = obj.data.volume + ' %';
        if (obj.data.volume == 0)
            domCache.volumeIcon.innerText = 'volume_off';
        else if (obj.data.volume < 50)
            domCache.volumeIcon.innerText = 'volume_down';
        else
            domCache.volumeIcon.innerText = 'volume_up';
    }
    domCache.volumeBar.value = obj.data.volume;

    //Set play counters
    setCounter(obj.data.currentSongId, obj.data.totalTime, obj.data.elapsedTime);
    
    //Get current song
    sendAPI({"cmd": "MPD_API_PLAYER_CURRENT_SONG"}, songChange);
    //clear playback card if not playing
    if (obj.data.songPos == '-1') {
        domCache.currentTrack.innerText = 'Not playing';
        domCache.currentAlbum.innerText = '';
        domCache.currentArtist.innerText = '';
        domCache.currentCover.style.backgroundImage = '';
    }

    lastState = obj;                    
}

function getQueue() {
    if (app.current.search.length >= 2) 
        sendAPI({"cmd": "MPD_API_QUEUE_SEARCH", "data": {"filter": app.current.filter, "offset": app.current.page, "searchstr": app.current.search}}, parseQueue);
    else {
        sendAPI({"cmd": "MPD_API_QUEUE_LIST", "data": {"offset": app.current.page}}, parseQueue);
    }
}

function parseQueue(obj) {
    if (app.current.app !== 'Queue')
        return;
    
    if (typeof(obj.totalTime) != undefined && obj.totalTime > 0 && obj.totalEntities <= settings.maxElementsPerPage )
        document.getElementById('panel-heading-queue').innerText = obj.totalEntities + ' Songs – ' + beautifyDuration(obj.totalTime);
    else if (obj.totalEntities > 0)
        document.getElementById('panel-heading-queue').innerText = obj.totalEntities + ' Songs';
    else
        document.getElementById('panel-heading-queue').innerText = '';

    var nrItems = obj.data.length;
    var table = document.getElementById(app.current.app + 'List');
    table.setAttribute('data-version', obj.queueVersion);
    var tbody = table.getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    for (var i = 0; i < nrItems; i++) {
        if (tr[i])
            if (tr[i].getAttribute('data-trackid') == obj.data[i].id && tr[i].getAttribute('data-songpos') == (obj.data[i].pos + 1))
                continue;
                
        var minutes = Math.floor(obj.data[i].duration / 60);
        var seconds = obj.data[i].duration - minutes * 60;
        var duration = minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
        var row = document.createElement('tr');
        row.setAttribute('draggable','true');
        row.setAttribute('data-trackid', obj.data[i].id);
        row.setAttribute('id','queueTrackId' + obj.data[i].id);
        row.setAttribute('data-songpos', (obj.data[i].pos + 1));
        row.setAttribute('data-duration', duration);
        row.setAttribute('data-uri', obj.data[i].uri);
        row.innerHTML = '<td>' + (obj.data[i].pos + 1) + '</td>' +
                        '<td>' + obj.data[i].title + '</td>' +
                        '<td>' + obj.data[i].artist + '</td>' + 
                        '<td>' + obj.data[i].album + '</td>' +
                        '<td>' + duration + '</td>' +
                        '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
        if (i < tr.length)
            tr[i].replaceWith(row); 
        else 
            tbody.append(row);  
    }
    var trLen = tr.length - 1;
    for (var i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }                    

    if (obj.type == 'queuesearch' && nrItems == 0)
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">No results, please refine your search!</td></tr>';
    else if (obj.type == 'queue' && nrItems == 0)
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">Empty queue</td></tr>';

    setPagination(obj.totalEntities);
    document.getElementById('QueueList').classList.remove('opacity05');
}

function parseSearch(obj) {
    if (app.current.app !== 'Search')
        return;
    document.getElementById('panel-heading-search').innerHTML = obj.totalEntities + ' Songs found';
    if (obj.totalEntities > 0) {
        document.getElementById('searchAddAllSongs').removeAttribute('disabled');
        document.getElementById('searchAddAllSongsBtn').removeAttribute('disabled');
    } 
    else {
        document.getElementById('searchAddAllSongs').setAttribute('disabled','disabled');
        document.getElementById('searchAddAllSongsBtn').setAttribute('disabled','disabled');
    }
    parseFilesystem(obj);
}

function parseFilesystem(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Filesystem' && app.current.app !== 'Search')
        return;
    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app + (app.current.tab==undefined ? '' : app.current.tab) + 'List').getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    for (var i = 0; i < nrItems; i++) {
        var uri = encodeURI(obj.data[i].uri);
        if (tr[i])
            if (tr[i].getAttribute('data-uri') == uri)
                continue;
        var row = document.createElement('tr');
        row.setAttribute('data-type', obj.data[i].type);
        row.setAttribute('data-uri', uri);
        row.setAttribute('data-name', obj.data[i].name);
        
        switch(obj.data[i].type) {
            case 'dir':
                row.innerHTML = '<td><span class="material-icons">folder_open</span></td>' +
                                '<td colspan="4">' + obj.data[i].name + '</td>' +
                                '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
                break;
            case 'song':
                var minutes = Math.floor(obj.data[i].duration / 60);
                var seconds = obj.data[i].duration - minutes * 60;
                row.innerHTML = '<td><span class="material-icons">music_note</span></td>' + 
                                '<td>' + obj.data[i].title + '</td>' +
                                '<td>' + obj.data[i].artist + '</td>' + 
                                '<td>' + obj.data[i].album  + '</td>' +
                                '<td>' + minutes + ':' + (seconds < 10 ? '0' : '') + seconds +
                                '</td><td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
                break;
            case 'plist':
                row.innerHTML = '<td><span class="material-icons">list</span></td>' +
                                '<td colspan="4">' + obj.data[i].name + '</td>' +
                                '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
                break;
        }
        if (i < tr.length)
            tr[i].replaceWith(row); 
        else 
            tbody.append(row);
    }
    var trLen = tr.length - 1;
    for (var i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }
    
    setPagination(obj.totalEntities);
                    
    if (nrItems == 0)
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">No results</td></tr>';
    document.getElementById(app.current.app + (app.current.tab==undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
}

function parsePlaylists(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Playlists')
        return;
    if (app.current.view == 'All') {
        document.getElementById('BrowsePlaylistsAllList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsDetailList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.add('hide');
        document.getElementById('btnPlaylistClear').parentNode.classList.add('hide');
    } else {
        if (obj.uri.indexOf('.') > -1) {
            document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-ro', 'true')
            document.getElementById('btnPlaylistClear').parentNode.classList.add('hide');
        }
        else {
            document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-ro', 'false');
            document.getElementById('btnPlaylistClear').parentNode.classList.remove('hide');
        }
        document.getElementById('BrowsePlaylistsDetailList').setAttribute('data-uri', obj.uri);
        document.getElementById('BrowsePlaylistsDetailList').getElementsByTagName('caption')[0].innerText = 'Playlist: ' + obj.uri;
        document.getElementById('BrowsePlaylistsDetailList').classList.remove('hide');
        document.getElementById('BrowsePlaylistsAllList').classList.add('hide');
        document.getElementById('btnBrowsePlaylistsAll').parentNode.classList.remove('hide');
    }
            
    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    if (app.current.view == 'All') {
        for (var i = 0; i < nrItems; i++) {
            var uri = encodeURI(obj.data[i].uri);
            if (tr[i])
                if (tr[i].getAttribute('data-uri') == uri)
                    continue;
            var d = new Date(obj.data[i].last_modified * 1000);
            var row = document.createElement('tr');
            row.setAttribute('data-uri', uri);
            row.setAttribute('data-type', 'plist');
            row.setAttribute('data-name', obj.data[i].name);
            row.innerHTML = '<td><span class="material-icons">list</span></td>' +
                            '<td>' + obj.data[i].name + '</td>' +
                            '<td>'+ d.toUTCString() + '</td>' +
                            '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
            if (i < tr.length)
                tr[i].replaceWith(row); 
            else 
                tbody.append(row);
        }
    }
    else if (app.current.view == 'Detail') {
        for (var i = 0; i < nrItems; i++) {
            var uri = encodeURI(obj.data[i].uri);
            var songpos = obj.offset + i + 1;
            if (tr[i])
                if (tr[i].getAttribute('data-uri') == uri && tr[i].getAttribute('id') == 'playlistTrackId' + songpos)
                    continue;
            var row = document.createElement('tr');
            row.setAttribute('draggable','true');
            row.setAttribute('id','playlistTrackId' + songpos);
            row.setAttribute('data-type', obj.data[i].type);
            row.setAttribute('data-uri', uri);
            row.setAttribute('data-name', obj.data[i].name);
            row.setAttribute('data-songpos', songpos);
            var minutes = Math.floor(obj.data[i].duration / 60);
            var seconds = obj.data[i].duration - minutes * 60;
            row.innerHTML = '<td>' + songpos + '</td>' + 
                            '<td>' + obj.data[i].title + '</td>' +
                            '<td>' + obj.data[i].artist + '</td>' + 
                            '<td>' + obj.data[i].album  + '</td>' +
                            '<td>' + minutes + ':' + (seconds < 10 ? '0' : '') + seconds +
                            '</td><td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>';
            if (i < tr.length)
                tr[i].replaceWith(row); 
            else 
                tbody.append(row);
        }
    }
    var trLen = tr.length - 1;
    for (var i = trLen; i >= nrItems; i --) {
        tr[i].remove();
    }

    setPagination(obj.totalEntities);
    
    if (nrItems == 0)
        if (app.current.view == 'All')
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="5">No playlists found.</td></tr>';
        else
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="5">Empty playlist.</td></tr>';
            
    document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').classList.remove('opacity05');
}

function parseListDBtags(obj) {
//    if (app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'AlbumArtist')
//    return;
    if (app.current.search != '') {
        document.getElementById('BrowseDatabaseAlbumList').classList.remove('hide');
        document.getElementById('BrowseDatabaseTagList').classList.add('hide');
        document.getElementById('btnBrowseDatabaseByTag').parentNode.classList.add('hide');
        document.getElementById('btnBrowseDatabaseTag').parentNode.classList.remove('hide');
        document.getElementById('BrowseDatabaseAddAllSongs').parentNode.parentNode.classList.remove('hide');
        document.getElementById('btnBrowseDatabaseTag').innerHTML = '&laquo; ' + app.current.view;
        document.getElementById('BrowseDatabaseAlbumListCaption').innerText = obj.searchtagtype + ': ' + obj.searchstr;
        var nrItems = obj.data.length;
        var cardContainer = document.getElementById('BrowseDatabaseAlbumList');
        var cards = cardContainer.getElementsByClassName('col-md');
        for (var i = 0; i < nrItems; i++) {
            var id=genId(obj.data[i].value);
            if (cards[i])
                if (cards[i].getAttribute('id') == id)
                    continue;              
            var card=document.createElement('div');
            card.classList.add('col-md');
            card.classList.add('mr-0');
            card.setAttribute('id', id);
            card.innerHTML = '<div class="card mb-4" id="card' + id + '">' +
                             ' <a href="#" class="card-img-top"><img class="card-img-top" src="" ></a>' +
                             ' <div class="card-body">' +
                             '  <h5 class="card-title" id="albumartist' + id + '"></h5>' +
                             '  <h4 class="card-title">' + obj.data[i].value + '</h4>' +
                             '  <table class="table table-sm table-hover" id="tbl' + id + '"><tbody></tbody></table'+
                             ' </div>'+
                             '</div>';
         
            if (i < cards.length)
                cards[i].replaceWith(card); 
            else 
                cardContainer.append(card);
                
            sendAPI({"cmd": "MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST", "data": { "album": obj.data[i].value, "search": app.current.search, "tag": app.current.view}}, parseListTitles);
        }
        var cardsLen = cards.length - 1;
        for (var i = cardsLen; i >= nrItems; i --) {
            cards[i].remove();
        }
        setPagination(obj.totalEntities);
        document.getElementById('BrowseDatabaseAlbumList').classList.remove('opacity05');        
    }  
    else {
        document.getElementById('BrowseDatabaseAlbumList').classList.add('hide');
        document.getElementById('BrowseDatabaseTagList').classList.remove('hide');
        document.getElementById('btnBrowseDatabaseByTag').parentNode.classList.remove('hide');
        document.getElementById('BrowseDatabaseAddAllSongs').parentNode.parentNode.classList.add('hide');
        document.getElementById('btnBrowseDatabaseTag').parentNode.classList.add('hide');
        
        var nrItems = obj.data.length;
        var tbody = document.getElementById(app.current.app + app.current.tab + 'TagList').getElementsByTagName('tbody')[0];
        var tr = tbody.getElementsByTagName('tr');
        for (var i = 0; i < nrItems; i++) {
            var uri = encodeURI(obj.data[i].value);
            if (tr[i])
                if (tr[i].getAttribute('data-uri') == uri)
                    continue;
            var row = document.createElement('tr');
            row.setAttribute('data-uri', uri);
            row.innerHTML='<td><span class="material-icons">album</span></td>' +
                          '<td>' + obj.data[i].value + '</td>';

            if (i < tr.length)
                tr[i].replaceWith(row); 
            else 
                tbody.append(row);

        }
        var trLen = tr.length - 1;
        for (var i = trLen; i >= nrItems; i --) {
            tr[i].remove();
        }

        setPagination(obj.totalEntities);

        if (nrItems == 0) 
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td>No entries found.</td></tr>';
        document.getElementById('BrowseDatabaseTagList').classList.remove('opacity05');                              
    }
}

function parseListTitles(obj) {
//    if (app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Album') 
//          return;
  
    var id = genId(obj.album);
    var card = document.getElementById('card' + id)
    var tbody = card.getElementsByTagName('tbody')[0];
    var img = card.getElementsByTagName('img')[0];
    var imga = img.parentNode;
    img.setAttribute('src', obj.cover);
    imga.setAttribute('data-uri', encodeURI(obj.data[0].uri.replace(/\/[^\/]+$/, '')));
    imga.setAttribute('data-name', obj.album);
    imga.setAttribute('data-type', 'dir');
    document.getElementById('albumartist' + id).innerText = obj.albumartist;
  
    var titleList = '';
    var nrItems = obj.data.length;
    for (var i = 0; i < nrItems; i++) {
        titleList += '<tr data-type="song" data-name="' + obj.data[i].title + '" data-uri="' + encodeURI(obj.data[i].uri) + '">' +
                     '<td>' + obj.data[i].track + '</td><td>' + obj.data[i].title + '</td>' +
                     '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>' + 
                     '</tr>';
    }
    tbody.innerHTML = titleList;
  
    imga.addEventListener('click', function(event) {
        showMenu(this, event);
    }, false);

    tbody.parentNode.addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute('data-uri')), event.target.parentNode.getAttribute('data-name'));
        }
        else if (event.target.nodeName == 'A') {
            showMenu(event.target, event);
        }
    }, false);
}

function setPagination(number) {
    var totalPages = Math.ceil(number / settings.maxElementsPerPage);
    var cat = app.current.app + (app.current.tab == undefined ? '': app.current.tab);
    if (totalPages == 0) 
        totalPages = 1;
    var p = ['PaginationTop', 'PaginationBottom'];
    for (var i = 0; i < 2; i++) {
        document.getElementById(cat + p[i] + 'Page').innerText = (app.current.page / settings.maxElementsPerPage + 1) + ' / ' + totalPages;
        if (totalPages > 1) {
            document.getElementById(cat + p[i] + 'Page').removeAttribute('disabled');
            var pl = '';
            for (var j = 0; j < totalPages; j++) {
                pl += '<button data-page="' + (j * settings.maxElementsPerPage) + '" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">' +
                    ( j + 1) + '</button>';
            }
            document.getElementById(cat + p[i] + 'Pages').innerHTML = pl;
        } else {
            document.getElementById(cat + p[i] + 'Page').setAttribute('disabled', 'disabled');
        }
    
        if (number > app.current.page + settings.maxElementsPerPage) {
            document.getElementById(cat + p[i] + 'Next').removeAttribute('disabled');
            document.getElementById(cat + p[i]).classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        } else {
            document.getElementById(cat + p[i] + 'Next').setAttribute('disabled', 'disabled');
            document.getElementById(cat + p[i]).classList.add('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
        }
    
        if (app.current.page > 0) {
            document.getElementById(cat + p[i] + 'Prev').removeAttribute('disabled');
            document.getElementById(cat + p[i]).classList.remove('hide');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        } else {
            document.getElementById(cat + p[i] + 'Prev').setAttribute('disabled', 'disabled');
        }
    }
}

function appendQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI({"cmd": "MPD_API_QUEUE_ADD_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" added', '', '', 'success');
            break;
        case 'plist':
            sendAPI({"cmd": "MPD_API_QUEUE_ADD_PLAYLIST", "data": {"plist": uri}});
            showNotification('"' + name + '" added', '', '', 'success');
            break;
    }
}

function appendAfterQueue(type, uri, to, name) {
    switch(type) {
        case 'song':
            sendAPI({"cmd": "MPD_API_QUEUE_ADD_TRACK_AFTER", "data": {"uri": uri, "to": to}});
            showNotification('"' + name + '" added to pos ' + to, '', '', 'success');
            break;
    }
}

function replaceQueue(type, uri, name) {
    switch(type) {
        case 'song':
        case 'dir':
            sendAPI({"cmd": "MPD_API_QUEUE_REPLACE_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" replaced', '', '', 'success');
            break;
        case 'plist':
            sendAPI({"cmd": "MPD_API_QUEUE_REPLACE_PLAYLIST", "data": {"plist": uri}});
            showNotification('"' + name + '" replaced', '', '', 'success');
            break;
    }
}

function songClick() {
    var uri = domCache.currentTrack.getAttribute('data-uri')
    if (uri != '')
        songDetails(uri);
}

function artistClick() {
    var albumartist = domCache.currentArtist.getAttribute('data-albumartist');
    if (albumartist != '') 
        appGoto('Browse', 'Database', 'AlbumArtist', '0/-/' + albumartist);
}

function albumClick() {
    var album = domCache.currentAlbum.getAttribute('data-album');
    if (album != '') 
        appGoto('Browse', 'Database', 'Album', '0/-/' + album);
}

function songDetails(uri) {
    sendAPI({"cmd": "MPD_API_DATABASE_SONGDETAILS", "data": {"uri": uri}}, parseSongDetails);
    modalSongDetails.show();
}

function parseSongDetails(obj) {
    var modal = document.getElementById('modalSongDetails');
    modal.getElementsByClassName('album-cover')[0].style.backgroundImage = 'url("' + obj.data.cover + '")';
    modal.getElementsByTagName('h1')[0].innerText = obj.data.title;
    
    var songDetails = '';
    for (var key in settings.tags) {
        if (settings.tags[key] == true) {
            var value = obj.data[key.toLowerCase()];
            if (key == 'duration') {
                var minutes = Math.floor(value / 60);
                var seconds = value - minutes * 60;
                value = minutes + ':' + (seconds < 10 ? '0' : '') + seconds;        
            }
            songDetails += '<tr><th>' + key + '</th><td>' + value + '</td></tr>';
        }
    }
    
    songDetails += '<tr><th>Uri</th><td><a class="text-success" href="/library/' + obj.data.uri + '">' + obj.data.uri + '</a></td></tr>';
    
    if (settings.stickers == true) {
        var like = 'not voted';
        if (obj.data.like == 0)
            like = '<span class="material-icons">thumb_down_alt</span>';
        else if (obj.data.like == 2)
            like = '<span class="material-icons">thumb_up_alt</span>';
        songDetails += '<tr><th colspan="2">Statistics</th></tr>' +
            '<tr><th>Play count</th><td>' + obj.data.playCount + '</td></tr>' +
            '<tr><th>Skip count</th><td>' + obj.data.skipCount + '</td></tr>' +
            '<tr><th>Last played</th><td>' + (obj.data.lastPlayed == 0 ? 'never' : new Date(value * 1000).toUTCString()) + '</td></tr>' +
            '<tr><th>Like</th><td>' + like + '</td></tr>';
    }
    
    modal.getElementsByTagName('tbody')[0].innerHTML = songDetails;
}

function playlistDetails(uri) {
    appGoto('Browse', 'Playlists', 'Detail', '0/-/' + uri);
}

function removeFromPlaylist(uri, pos) {
    pos--;
    sendAPI({"cmd": "MPD_API_PLAYLIST_RM_TRACK", "data": {"uri": uri, "track": pos}});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
    sendAPI({"cmd": "MPD_API_PLAYLIST_CONTENT_LIST", "data": {"offset": app.current.page, "filter": app.current.filter, "uri": app.current.search}}, parsePlaylists);
}

function playlistClear() {
    var uri = document.getElementById('BrowsePlaylistsDetailList').getAttribute('data-uri');
    sendAPI({"cmd": "MPD_API_PLAYLIST_CLEAR", "data": {"uri": uri}});
    document.getElementById('BrowsePlaylistsDetailList').classList.add('opacity05');    
    sendAPI({"cmd": "MPD_API_PLAYLIST_CONTENT_LIST", "data": {"offset": app.current.page, "filter": app.current.filter, "uri": app.current.search}}, parsePlaylists);
}

function getAllPlaylists(obj) {
    var nrItems = obj.data.length;
    var playlists = '<option></option><option>New Playlist</option>';
    for (var i = 0; i < nrItems; i++) {
        playlists += '<option>' + obj.data[i].uri + '</option>';
    }
    document.getElementById('addToPlaylistPlaylist').innerHTML += playlists;
    if (obj.totalEntities > obj.returnedEntities) {
        obj.offset += settings.maxElementsPerPage;
        sendAPI({"cmd": "MPD_API_PLAYLIST_LIST","data": {"offset": obj.offset, "filter": "-"}}, getAllPlaylists);
    }
}

function voteSong(vote) {
    var uri = domCache.currentTrack.getAttribute('data-uri');
    if (uri == '')
        return;
        
    if (vote == 2 && domCache.btnVoteUp.classList.contains('active-fg-green'))
        vote = 1;
    else if (vote == 0 && domCache.btnVoteDown.classList.contains('active-fg-red'))
        vote = 1;
    sendAPI({"cmd": "MPD_API_LIKE","data": {"uri": uri, "like": vote}});
    setVoteSongBtns(vote, uri);
}

function setVoteSongBtns(vote, uri) {
    if (uri == '' || uri.indexOf('http://') == 0 || uri.indexOf('https://') == 0) {
        domCache.btnVoteUp.setAttribute('disabled', 'disabled');
        domCache.btnVoteDown.setAttribute('disabled', 'disabled');
    } else {
        domCache.btnVoteUp.removeAttribute('disabled');
        domCache.btnVoteDown.removeAttribute('disabled');    
    }
    
    if (vote == 0) {
        domCache.btnVoteUp.classList.remove('active-fg-green');
        domCache.btnVoteDown.classList.add('active-fg-red');
    } else if (vote == 1) {
        domCache.btnVoteUp.classList.remove('active-fg-green');
        domCache.btnVoteDown.classList.remove('active-fg-red');
    } else if (vote == 2) {
        domCache.btnVoteUp.classList.add('active-fg-green');
        domCache.btnVoteDown.classList.remove('active-fg-red');
    }
}

function toggleAddToPlaylistFrm() {
    var btn = document.getElementById('toggleAddToPlaylistBtn');
    toggleBtn('toggleAddToPlaylistBtn');
    if (btn.classList.contains('active')) {
        document.getElementById('addToPlaylistFrm').classList.remove('hide');
        document.getElementById('addStreamFooter').classList.add('hide');
        document.getElementById('addToPlaylistFooter').classList.remove('hide');
    }    
    else {
        document.getElementById('addToPlaylistFrm').classList.add('hide');
        document.getElementById('addStreamFooter').classList.remove('hide');
        document.getElementById('addToPlaylistFooter').classList.add('hide');
    }
}

function showAddToPlaylist(uri) {
    document.getElementById('addToPlaylistUri').value = uri;
    document.getElementById('addToPlaylistPlaylist').innerHTML = '';
    document.getElementById('addToPlaylistNewPlaylist').value = '';
    document.getElementById('addToPlaylistNewPlaylistDiv').classList.add('hide');
    document.getElementById('addToPlaylistFrm').classList.remove('was-validated');
    document.getElementById('addToPlaylistNewPlaylist').classList.remove('is-invalid');
    toggleBtn('toggleAddToPlaylistBtn',0);
    var streamUrl = document.getElementById('streamUrl')
    streamUrl.focus();
    streamUrl.value = '';
    streamUrl.classList.remove('is-invalid');
    document.getElementById('addStreamFrm').classList.remove('was-validated');
    if (uri != 'stream') {
        document.getElementById('addStreamFooter').classList.add('hide');
        document.getElementById('addStreamFrm').classList.add('hide');
        document.getElementById('addToPlaylistFooter').classList.remove('hide');
        document.getElementById('addToPlaylistFrm').classList.remove('hide');
        document.getElementById('addToPlaylistLabel').innerText = 'Add to playlist';
    } else {
        document.getElementById('addStreamFooter').classList.remove('hide');
        document.getElementById('addStreamFrm').classList.remove('hide');
        document.getElementById('addToPlaylistFooter').classList.add('hide');
        document.getElementById('addToPlaylistFrm').classList.add('hide');
        document.getElementById('addToPlaylistLabel').innerText = 'Add Stream';
    }
    modalAddToPlaylist.show();
    sendAPI({"cmd": "MPD_API_PLAYLIST_LIST","data": {"offset": 0, "filter": "-"}}, getAllPlaylists);
}

function addToPlaylist() {
    var uri = document.getElementById('addToPlaylistUri').value;
    if (uri == 'stream') {
        uri = document.getElementById('streamUrl').value;
        if (uri == '' || uri.indexOf('http') == -1) {
            document.getElementById('streamUrl').classList.add('is-invalid');
            document.getElementById('addStreamFrm').classList.add('was-validated');
            return;
        }
    }
    var plistEl = document.getElementById('addToPlaylistPlaylist');
    var plist = plistEl.options[plistEl.selectedIndex].text;
    if (plist == 'New Playlist') {
        var newPl = document.getElementById('addToPlaylistNewPlaylist').value;
        var valid = newPl.replace(/\w\-/g, '');
        if (newPl != '' && valid == '') {
            plist = newPl;
        } else {
            document.getElementById('addToPlaylistNewPlaylist').classList.add('is-invalid');
            document.getElementById('addToPlaylistFrm').classList.add('was-validated');
            return;
        }
    }
    if (plist != '') {
        if (uri == 'SEARCH')
            addAllFromSearchPlist(plist);
        else if (uri == 'DATABASE')
            addAllFromBrowseDatabasePlist(plist);
        else
            sendAPI({"cmd": "MPD_API_PLAYLIST_ADD_TRACK", "data": {"uri": uri, "plist": plist}});
        modalAddToPlaylist.hide();
    }
    else {
        document.getElementById('addToPlaylistPlaylist').classList.add('is-invalid');
        document.getElementById('addToPlaylistFrm').classList.add('was-validated');
    }
}

function addStream() {
    var streamUrl = document.getElementById('streamUrl').value;
    if (streamUrl != '' && streamUrl.indexOf('http') == 0) {
        sendAPI({"cmd": "MPD_API_QUEUE_ADD_TRACK", "data": {"uri": streamUrl}});
        modalAddToPlaylist.hide();
        showNotification('Added stream ' + streamUrl + 'to queue', '', '', 'success');
    }
    else {
        document.getElementById('streamUrl').classList.add('is-invalid');
        document.getElementById('addStreamFrm').classList.add('was-validated');
    }
}

function showRenamePlaylist(from) {
    document.getElementById('renamePlaylistFrm').classList.remove('was-validated');
    document.getElementById('renamePlaylistTo').classList.remove('is-invalid');
    modalRenamePlaylist.show();
    document.getElementById('renamePlaylistFrom').value = from;
    document.getElementById('renamePlaylistTo').value = '';
}

function renamePlaylist() {
    var from = document.getElementById('renamePlaylistFrom').value;
    var to = document.getElementById('renamePlaylistTo').value;
    var valid = to.replace(/\w\-/g, '');
    if (to != '' && to != from && valid == '') {
        sendAPI({"cmd": "MPD_API_PLAYLIST_RENAME", "data": {"from": from, "to": to}});
        modalRenamePlaylist.hide();
        sendAPI({"cmd": "MPD_API_PLAYLIST_LIST","data": {"offset": app.current.page, "filter": app.current.filter}}, parsePlaylists);
    }
    else {
        document.getElementById('renamePlaylistTo').classList.add('is-invalid');
        document.getElementById('renamePlaylistFrm').classList.add('was-validated');
    }
}

function dirname(uri) {
    return uri.replace(/\/[^\/]*$/, '');
}

function addMenuItem(href, text) {
    return '<a class="dropdown-item" href="#" data-href=\'' + btoa(JSON.stringify(href)) + '\'>' + text +'</a>';
}

function showMenu(el, event) {
    event.preventDefault();
    event.stopPropagation();

    var oldPopover = document.getElementsByClassName('popover');
    for (var i = 0; i < oldPopover.length; i++)
        oldPopover[i].remove();

    var type = el.getAttribute('data-type');
    var uri = decodeURI(el.getAttribute('data-uri'));
    var name = el.getAttribute('data-name');
    var nextsongpos = 0;
    if (type == null || uri == null) {
        type = el.parentNode.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.parentNode.getAttribute('data-uri'));
        name = el.parentNode.parentNode.getAttribute('data-name');
    }
    
    if (lastState)
        nextsongpos = lastState.data.nextSongPos;

    var menu = '';
    if ((app.current.app == 'Browse' && app.current.tab == 'Filesystem') || app.current.app == 'Search' ||
        (app.current.app == 'Browse' && app.current.tab == 'Database')) {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue') +
            (type == 'song' ? addMenuItem({"cmd": "appendAfterQueue", "options": [type, uri, nextsongpos, name]}, 'Add after current playing song') : '') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue') +
            (type != 'plist' ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri]}, 'Add to playlist') : '') +
            (type == 'song' ? addMenuItem({"cmd": "songDetails", "options": [uri]}, 'Songdetails') : '') +
            (type == 'plist' ? addMenuItem({"cmd": "playlistDetails", "options": [uri]}, 'Show playlist') : '');
        if (app.current.app == 'Search') {
            var baseuri = dirname(uri);
            menu += '<div class="dropdown-divider"></div>' +
                '<a class="dropdown-item" id="advancedMenuLink" data-toggle="collapse" href="#advancedMenu"><span class="material-icons material-icons-small-left">keyboard_arrow_right</span>Album actions</a>' +
                '<div class="collapse" id="advancedMenu">' +
                    addMenuItem({"cmd": "appendQueue", "options": [type, baseuri, name]}, 'Append to queue') +
                    addMenuItem({"cmd": "appendAfterQueue", "options": [type, baseuri, nextsongpos, name]}, 'Add after current playing song') +
                    addMenuItem({"cmd": "replaceQueue", "options": [type, baseuri, name]}, 'Replace queue') +
                    addMenuItem({"cmd": "showAddToPlaylist", "options": [baseuri]}, 'Add to playlist') +
                '</div>';
        }
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists' && app.current.view == 'All') {
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]},'Replace queue') +
            addMenuItem({"cmd": "playlistDetails", "options": [uri]}, 'Edit playlist') +
            addMenuItem({"cmd": "showRenamePlaylist", "options": [uri]}, 'Rename playlist') + 
            addMenuItem({"cmd": "delPlaylist", "options": [uri]}, 'Delete playlist');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists' && app.current.view == 'Detail') {
        var x = document.getElementById('BrowsePlaylistsDetailList');
        menu += addMenuItem({"cmd": "appendQueue", "options": [type, uri, name]}, 'Append to queue') +
            addMenuItem({"cmd": "replaceQueue", "options": [type, uri, name]}, 'Replace queue') +
            (x.getAttribute('data-ro') == 'false' ? addMenuItem({"cmd": "removeFromPlaylist", "options": [x.getAttribute('data-uri'), 
                    el.parentNode.parentNode.getAttribute('data-songpos')]}, 'Remove') : '') +
            (type != 'plist' ? addMenuItem({"cmd": "showAddToPlaylist", "options": [uri]}, 'Add to playlist') : '');
    }
    else if (app.current.app == 'Queue') {
        menu += addMenuItem({"cmd": "delQueueSong", "options": ["single", el.parentNode.parentNode.getAttribute('data-trackid')]}, 'Remove') +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", 0, el.parentNode.parentNode.getAttribute('data-songpos')]}, 'Remove all upwards') +
            addMenuItem({"cmd": "delQueueSong", "options": ["range", (parseInt(el.parentNode.parentNode.getAttribute('data-songpos'))-1), -1]}, 'Remove all downwards') +
            (uri.indexOf('http') == -1 ? addMenuItem({"cmd": "songDetails", "options": [uri]}, 'Songdetails') : '');
    }
    
    new Popover(el, { trigger: 'click', delay: 0, dismissible: true, template: '<div class="popover" role="tooltip">' +
        '<div class="arrow"></div>' +
        '<div class="popover-content">' + menu + '</div>' +
        '</div>'});
    var popoverInit = el.Popover;

    if (el.getAttribute('data-init')) {
        popoverInit.show();
        return;
    }

    el.setAttribute('data-init', 'true');
    el.addEventListener('shown.bs.popover', function(event) {
        document.getElementsByClassName('popover-content')[0].addEventListener('click', function(event) {
            event.preventDefault();
            event.stopPropagation();
            if (event.target.nodeName == 'A') {
                var dh = event.target.getAttribute('data-href');
                if (dh) {
                    var cmd = JSON.parse(atob(dh));
                    if (typeof window[cmd.cmd] === 'function') {
                        switch(cmd.cmd) {
                            case 'sendAPI':
                                sendAPI(... cmd.options); 
                                break;
                            default:
                                window[cmd.cmd](... cmd.options);                    
                        }
                    }
                    popoverInit.hide();
                }
            }
        }, false);        
        var collapseLink = document.getElementById('advancedMenuLink');
        if (collapseLink) {
            collapseLink.addEventListener('click', function(event) {
            var icon = this.getElementsByTagName('span')[0];
            if (icon.innerText == 'keyboard_arrow_right')
                icon.innerText = 'keyboard_arrow_down';
            else
                icon.innerText = 'keyboard_arrow_right';
        }, false);
            var myCollapseInit = new Collapse(collapseLink);
        }
    }, false);
    popoverInit.show();
}

function sendAPI(request, callback) {
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState == 4) {
            if (ajaxRequest.responseText != '') {
                var obj = JSON.parse(ajaxRequest.responseText);
                if (obj.type == 'error') {
                    showNotification('Error', obj.data, obj.data, 'danger');
                    console.log('Error: ' + obj.data);
                }
                else if (obj.type == 'result' && obj.data != 'ok')
                    showNotification(obj.data, '', '', 'success');
                else if (callback != undefined && typeof(callback) == 'function')
                    callback(obj);
            }
            else {
                console.log('Empty response for request: ' + JSON.stringify(request));
            }
        }
    };
    ajaxRequest.send(JSON.stringify(request));
}

function openLocalPlayer() {
    window.open('/player.html#' + settings.mpdstream, 'LocalPlayer');
}

function updateDB() {
    sendAPI({"cmd": "MPD_API_DATABASE_UPDATE"});
    updateDBstarted(true);
}

function updateDBstarted(showModal) {
    if (showModal == true) {
        document.getElementById('updateDBfinished').innerText = '';
        document.getElementById('updateDBfooter').classList.add('hide');
        updateDBprogress.style.width = '20px';
        updateDBprogress.style.marginLeft = '-20px';
        modalUpdateDB.show();
        document.getElementById('updateDBprogress').classList.add('updateDBprogressAnimate');
    }
    else {
        showNotification('Database update started', '', '', 'success');
    }
}

function updateDBfinished(idleEvent) {
    if (document.getElementById('modalUpdateDB').classList.contains('show')) {
        if (idleEvent == 'update_database')
            document.getElementById('updateDBfinished').innerText = 'Database successfully updated.';
        else if (idleEvent == 'update_finished')
            document.getElementById('updateDBfinished').innerText = 'Database update finished.';
        var updateDBprogress = document.getElementById('updateDBprogress');
        updateDBprogress.classList.remove('updateDBprogressAnimate');
        updateDBprogress.style.width = '100%';
        updateDBprogress.style.marginLeft = '0px';
        document.getElementById('updateDBfooter').classList.remove('hide');
    }
    else {
        if (idleEvent == 'update_database')
            showNotification('Database successfully updated.', '', '', 'success');
        else if (idleEvent == 'update_finished')
            showNotification('Database update finished.', '', '', 'success');
    }
}

function clickPlay() {
    if (playstate != 'play')
        sendAPI({"cmd": "MPD_API_PLAYER_PLAY"});
    else
        sendAPI({"cmd": "MPD_API_PLAYER_PAUSE"});
}

function clickStop() {
    sendAPI({"cmd": "MPD_API_PLAYER_STOP"});
}

function clickPrev() {
    sendAPI({"cmd": "MPD_API_PLAYER_PREV"});
}

function clickNext() {
    sendAPI({"cmd": "MPD_API_PLAYER_NEXT"});
}

function delQueueSong(mode, start, end) {
    if (mode == 'range')
        sendAPI({"cmd": "MPD_API_QUEUE_RM_RANGE", "data": {"start": start, "end": end}});
    else if (mode == 'single')
        sendAPI({"cmd": "MPD_API_QUEUE_RM_TRACK", "data": { "track": start}});
}

function delPlaylist(uri) {
    sendAPI({"cmd": "MPD_API_PLAYLIST_RM", "data": {"uri": uri}});
    sendAPI({"cmd": "MPD_API_PLAYLIST_LIST","data": {"offset": app.current.page, "filter": app.current.filter}}, parsePlaylists);    
}

function confirmSettings() {
    var formOK = true;
    var inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        var value = parseInt(inputCrossfade.value);
        if (!isNaN(value))
            inputCrossfade.value = value;
        else {
            inputCrossfade.classList.add('is-invalid');
            formOK = false;
        }
    }
    if (settings.mixramp) {
        var inputMixrampdb = document.getElementById('inputMixrampdb');
        if (!inputMixrampdb.getAttribute('disabled')) {
            var value = parseFloat(inputMixrampdb.value);
            if (!isNaN(value))
                inputMixrampdb.value = value;
            else {
                inputMixrampdb.classList.add('is-invalid');
                formOK = false;
            } 
        }
        var inputMixrampdelay = document.getElementById('inputMixrampdelay');
        if (!inputMixrampdelay.getAttribute('disabled')) {
            if (inputMixrampdelay.value == 'nan') 
                inputMixrampdelay.value = '-1';
            var value = parseFloat(inputMixrampdelay.value);
            if (!isNaN(value))
                inputMixrampdelay.value = value;
            else {
                inputMixrampdelay.classList.add('is-invalid');
                formOK = false;
            }
        }
    }
    
    if (formOK == true) {
        var selectReplaygain = document.getElementById('selectReplaygain');
        sendAPI({"cmd": "MPD_API_SETTINGS_SET", "data": {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random":  (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single":  (document.getElementById('btnSingle').classList.contains('active') ? 1 : 0),
            "repeat":  (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": selectReplaygain.options[selectReplaygain.selectedIndex].value,
            "crossfade": document.getElementById('inputCrossfade').value,
            "mixrampdb": (settings.mixramp == true ? document.getElementById('inputMixrampdb').value : settings.mixrampdb),
            "mixrampdelay": (settings.mixramp == true ? document.getElementById('inputMixrampdelay').value : settings.mixrampdelay),
            "notificationWeb": (document.getElementById('btnnotifyWeb').classList.contains('active') ? true : false),
            "notificationPage": (document.getElementById('btnnotifyPage').classList.contains('active') ? true : false),
            "jukeboxMode": (document.getElementById('btnJukebox').classList.contains('active') ? true : false)
        }}, getSettings);
        modalSettings.hide();
    } else
        document.getElementById('settingsFrm').classList.add('was-validated');
}

function addAllFromBrowseFilesystem() {
    sendAPI({"cmd": "MPD_API_QUEUE_ADD_TRACK", "data": {"uri": app.current.search}});
    showNotification('Added all songs', '', '', 'success');
}

function addAllFromSearchPlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI({"cmd": "MPD_API_DATABASE_SEARCH", "data": {"plist": plist, "filter": app.current.filter, "searchstr": app.current.search, "offset": 0}});
        showNotification('Added '+ parseInt(document.getElementById('panel-heading-search').innerText) +' songs from search to ' + plist, '', '', 'success');
    }
}

function addAllFromBrowseDatabasePlist(plist) {
    if (app.current.search.length >= 2) {
        sendAPI({"cmd": "MPD_API_DATABASE_SEARCH", "data": {"plist": plist, "filter": app.current.view, "searchstr": app.current.search, "offset": 0}});
        showNotification('Added songs from database selection to ' + plist, '', '', 'success');
    }
}

function scrollTo(pos) {
    document.body.scrollTop = pos; // For Safari
    document.documentElement.scrollTop = pos; // For Chrome, Firefox, IE and Opera
}

function gotoPage(x) {
    switch (x) {
        case 'next':
            app.current.page += settings.maxElementsPerPage;
            break;
        case 'prev':
            app.current.page -= settings.maxElementsPerPage;
            if (app.current.page < 0)
                app.current.page = 0;
            break;
        default:
            app.current.page = x;
    }
    appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + app.current.search);
}

function saveQueue() {
    var plName = document.getElementById('saveQueueName').value;
    var valid = plName.replace(/\w\-/g, '');
    if (plName != '' && valid == '') {
        sendAPI({"cmd": "MPD_API_QUEUE_SAVE", "data": {"plist": plName}});
        modalSavequeue.hide();
    }
    else {
        document.getElementById('saveQueueName').classList.add('is-invalid');
        document.getElementById('saveQueueFrm').classList.add('was-validated');
    }
}

function showNotification(notificationTitle,notificationText,notificationHtml,notificationType) {
    if (settings.notificationWeb == true) {
        var notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
        setTimeout(function(notification) {
            notification.close();
        }, 3000, notification);    
    } 
    if (settings.notificationPage == true) {
        var alertBox;
        if (!document.getElementById('alertBox')) {
            alertBox = document.createElement('div');
            alertBox.setAttribute('id', 'alertBox');
            alertBox.addEventListener('click', function() {
                hideNotification();
            }, false);
        }
        else {
            alertBox = document.getElementById('alertBox');
        }
        alertBox.classList.remove('alert-success', 'alert-danger');
        alertBox.classList.add('alert','alert-' + notificationType);
        alertBox.innerHTML = '<div><strong>' + notificationTitle + '</strong><br/>' + notificationHtml + '</div>';
        document.getElementsByTagName('main')[0].append(alertBox);
        document.getElementById('alertBox').classList.add('alertBoxActive');
        if (alertTimeout)
            clearTimeout(alertTimeout);
        alertTimeout = setTimeout(function() {
            hideNotification();    
        }, 3000);
    }
}

function hideNotification() {
    if (document.getElementById('alertBox')) {
        document.getElementById('alertBox').classList.remove('alertBoxActive');
        setTimeout(function() {
            var alertBox = document.getElementById('alertBox');
            if (alertBox)
                alertBox.remove();
        }, 600);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function songChange(obj) {
    if (obj.type == 'error' || obj.type == 'result') 
        return;
    var curSong = obj.data.title + obj.data.artist + obj.data.album + obj.data.uri + obj.data.currentSongId;
    if (lastSong == curSong) 
        return;
    var textNotification = '';
    var htmlNotification = '';
    var pageTitle = 'myMPD: ';

    domCache.currentCover.style.backgroundImage = 'url("' + obj.data.cover + '")';

    if (typeof obj.data.artist != 'undefined' && obj.data.artist.length > 0 && obj.data.artist != '-') {
        textNotification += obj.data.artist;
        htmlNotification += obj.data.artist;
        pageTitle += obj.data.artist + ' - ';
        domCache.currentArtist.innerText = obj.data.artist;
        domCache.currentArtist.setAttribute('data-albumartist', obj.data.albumartist);
    } else
        domCache.currentArtist.innerText = '';

    if (typeof obj.data.album != 'undefined' && obj.data.album.length > 0 && obj.data.album != '-') {
        textNotification += ' - ' + obj.data.album;
        htmlNotification += '<br/>' + obj.data.album;
        domCache.currentAlbum.innerText = obj.data.album;
        domCache.currentAlbum.setAttribute('data-album', obj.data.album);
    }
    else
        domCache.currentAlbum.innerText = '';

    if (typeof obj.data.title != 'undefined' && obj.data.title.length > 0) {
        pageTitle += obj.data.title;
        domCache.currentTrack.innerText = obj.data.title;
        domCache.currentTrack.setAttribute('data-uri', obj.data.uri);
    } else {
        domCache.currentTrack.innerText = '';
        domCache.currentTrack.setAttribute('data-uri', '');
    }

    document.title = pageTitle;

    if (settings.stickers == true) {
        setVoteSongBtns(obj.data.like, obj.data.uri);
    }
    
    //Update Artist in queue view for http streams
    var playingTr = document.getElementById('queueTrackId' + obj.data.currentSongId);
    if (playingTr)
        playingTr.getElementsByTagName('td')[1].innerText = obj.data.title;

    showNotification(obj.data.title, textNotification, htmlNotification, 'success');
    lastSong = curSong;
}

function doSetFilterLetter(x) {
    var af = document.getElementById(x + 'Letters').getElementsByClassName('active')[0];
    if (af)
        af.classList.remove('active');
    var filter = app.current.filter;
    if (filter == '0')
        filter = '#';
    
    document.getElementById(x).innerText = 'Filter' + (filter != '-' ? ': '+filter : '');
    
    if (filter != '-') {
        var btns = document.getElementById(x + 'Letters').getElementsByTagName('button');
        var btnsLen = btns.length;
        for (var i = 0; i < btnsLen; i++) {
            if (btns[i].innerText == filter) {
                btns[i].classList.add('active');
                break;
            }
        }
    }
}

function addFilterLetter(x) {
    var filter = '<button class="mr-1 mb-1 btn btn-sm btn-secondary material-icons material-icons-small">delete</button>' +
        '<button class="mr-1 mb-1 btn btn-sm btn-secondary">#</button>';
    for (var i = 65; i <= 90; i++)
        filter += '<button class="mr-1 mb-1 btn-sm btn btn-secondary">' + String.fromCharCode(i) + '</button>';

    var letters = document.getElementById(x);
    letters.innerHTML = filter;
    
    letters.addEventListener('click', function(event) {
        switch (event.target.innerText) {
            case 'delete':
                filter = '-';
                break;
            case '#':
                filter = '0';
                break;
            default:
                filter = event.target.innerText;
        }
        appGoto(app.current.app, app.current.tab, app.current.view, '0/' + filter + '/' + app.current.search);
    }, false);
}

function selectTag(btnsEl, desc, setTo) {
    var btns = document.getElementById(btnsEl);
    var aBtn = btns.querySelector('.active')
    if (aBtn)
        aBtn.classList.remove('active');
    aBtn = btns.querySelector('[data-tag=' + setTo + ']');
    if (aBtn) {
        aBtn.classList.add('active');
        document.getElementById(desc).innerText = aBtn.innerText;
    }
}

function addTagList(x, any) {
    var tagList = '';
    if (any == true)
        tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="any">Any Tag</button>';
    for (var key in settings.tags) {
        if (settings.tags[key] == true && key != 'Track') {
            tagList += '<button type="button" class="btn btn-secondary btn-sm btn-block" data-tag="' + key + '">' + key + '</button>';
        }
    }
    var tagListEl = document.getElementById(x);
    tagListEl.innerHTML = tagList;
}

function gotoTagList() {
    appGoto(app.current.app, app.current.tab, app.current.view, '0/-/');
}

function chVolume(increment) {
    var newValue = parseInt(domCache.volumeBar.value) + increment;
    if (newValue < 0) 
        newValue = 0;
    else if (newValue > 100)
        newValue = 100;
    domCache.volumeBar.value = newValue;
    sendAPI({"cmd": "MPD_API_PLAYER_VOLUME", "data": {"volume": newValue}});
}

function beautifyDuration(x) {
    var days = Math.floor(x / 86400);
    var hours = Math.floor(x / 3600) - days * 24;
    var minutes = Math.floor(x / 60) - hours * 60 - days * 1440;
    var seconds = x - days * 86400 - hours * 3600 - minutes * 60;

    return (days > 0 ? days + '\u2009d ' : '') +
        (hours > 0 ? hours + '\u2009h ' + (minutes < 10 ? '0' : '') : '') +
        minutes + '\u2009m ' + (seconds < 10 ? '0' : '') + seconds + '\u2009s';
}

function genId(x) {
    return 'id' + x.replace(/[^\w\-]/g, '');
}

//Init app
appInit();
