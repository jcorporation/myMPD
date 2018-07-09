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
var last_song = '';
var last_state;
var current_song = new Object();
var playstate = '';
var settings = {};
var alertTimeout;

var app = {};
app.apps = { "Playback": { "state": "0/-/", "scrollPos": 0 },
             "Queue": 	 { "state": "0/Any Tag/", "scrollPos": 0 },
             "Browse":   { 
                  "active": "Database", 
                  "tabs":  { "Filesystem": { "state": "0/-/", "scrollPos": 0 },
                             "Playlists":  { "state": "0/-/", "scrollPos": 0 },
                             "Database":   { 
                                    "active": "Artist",
                                    "views": { "Artist": { "state": "0/-/", "scrollPos": 0 },
                                               "Album":  { "state": "0/-/", "scrollPos": 0 }
                                     }
                             }
                  }
             },
             "Search": { "state": "0/Any Tag/", "scrollPos": 0 }
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
domCache.btnPlay = document.getElementById('btnPlay');
domCache.btnPrev = document.getElementById('btnPrev');
domCache.btnNext = document.getElementById('btnNext');
domCache.progressBar = document.getElementById('progressBar');
domCache.volumeBar = document.getElementById('volumeBar');
domCache.outputs = document.getElementById('outputs');

var modalConnectionError = new Modal(document.getElementById('modalConnectionError'));
var modalSettings = new Modal(document.getElementById('modalSettings'));
var modalAddstream = new Modal(document.getElementById('modalAddstream'));
var modalSavequeue = new Modal(document.getElementById('modalSavequeue'));
var modalSongDetails = new Modal(document.getElementById('modalSongDetails'));
var mainMenu = new Dropdown(document.getElementById('mainMenu'));

function appPrepare(scrollPos) {
    if (app.current.app != app.last.app || app.current.tab != app.last.tab || app.current.view != app.last.view) {
        //Hide all cards + nav
        for (var i = 0; i < domCache.navbarBottomBtnsLen; i ++) {
            domCache.navbarBottomBtns[i].classList.remove('active');
        }
        document.getElementById('cardPlayback').classList.add('hide');
        document.getElementById('cardQueue').classList.add('hide');
        document.getElementById('cardBrowse').classList.add('hide');
        document.getElementById('cardSearch').classList.add('hide');
        for (var i = 0; i < domCache.panelHeadingBrowseLen; i ++) {
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
        sendAPI({"cmd":"MPD_API_GET_CURRENT_SONG"}, songChange);
    }    
    else if (app.current.app == 'Queue' ) {
        document.getElementById('QueueList').classList.add('opacity05');
        if (app.last.app != app.current.app) {
            if (app.current.search.length < 2) {
                setPagination(app.current.page);        
            }
        }
        var btns = document.getElementById('searchqueuetag').getElementsByTagName('button');
        for (var i = 0; i < btns.length; i ++) {
            btns[i].classList.remove('active');
            if (btns[i].innerText == app.current.filter) { 
                btns[i].classList.add('active'); 
                document.getElementById('searchqueuetagdesc').innerText = btns[i].innerText;
            }
        }
        getQueue();
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists') {
        document.getElementById('BrowsePlaylistsList').classList.add('opacity05');
        sendAPI({"cmd":"MPD_API_GET_PLAYLISTS","data": {"offset": app.current.page, "filter": app.current.filter}},parsePlaylists);
        doSetFilterLetter('BrowsePlaylistsFilter');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Artist') {
        document.getElementById('BrowseDatabaseArtistList').classList.add('opacity05');
        sendAPI({"cmd":"MPD_API_GET_ARTISTS","data": {"offset": app.current.page, "filter": app.current.filter}}, parseListDBtags);
        doSetFilterLetter('BrowseDatabaseFilter');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Album') {
        document.getElementById('BrowseDatabaseAlbumCards').classList.add('opacity05');
        sendAPI({"cmd":"MPD_API_GET_ARTISTALBUMS","data": {"offset": app.current.page, "filter": app.current.filter, "albumartist": app.current.search}}, parseListDBtags);
        doSetFilterLetter('BrowseDatabaseFilter');
    }    
    else if (app.current.app == 'Browse' && app.current.tab == 'Filesystem') {
        document.getElementById('BrowseFilesystemList').classList.add('opacity05');
        sendAPI({"cmd":"MPD_API_GET_FILESYSTEM","data": {"offset": app.current.page, "path": (app.current.search ? app.current.search : "/"), "filter": app.current.filter}}, parseFilesystem);
        // Don't add all songs from root
        if (app.current.search)
            document.getElementById('BrowseFilesystemAddAllSongs').removeAttribute('disabled');
        else
            document.getElementById('BrowseFilesystemAddAllSongs').setAttribute('disabled', 'disabled')
        // Create breadcrumb
        var breadcrumbs='<li class="breadcrumb-item"><a data-uri="">root</a></li>';
        var pathArray = app.current.search.split('/');
        var pathArrayLen = pathArray.length;
        var fullPath = '';
        for (var i = 0; i < pathArrayLen; i ++) {
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
        for (var i = 0; i < breadcrumbItemsLen; i ++) {
            breadcrumbItems[i].addEventListener('click', function() {
	        appGoto('Browse', 'Filesystem', undefined, '0/' + app.current.filter + '/' + this.getAttribute('data-uri'));
            }, false);
        }
        doSetFilterLetter('BrowseFilesystemFilter');
    }
    else if (app.current.app == 'Search') {
        document.getElementById('searchstr2').focus();
        document.getElementById('SearchList').classList.add('opacity05');
        if (app.last.app != app.current.app) {
            if (app.current.search != '')
                document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                    '<tr><td><span class="material-icons">search</span></td>' +
                    '<td colspan="5">Searching...</td></tr>';
            else
                setPagination(app.current.page);        
                
            document.getElementById('searchstr2').value = app.current.search;
        }

        if (app.current.search.length >= 2) {
            sendAPI({"cmd":"MPD_API_SEARCH", "data": { "mpdtag": app.current.filter, "offset": app.current.page, "searchstr": app.current.search}}, parseSearch);
        } else {
            document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML = '';
            document.getElementById('searchAddAllSongs').setAttribute('disabled', 'disabled');
            document.getElementById('panel-heading-search').innerText = '';
            setPagination(app.current.page);
            document.getElementById('SearchList').classList.remove('opacity05');
        }
        
        var btns = document.getElementById('searchtags2').getElementsByTagName('button');
        var btnsLen = btns.length;
        for (var i = 0; i < btnsLen; i ++) {
            btns[i].classList.remove('active');
            if (btns[i].innerText == app.current.filter) { 
                btns[i].classList.add('active'); 
                document.getElementById('searchtags2desc').innerText = btns[i].innerText;
            }
        }
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
    sendAPI({"cmd":"MPD_API_GET_OUTPUTNAMES"}, parseOutputnames);

    webSocketConnect();

    domCache.volumeBar.value = 0;
    domCache.volumeBar.addEventListener('change', function(event) {
        sendAPI({"cmd": "MPD_API_SET_VOLUME","data": {"volume": domCache.volumeBar.value}});
    }, false);

    domCache.progressBar.value = 0;
    domCache.progressBar.addEventListener('change', function(event) {
        if(current_song && current_song.currentSongId >= 0) {
            var seekVal = Math.ceil(current_song.totalTime * (domCache.progressBar.value / 100));
            sendAPI({"cmd": "MPD_API_SET_SEEK", "data": {"songid":current_song.currentSongId,"seek": seekVal}});
        }
    }, false);
    
    document.getElementById('modalAbout').addEventListener('shown.bs.modal', function () {
        sendAPI({"cmd": "MPD_API_GET_STATS"}, parseStats);
    });
        
    document.getElementById('modalSettings').addEventListener('shown.bs.modal', function () {
        getSettings();
        document.getElementById('settingsFrm').classList.remove('was-validated');
        document.getElementById('inputCrossfade').classList.remove('is-invalid');
        document.getElementById('inputMixrampdb').classList.remove('is-invalid');
        document.getElementById('inputMixrampdelay').classList.remove('is-invalid');
    });

    document.getElementById('modalAddstream').addEventListener('shown.bs.modal', function () {
        document.getElementById('streamurl').focus();
    });
    
    document.getElementById('addstreamFrm').addEventListener('submit', function () {
        addStream();
    });
    
    document.getElementById('mainMenu').addEventListener('shown.bs.dropdown', function () {
        var si = document.getElementById('inputSearch');
        si.value = '';
        si.focus();
    });
    
    document.getElementById('inputSearch').addEventListener('click', function(event) {
        event.stopPropagation();
    });
    
    addFilterLetter('BrowseFilesystemFilterLetters');
    addFilterLetter('BrowseDatabaseFilterLetters');
    addFilterLetter('BrowsePlaylistsFilterLetters');

    var hrefs = document.querySelectorAll('button[data-href], a[data-href]');
    var hrefsLen = hrefs.length;
    for (var i = 0; i < hrefsLen; i ++) {
        hrefs[i].addEventListener('click', function(event) {
            event.preventDefault();
            event.stopPropagation();
            var cmd = JSON.parse(this.getAttribute('data-href').replace(/\'/g,'"'));
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

    var pd = document.querySelectorAll('.pages');
    var pdLen = pd.length;
    for (var i = 0; i < pdLen; i ++) {
        pd[i].addEventListener('click', function(event) {
            if (event.target.nodeName == 'BUTTON') {
                gotoPage(event.target.getAttribute('data-page'));
            }
        }, false);
    }

    document.getElementById('outputs').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON') 
            event.stopPropagation();
            sendAPI({"cmd": "MPD_API_TOGGLE_OUTPUT", "data": {"output": event.target.getAttribute('data-output-id'), "state": (event.target.classList.contains('active') ? 0 : 1)}});
            toggleBtn(event.target.id);
    }, false);
    
    document.getElementById('QueueList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') 
            sendAPI({"cmd": "MPD_API_PLAY_TRACK","data": {"track": event.target.parentNode.getAttribute('data-trackid')}});
        else if (event.target.nodeName == 'A') {
            event.preventDefault();
            showMenu(event.target);
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
            event.preventDefault();
            showMenu(event.target);
        }
    }, false);
    
    document.getElementById('BrowsePlaylistsList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('plist', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName == 'A') {
            event.preventDefault();
            showMenu(event.target);
        }
    }, false);
    
    document.getElementById('BrowseDatabaseArtistList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appGoto('Browse', 'Database', 'Album', '0/-/' + event.target.parentNode.getAttribute('data-uri'));
        }
    }, false);
    
    document.getElementById('SearchList').addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute("data-uri")), event.target.parentNode.getAttribute("data-name"));
        }
        else if (event.target.nodeName == 'A') {
            event.preventDefault();
            showMenu(event.target);
        }
    }, false);

    document.getElementById('searchtags2').addEventListener('click', function(event) {
        if (event.target.nodeName == 'BUTTON')
            appGoto(app.current.app, app.current.tab, app.current.view, '0/' + event.target.innerText + '/' + app.current.search);            
    }, false);

    document.getElementById('searchqueuestr').addEventListener('keyup', function(event) {
        appGoto(app.current.app, app.current.tab, app.current.view, '0/' + app.current.filter + '/' + this.value);
    }, false);

    document.getElementById('searchqueuetag').addEventListener('click', function (event) {
        if (event.target.nodeName == 'BUTTON')
            appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + event.target.innerText + '/' + app.current.search);
    }, false);

    document.getElementById('inputSearch').addEventListener('keypress', function (event) {
        if ( event.which == 13 )
            mainMenu.toggle();
    }, false);

    document.getElementById('search').addEventListener('submit', function () {
        var searchStr = document.getElementById('inputSearch').value;
        appGoto('Search', undefined, undefined, app.current.page + '/Any Tag/' + searchStr);
        document.getElementById('searchstr2').value = searchStr;
        return false;
    }, false);

    document.getElementById('search2').addEventListener('submit', function () {
        return false;
    }, false);

    document.getElementById('searchqueue').addEventListener('submit', function () {
        return false;
    }, false);

    document.getElementById('searchstr2').addEventListener('keyup', function (event) {
        appGoto('Search', undefined, undefined, '0/' + app.current.filter + '/' + this.value);
    }, false);

    window.addEventListener('hashchange', appRoute, false);
    
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
}

function webSocketConnect() {
    socket = new WebSocket(getWsUrl());

    try {
        socket.onopen = function() {
            console.log('connected');
            showNotification('Connected to myMPD', '', '', 'success');
            modalConnectionError.hide();
            appRoute();
        }

        socket.onmessage = function got_packet(msg) {
            if(msg.data === last_state || msg.data.length == 0)
                return;
                
            try {
                var obj = JSON.parse(msg.data);
            } catch(e) {
                console.log('Invalid JSON data received: ' + msg.data);
            }

            switch (obj.type) {
                case 'state':
                    parseState(obj);
                    break;
                case 'disconnected':
                    showNotification('myMPD lost connection to MPD', '', '', 'danger');
                    break;
                case 'update_queue':
                    if(app.current.app === 'Queue')
                        getQueue();
                    break;
                case 'song_change':
                    songChange(obj);
                    break;
                case 'error':
                    showNotification(obj.data, '', '', 'danger');
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
    document.getElementById('mpdstats_dbplaytime').innerText = beautifyDuration(obj.data.dbplaytime);
    document.getElementById('mpdstats_playtime').innerText = beautifyDuration(obj.data.playtime);
    document.getElementById('mpdstats_uptime').innerText = beautifyDuration(obj.data.uptime);
    var d = new Date(obj.data.dbupdated * 1000);
    document.getElementById('mpdstats_dbupdated').innerText = d.toUTCString();
    document.getElementById('mympdVersion').innerText = obj.data.mympd_version;
    document.getElementById('mpdVersion').innerText = obj.data.mpd_version;
}

function toggleBtn(btn, state) {
    var b = document.getElementById(btn);
    if (state == undefined)
        state = b.classList.contains('active') ? 0 : 1;
    if (state == 1) {
        b.classList.add('active');
        b.setAttribute('aria-pressed','true');
    } else {
        b.classList.remove('active');
        b.setAttribute('aria-pressed','false');
    }
}

function parseSettings(obj) {
    toggleBtn('btnRandom', obj.data.random);
    toggleBtn('btnConsume', obj.data.consume);
    toggleBtn('btnSingle', obj.data.single);
    toggleBtn('btnRepeat', obj.data.repeat);

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
                    obj.data.notificationWeb = 0;
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

    settings=obj.data;
    settings.mpdstream = 'http://';
    if (settings.mpdhost == '127.0.0.1' || settings.mpdhost == 'localhost')
        settings.mpdstream += window.location.hostname;
    else
        settings.mpdstream += settings.mpdhost;
    settings.mpdstream += ':' + settings.streamport + '/';
}

function getSettings() {
    sendAPI({"cmd": "MPD_API_GET_SETTINGS"}, parseSettings);
}

function parseOutputnames(obj) {
    var btns = '';
    var outputsLen = obj.data.outputs.length;
    for (var i = 0; i < outputsLen; i ++) {
        btns += '<button id="btnoutput' + obj.data.outputs[i].id +'" data-output-id="' + obj.data.outputs[i].id + '" class="btn btn-secondary btn-block">'+
                '<span class="material-icons float-left">volume_up</span> ' + obj.data.outputs[i].name + '</button>';
    }
    domCache.outputs.innerHTML = btns;
}

function parseState(obj) {
    if (JSON.stringify(obj) === JSON.stringify(last_state))
        return;

    //Set playstate
    if (obj.data.state == 1) {
        domCache.btnPlay.innerText = 'play_arrow';
        playstate = 'stop';
    } else if (obj.data.state == 2) {
        domCache.btnPlay.innerText = 'pause';
        playstate = 'play';
    } else {
        domCache.btnPlay.innerText = 'play_arrow';
	playstate = 'pause';
    }

    if (obj.data.nextsongpos == -1)
        domCache.btnNext.setAttribute('disabled','disabled');
    else
        domCache.btnNext.removeAttribute('disabled');
    
    if (obj.data.songpos <= 0)
        domCache.btnPrev.setAttribute('disabled','disabled');
    else
        domCache.btnPrev.removeAttribute('disabled');
    
    if (obj.data.queue_length == 0)
        domCache.btnPlay.setAttribute('disabled','disabled');
    else
        domCache.btnPlay.removeAttribute('disabled');

    //Set volume
    if (obj.data.volume == -1) {
      domCache.volumePrct.innerText('Volumecontrol disabled');
      domCache.volumeControl.classList.add('hide');
    } else {
        domCache.volumeControl.classList.remove('hide');
        domCache.volumePrct.innerText = obj.data.volume + ' %';
        if(obj.data.volume == 0)
            domCache.volumeIcon.innerText = 'volume_off';
        else if (obj.data.volume < 50)
            domCache.volumeIcon.innerText = 'volume_down';
        else
            domCache.volumeIcon.innerText = 'volume_up';
    }
    domCache.volumeBar.value = obj.data.volume;

    //Set play counters
    current_song.totalTime  = obj.data.totalTime;
    current_song.currentSongId = obj.data.currentsongid;
    var total_minutes = Math.floor(obj.data.totalTime / 60);
    var total_seconds = obj.data.totalTime - total_minutes * 60;
    var elapsed_minutes = Math.floor(obj.data.elapsedTime / 60);
    var elapsed_seconds = obj.data.elapsedTime - elapsed_minutes * 60;

    domCache.progressBar.value = Math.floor(100 * obj.data.elapsedTime / obj.data.totalTime);

    var counterText = elapsed_minutes + ":" + 
        (elapsed_seconds < 10 ? '0' : '') + elapsed_seconds + " / " +
        total_minutes + ":" + (total_seconds < 10 ? '0' : '') + total_seconds;
    domCache.counter.innerText = counterText;
    
    //Set playing track in queue view
    if (last_state) {
        var tr = document.getElementById('queueTrackId' + last_state.data.currentsongid);
        if (tr) {
            var trtds = tr.getElementsByTagName('td');
            trtds[4].innerText = tr.getAttribute('data-duration');
            trtds[0].classList.remove('material-icons');
            trtds[0].innerText = tr.getAttribute('data-songpos');
            tr.classList.remove('font-weight-bold');
        }
    }
    var tr = document.getElementById('queueTrackId' + obj.data.currentsongid);
    if (tr) {
        var trtds = tr.getElementsByTagName('td');
        trtds[4].innerText = counterText;
        trtds[0].classList.add('material-icons');
        trtds[0].innerText = 'play_arrow';
        tr.classList.add('font-weight-bold');
    }
    
    //Get current song on queue change for http streams
    if (last_state == undefined || obj.data.queue_version != last_state.data.queue_version)
        sendAPI({"cmd": "MPD_API_GET_CURRENT_SONG"}, songChange);
        
    //  Set outputs state                  
    var outputsLen = obj.data.outputs.length;
    for (var i = 0; i < outputsLen; i ++) {
        toggleBtn('btnoutput' + obj.data.outputs[i].id, obj.data.outputs[i].state);
    }

    last_state = obj;                    
}

function getQueue() {
    if (app.current.search.length >= 2) 
        sendAPI({"cmd": "MPD_API_SEARCH_QUEUE", "data": {"mpdtag":app.current.filter, "offset":app.current.page, "searchstr": app.current.search}}, parseQueue);
    else
        sendAPI({"cmd": "MPD_API_GET_QUEUE", "data": {"offset": app.current.page}}, parseQueue);
}

function parseQueue(obj) {
    if (app.current.app !== 'Queue')
        return;
    
    if (typeof(obj.totalTime) != undefined && obj.totalTime > 0 )
        document.getElementById('panel-heading-queue').innerText = obj.totalEntities + ' Songs – ' + beautifyDuration(obj.totalTime);
    else if (obj.totalEntities > 0)
        document.getElementById('panel-heading-queue').innerText = obj.totalEntities + ' Songs';
    else
        document.getElementById('panel-heading-queue').innerText = '';

    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app + 'List').getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    for (var i = 0; i < nrItems; i ++) {
        if (tr[i])
            if (tr[i].getAttribute('data-trackid') == obj.data[i].id)
                continue;
                
        var minutes = Math.floor(obj.data[i].duration / 60);
        var seconds = obj.data[i].duration - minutes * 60;
        var duration = minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
        var row = document.createElement('tr');
        
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
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
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
    if (obj.totalEntities > 0)
        document.getElementById('searchAddAllSongs').removeAttribute('disabled');
    else
        document.getElementById('searchAddAllSongs').setAttribute('disabled','disabled');                    
    parseFilesystem(obj);
}

function parseFilesystem(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Filesystem' && app.current.app !== 'Search')
        return;
    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app + (app.current.tab==undefined ? '' : app.current.tab) + 'List').getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    for (var i = 0; i < nrItems; i ++) {
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
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
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
        
    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app + app.current.tab + 'List').getElementsByTagName('tbody')[0];
    var tr = tbody.getElementsByTagName('tr');
    for (var i = 0; i < nrItems; i ++) {
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
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
        tr[i].remove();
    }

    setPagination(obj.totalEntities);
    
    if (nrItems == 0) 
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">No playlists found.</td></tr>'
    document.getElementById(app.current.app + (app.current.tab==undefined ? '' : app.current.tab) + 'List').classList.remove('opacity05');
}

function parseListDBtags(obj) {
    if(app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Artist') return;
  
    if (obj.tagtype == 'AlbumArtist') {
        document.getElementById('BrowseDatabaseAlbumCards').classList.add('hide');
        document.getElementById('BrowseDatabaseArtistList').classList.remove('hide');
        document.getElementById('btnBrowseDatabaseArtist').classList.add('hide');
        var nrItems = obj.data.length;
        var tbody = document.getElementById(app.current.app + app.current.tab + app.current.view + 'List').getElementsByTagName('tbody')[0];
        var tr = tbody.getElementsByTagName('tr');
        for (var i = 0; i < nrItems; i ++) {
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
        var tr_length=tr.length - 1;
        for (var i = tr_length; i >= nrItems; i --) {
            tr[i].remove();
        }

        setPagination(obj.totalEntities);

        if (nrItems == 0) 
            tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                              '<td colspan="5">No entries found.</td></tr>';
        document.getElementById('BrowseDatabaseArtistList').classList.remove('opacity05');                              
                               
    } else if (obj.tagtype == 'Album') {
        document.getElementById('BrowseDatabaseAlbumCards').classList.remove('hide');
        document.getElementById('BrowseDatabaseArtistList').classList.add('hide');
        document.getElementById('btnBrowseDatabaseArtist').classList.remove('hide');    
        var nrItems = obj.data.length;
        var cardContainer = document.getElementById('BrowseDatabaseAlbumCards');
        var cards = cardContainer.querySelectorAll('.col-md');
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
                             ' <a href="#"><img class="card-img-top" src="" ></a>' +
                             ' <div class="card-body">' +
                             '  <h5 class="card-title">' + obj.searchstr + '</h5>' +
                             '  <h4 class="card-title">' + obj.data[i].value + '</h4>' +
                             '  <table class="table table-sm table-hover" id="tbl' + id + '"><tbody></tbody></table'+
                             ' </div>'+
                             '</div>';
         
            if (i < cards.length)
                cards[i].replaceWith(card); 
            else 
                cardContainer.append(card);
                
            sendAPI({"cmd":"MPD_API_GET_ARTISTALBUMTITLES", "data": { "albumartist": obj.searchstr, "album": obj.data[i].value}}, parseListTitles);
        }
        var cards_length=cards.length - 1;
        for (var i = cards_length; i >= nrItems; i --) {
            cards[i].remove();
        }
        setPagination(obj.totalEntities);
        document.getElementById('BrowseDatabaseAlbumCards').classList.remove('opacity05');        
    }
}

function parseListTitles(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Album') 
          return;
  
    var id = genId(obj.album);
    var card = document.getElementById('card' + id)
    var tbody = card.getElementsByTagName('tbody')[0];
    var img = card.getElementsByTagName('img')[0];
    var imga = img.parentNode;
    img.setAttribute('src', obj.cover);
    imga.setAttribute('data-uri', encodeURI(obj.data[0].uri.replace(/\/[^\/]+$/,'')));
    imga.setAttribute('data-name', obj.album);
    imga.setAttribute('data-type', 'dir');
  
    var titleList = '';
    var nrItems = obj.data.length;
    for (var i = 0; i < nrItems; i ++) {
        titleList += '<tr data-type="song" data-name="' + obj.data[i].title + '" data-uri="' + encodeURI(obj.data[i].uri) + '">' +
                     '<td>' + obj.data[i].track + '</td><td>' + obj.data[i].title + '</td>' +
                     '<td><a href="#" class="material-icons color-darkgrey">playlist_add</a></td>' + 
                     '</tr>';
    }
    tbody.innerHTML = titleList;
  
    imga.addEventListener('click', function(event) {
        event.preventDefault();
        showMenu(this);
    }, false);

    tbody.parentNode.addEventListener('click', function(event) {
        if (event.target.nodeName == 'TD') {
            appendQueue('song', decodeURI(event.target.parentNode.getAttribute('data-uri')), event.target.parentNode.getAttribute('data-name'));
        }
        else if (event.target.nodeName == 'A') {
            event.preventDefault();
            showMenu(event.target);
        }
    }, false);
}

function setPagination(number) {
    var totalPages = Math.ceil(number / settings.max_elements_per_page);
    var cat = app.current.app + (app.current.tab == undefined ? '': app.current.tab);
    if (totalPages == 0) 
        totalPages = 1;
    var p = ['PaginationTop', 'PaginationBottom'];
    for (var i = 0; i < 2; i ++) {
        document.getElementById(cat + p[i] + 'Page').innerText = (app.current.page / settings.max_elements_per_page + 1) + ' / ' + totalPages;
        if (totalPages > 1) {
            document.getElementById(cat + p[i] + 'Page').removeAttribute('disabled');
            var pl = '';
            for (var j = 0; j < totalPages; j ++) {
                pl += '<button data-page="' + (j * settings.max_elements_per_page) + '" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">' +
                    ( j + 1) + '</button>';
            }
            document.getElementById(cat + p[i] + 'Pages').innerHTML = pl;
        } else {
            document.getElementById(cat + p[i] + 'Page').setAttribute('disabled', 'disabled');
        }
    
        if (number > app.current.page + settings.max_elements_per_page) {
            document.getElementById(cat + p[i] + 'Next').removeAttribute('disabled');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        } else {
            document.getElementById(cat + p[i] + 'Next').setAttribute('disabled', 'disabled');
            document.getElementById(cat + 'ButtonsBottom').classList.add('hide');
        }
    
        if (app.current.page > 0) {
            document.getElementById(cat + p[i] + 'Prev').removeAttribute('disabled');
            document.getElementById(cat + 'ButtonsBottom').classList.remove('hide');
        } else {
            document.getElementById(cat + p[i] + 'Prev').setAttribute('disabled', 'disabled');
        }
    }
}

function appendQueue(type, uri, name) {
    switch(type) {
        case 'song':
            sendAPI({"cmd": "MPD_API_ADD_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" added', '', '', 'success');
            break;
        case 'dir':
            sendAPI({"cmd": "MPD_API_ADD_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" added', '', '', 'success');
            break;
        case 'plist':
            sendAPI({"cmd": "MPD_API_ADD_PLAYLIST", "data": {"plist": uri}});
            showNotification('"' + name + '" added', '', '', 'success');
            break;
    }
}

function appendAfterQueue(type, uri, to, name) {
    switch(type) {
        case 'song':
            sendAPI({"cmd": "MPD_API_ADD_TRACK_AFTER", "data": {"uri": uri, "to": to}});
            showNotification('"' + name + '" added to pos ' + to, '', '', 'success');
            break;
        case 'dir':
            sendAPI({"cmd": "MPD_API_ADD_TRACK_AFTER", "data": {"uri": uri, "to": to}});
            showNotification('"' + name + '" added to pos ' + to, '', '', 'success');
            break;
    }
}

function replaceQueue(type, uri, name) {
    switch(type) {
        case 'song':
            sendAPI({"cmd": "MPD_API_REPLACE_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" replaced', '', '', 'success');
            break;
        case 'dir':
            sendAPI({"cmd": "MPD_API_REPLACE_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" replaced', '', '', 'success');
            break;
        case 'plist':
            sendAPI({"cmd": "MPD_API_REPLACE_PLAYLIST", "data": {"plist": uri}});
            showNotification('"' + name + '" replaced', '', '', 'success');
            break;
    }
}

function songDetails(uri) {
    sendAPI({"cmd": "MPD_API_GET_SONGDETAILS", "data": {"uri": uri}}, parseSongDetails);
    modalSongDetails.show();
}

function parseSongDetails(obj) {
    var modal = document.getElementById('modalSongDetails');
    modal.querySelector('.album-cover').style.backgroundImage = 'url("' + obj.data.cover + '")';
    modal.getElementsByTagName('h1')[0].innerText = obj.data.title;
    var tr = modal.getElementsByTagName('tr');
    var trLen = tr.length;
    for (var i = 0; i < trLen; i ++) {
        var key = tr[i].getAttribute('data-name');
        var value = obj.data[key];
        if (key == 'duration') {
            var minutes = Math.floor(value / 60);
            var seconds = value - minutes * 60;
            value = minutes + ':' + (seconds < 10 ? '0' : '') + seconds;        
        }
        else if (key == 'uri') {
            value = '<a class="text-success" href="/library/' + value + '">' + value + '</a>';
        }
        tr[i].getElementsByTagName('td')[1].innerHTML = value;
    }
}

function showMenu(el) {
    var type = el.getAttribute('data-type');
    var uri = decodeURI(el.getAttribute('data-uri'));
    var name = el.getAttribute('data-name');
    if (type == null || uri == null) {
        type = el.parentNode.parentNode.getAttribute('data-type');
        uri = decodeURI(el.parentNode.parentNode.getAttribute('data-uri'));
        name = el.parentNode.parentNode.getAttribute('data-name');
    }

    var menu = '';
    if ((app.current.app == 'Browse' && app.current.tab == 'Filesystem') || app.current.app == 'Search' ||
        (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Album')) {
        menu += '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'appendQueue\', \'options\': [\'' + type + '\',\'' + 
            uri + '\',\'' + name + '\']}">Append to queue</a>' +
            ( type != 'plist' ? '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'appendAfterQueue\', \'options\': [\'' + type + '\',\'' +
            uri + '\',' + last_state.data.nextsongpos + ',\'' + name + '\']}">Add after current playing song</a>' : '') +
            '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'replaceQueue\', \'options\': [\'' + type + '\',\'' + 
            uri + '\',\'' + name + '\']}">Replace queue</a>' +
//            ( type != 'plist' ? '<div class="dropdown-divider"></div><a class="dropdown-item" href="#">Add to playlist</a>' : '') +
            ( type != 'dir' ? '<div class="dropdown-divider"></div>' : '') +
            ( type == 'song' ? '<a class="dropdown-item" data-href="{\'cmd\': \'songDetails\', \'options\': [\'' + uri + '\']}" href="#">Songdetails</a>' : '');
//            ( type == 'plist' ? '<a class="dropdown-item" href="#">Show playlist</a>' : '');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists') {
        menu += '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'appendQueue\', \'options\': [\'' + type + '\',\'' + 
            uri + '\',\'' + name + '\']}">Append to queue</a>' +
            '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'replaceQueue\', \'options\': [\'' + type + '\',\'' + 
            uri + '\',\'' + name + '\']}">Replace queue</a>' +
            '<div class="dropdown-divider"></div>' +
/*            '<a class="dropdown-item" href="#">Show playlist</a>' +
            '<a class="dropdown-item" href="#">Rename playlist</a>' + */
            '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'delPlaylist\', \'options\': [\'' + 
            uri + '\',\'' + name + '\']}">Delete playlist</a>';
    }
    else if (app.current.app == 'Queue') {
        menu += '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'delQueueSong\', \'options\': [\'single\',' + 
            el.parentNode.parentNode.getAttribute('data-trackid') + ']}">Remove</a>' +
            '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'delQueueSong\', \'options\': [\'range\',0,'+ 
            el.parentNode.parentNode.getAttribute('data-songpos') + ']}">Remove all upwards</a>' +
            '<a class="dropdown-item" href="#" data-href="{\'cmd\': \'delQueueSong\', \'options\': [\'range\',' + 
            (parseInt(el.parentNode.parentNode.getAttribute('data-songpos'))-1) + ',-1]}">Remove all downwards</a>' +
            '<div class="dropdown-divider"></div>' +
            '<a class="dropdown-item" data-href="{\'cmd\': \'songDetails\', \'options\': [\'' + uri + '\']}" href="#">Songdetails</a>';
    }    
    if (el.Popover == undefined) {
        new Popover(el, { trigger: 'click', template: '<div class="popover" role="tooltip">' +
            '<div class="arrow"></div>' +
            '<div class="popover-content">' + menu + '</div>' +
            '</div>'});
        var popoverInit = el.Popover;
        el.addEventListener('shown.bs.popover', function(event) {
            document.querySelector('.popover-content').addEventListener('click', function(event) {
                event.preventDefault();
                event.stopPropagation();
                var cmd = JSON.parse(event.target.getAttribute('data-href').replace(/\'/g,'"'));
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
        }, false);
        popoverInit.show();
    }
}

function sendAPI(request, callback) {
    var ajaxRequest=new XMLHttpRequest();
    ajaxRequest.open('POST', '/api', true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState == 4)
            if (callback != undefined && typeof(callback) == 'function')
                if (ajaxRequest.responseText != '')
                    callback(JSON.parse(ajaxRequest.responseText));
    };
    ajaxRequest.send(JSON.stringify(request));
}

function openLocalPlayer() {
    window.open('/player.html#' + settings.mpdstream, 'LocalPlayer');
}

function updateDB() {
    sendAPI({"cmd": "MPD_API_UPDATE_DB"});
    showNotification('Updating MPD Database...', '', '', 'success');
}

function clickPlay() {
    if( playstate != 'play')
        sendAPI({"cmd": "MPD_API_SET_PLAY"});
    else
        sendAPI({"cmd": "MPD_API_SET_PAUSE"});
}

function clickStop() {
    sendAPI({"cmd": "MPD_API_SET_STOP"});
}

function clickPrev() {
    sendAPI({"cmd": "MPD_API_SET_PREV"});
}

function clickNext() {
    sendAPI({"cmd": "MPD_API_SET_NEXT"});
}

function delQueueSong(mode, start, end) {
    if (mode == 'range')
        sendAPI({"cmd": "MPD_API_RM_RANGE", "data": {"start": start, "end": end}});
    else if (mode == 'single')
        sendAPI({"cmd": "MPD_API_RM_TRACK", "data": { "track": start}});
}

function delPlaylist(plist, name) {
    sendAPI({"cmd": "MPD_API_RM_PLAYLIST", "data": {"plist": plist}});
    document.getElementById('BrowsePlaylistsList').querySelector('tr[data-uri=' + encodeURI(plist) + ']').remove();
}

function confirmSettings() {
    var formOK = true;
    var inputCrossfade = document.getElementById('inputCrossfade');
    if (!inputCrossfade.getAttribute('disabled')) {
        var value = parseInt(inputCrossfade.value);
        if (!isNaN(value)) {
            inputCrossfade.value = value;
        } else {
            inputCrossfade.classList.add('is-invalid');
            formOK = false;
        }
    }
    var inputMixrampdb = document.getElementById('inputMixrampdb');
    if (!inputMixrampdb.getAttribute('disabled')) {
        var value = parseFloat(inputMixrampdb.value);
        if (!isNaN(value)) {
            inputMixrampdb.value = value;
        } else {
            inputMixrampdb.classList.add('is-invalid');
            formOK = false;
        } 
    }
    var inputMixrampdelay = document.getElementById('inputMixrampdelay');
    if (!inputMixrampdelay.getAttribute('disabled')) {
        if (inputMixrampdelay.value == 'nan') inputMixrampdelay.value = '-1';
        var value = parseFloat(inputMixrampdelay.value);
        if (!isNaN(value)) {
            inputMixrampdelay.value = value;
        } else {
            inputMixrampdelay.classList.add('is-invalid');
            formOK = false;
        }
    }
    if (formOK == true) {
        var selectReplaygain = document.getElementById('selectReplaygain');
        sendAPI({"cmd":"MPD_API_SET_SETTINGS", "data": {
            "consume": (document.getElementById('btnConsume').classList.contains('active') ? 1 : 0),
            "random":  (document.getElementById('btnRandom').classList.contains('active') ? 1 : 0),
            "single":  (document.getElementById('btnSingle').classList.contains('active') ? 1 : 0),
            "repeat":  (document.getElementById('btnRepeat').classList.contains('active') ? 1 : 0),
            "replaygain": selectReplaygain.options[selectReplaygain.selectedIndex].value,
            "crossfade": document.getElementById('inputCrossfade').value,
            "mixrampdb": document.getElementById('inputMixrampdb').value,
            "mixrampdelay": document.getElementById('inputMixrampdelay').vaue,
            "notificationWeb": (document.getElementById('btnnotifyWeb').classList.contains('active') ? 1 : 0),
            "notificationPage": (document.getElementById('btnnotifyPage').classList.contains('active') ? 1 : 0)
        }}, getSettings);
        modalSettings.hide();
    } else {
        document.getElementById('settingsFrm').classList.add('was-validated');
    }
}

function addAllFromBrowse() {
    sendAPI({"cmd": "MPD_API_ADD_TRACK", "data": { "uri": app.current.search}});
    showNotification('Added all songs', '', '', 'success');
}

function addAllFromSearch() {
    if (app.current.search.length >= 2) {
        sendAPI({"cmd":"MPD_API_SEARCH_ADD", "data":{"filter": app.current.filter, "searchstr": app.current.search}});
        showNotification('Added '+ parseInt(document.getElementById('panel-heading-search').innerText) +' songs from search', '', '', 'success');
    }
}

function scrollTo(pos) {
    document.body.scrollTop = pos; // For Safari
    document.documentElement.scrollTop = pos; // For Chrome, Firefox, IE and Opera
}

function gotoPage(x) {
    switch (x) {
        case 'next':
            app.current.page += settings.max_elements_per_page;
            break;
        case 'prev':
            app.current.page -= settings.max_elements_per_page;
            if (app.current.page <= 0)
                app.current.page = 0;
            break;
        default:
            app.current.page = x;
    }
    appGoto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + app.current.search);
}

function addStream() {
    var streamUrl = document.getElementById('streamurl');
    if (streamUrl.value != '')
        sendAPI({"cmd": "MPD_API_ADD_TRACK", "data": {"uri": streamUrl.value}});
    streamUrl.value = '';
    modalAddstream.hide();
}

function saveQueue() {
    var plName = document.getElementById('playlistname');
    if (plName.value != '')
        sendAPI({"cmd":"MPD_API_SAVE_QUEUE", "data": {"plist": plName.value}});
    plName.value = '';
    modalSavequeue.hide();
}

function showNotification(notificationTitle,notificationText,notificationHtml,notificationType) {
    if (settings.notificationWeb == 1) {
      var notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
      setTimeout(function(notification) {
        notification.close();
      }, 3000, notification);    
    } 
    if (settings.notificationPage == 1) {
        var alertBox;
        if (!document.getElementById('alertBox')) {
            alertBox = document.createElement('div');
            alertBox.setAttribute('id', 'alertBox');
        }
        else {
            alertBox = document.getElementById('alertBox');
        }
        alertBox.classList.add('alert','alert-' + notificationType);
        alertBox.innerHTML = '<div><strong>' + notificationTitle + '</strong>' + notificationHtml + '</div>';
        document.getElementsByTagName('main')[0].append(alertBox);
        document.getElementById('alertBox').classList.add('alertBoxActive');
        if (alertTimeout)
            clearTimeout(alertTimeout);
        alertTimeout = setTimeout(function() {
            if (document.getElementById('alertBox'))
                document.getElementById('alertBox').classList.remove('alertBoxActive');
            setTimeout(function() {
                document.getElementById('alertBox').remove();
            }, 600);
        }, 3000);
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function songChange(obj) {
    var cur_song = obj.data.title + obj.data.artist + obj.data.album + obj.data.uri + obj.data.currentsongid;
    if (last_song == cur_song) 
        return;
    var textNotification = '';
    var htmlNotification = '';
    var pageTitle = 'myMPD: ';

    document.getElementById('album-cover').style.backgroundImage = 'url("' + obj.data.cover + '")';

    if(typeof obj.data.artist != 'undefined' && obj.data.artist.length > 0 && obj.data.artist != '-') {
        textNotification += obj.data.artist;
        htmlNotification += '<br/>' + obj.data.artist;
        pageTitle += obj.data.artist + ' - ';
        document.getElementById('artist').innerText = obj.data.artist;
    } else {
        document.getElementById('artist').innerText = '';
    }
    if(typeof obj.data.album != 'undefined' && obj.data.album.length > 0 && obj.data.album != '-') {
        textNotification += ' - ' + obj.data.album;
        htmlNotification += '<br/>' + obj.data.album;
        document.getElementById('album').innerText = obj.data.album;
    }
    else {
        document.getElementById('album').innerText = '';
    }
    if(typeof obj.data.title != 'undefined' && obj.data.title.length > 0) {
        pageTitle += obj.data.title;
        document.getElementById('currenttrack').innerText = obj.data.title;
    } else {
        document.getElementById('currenttrack').innerText = '';
    }
    document.title = pageTitle;
    //Update Artist in queue view for http streams
    var playingTr = document.getElementById('queueTrackId' + obj.data.currentsongid);
    if (playingTr)
        playingTr.getElementsByTagName('td')[1].innerText = obj.data.title;

    showNotification(obj.data.title, textNotification, htmlNotification, 'success');
    last_song = cur_song;
}

function doSetFilterLetter(x) {
    var af = document.getElementById(x + 'Letters').querySelector('.active');
    if (af)
        af.classList.remove('active');
    var filter = app.current.filter;
    if (filter == '0')
        filter = '#';
    
    document.getElementById(x).innerText = 'Filter' + (filter != '-' ? ': '+filter : '');
    
    if (filter != '-') {
        var btns = document.getElementById(x + 'Letters').getElementsByTagName('button');
        var btnsLen = btns.length;
        for (var i = 0; i < btnsLen; i ++) {
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
    for (i = 65; i <= 90; i ++) {
        filter += '<button class="mr-1 mb-1 btn-sm btn btn-secondary">' + String.fromCharCode(i) + '</button>';
    }
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

function chVolume(increment) {
    var newValue = parseInt(domCache.volumeBar.value) + increment;
    if (newValue < 0) 
        newValue = 0;
    else if (newValue > 100)
        newValue=100;
    domCache.volumeBar.value = newValue;
    sendAPI({"cmd": "MPD_API_SET_VOLUME", "data": {"volume": newValue}});
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
    return 'id'+x.replace(/[^\w]/g,'');
}

//Init app
appInit();
