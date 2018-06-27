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
var last_outputs;
var current_song = new Object();
var playstate = '';
var progressBar;
var volumeBar;
var settings = {};

var app = {};
app.apps = { "Playback": { "state": "0/-/" },
             "Queue": 	 { "state": "0/Any Tag/" },
             "Browse":   { 
                  "active": "Database", 
                  "tabs":  { "Filesystem": { "state": "0/-/" },
                             "Playlists":  { "state": "0/-/" },
                             "Database":   { 
                                    "active": "Artist",
                                    "views": { "Artist": { "state": "0/-/" },
                                               "Album":  { "state": "0/-/" }
                                     }
                             }
                  }
             },
             "Search": { "state": "0/Any Tag/" }
           };
           
app.current = { "app": "Playback", "tab": undefined, "view": undefined, "page": 0, "filter": "", "search": "" };
app.last = { "app": undefined, "tab": undefined, "view": undefined };

var domCache = {};
domCache.navbarBottomBtns = document.getElementById('navbar-bottom').getElementsByTagName('div');
domCache.navbarBottomBtnsLen = domCache.navbarBottomBtns.length;
domCache.panelHeadingBrowse = document.getElementById('panel-heading-browse').getElementsByTagName('a');
domCache.panelHeadingBrowseLen = domCache.panelHeadingBrowse.length;
domCache.counter = document.getElementById('counter');

app.prepare=function() {
    if (app.current.app != app.last.app || app.current.tab != app.last.tab || app.current.view != app.last.view) {
        //Hide all cards + nav
        for ( var i = 0; i < domCache.navbarBottomBtnsLen; i ++) {
            domCache.navbarBottomBtns[i].classList.remove('active');
        }
        document.getElementById('cardPlayback').classList.add('hide');
        document.getElementById('cardQueue').classList.add('hide');
        document.getElementById('cardBrowse').classList.add('hide');
        document.getElementById('cardSearch').classList.add('hide');
        for ( var i = 0; i < domCache.panelHeadingBrowseLen; i ++) {
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
    }
}

app.goto=function(a,t,v,s) {
    var hash='';
    if (app.apps[a].tabs) {
        if (t == undefined) 
            t = app.apps[a].active;
        if (app.apps[a].tabs[t].views) {
            if (v == undefined) 
                v = app.apps[a].tabs[t].active;
            hash = '/'+a+'/'+t+'/'+v+'!'+ (s == undefined ? app.apps[a].tabs[t].views[v].state : s);
        } else {
            hash = '/'+a+'/'+t+'!'+ (s == undefined ? app.apps[a].tabs[t].state : s);
        }
    } else {
        hash = '/'+a+'!'+ (s == undefined ? app.apps[a].state : s);
    }
    location.hash=hash;
}

app.route=function() {
    var hash = decodeURI(location.hash);
    if (params=hash.match(/^\#\/(\w+)\/?(\w+)?\/?(\w+)?\!((\d+)\/([^\/]+)\/(.*))$/)) {
        app.current.app = params[1];
        app.current.tab = params[2];
        app.current.view = params[3];
        if (app.apps[app.current.app].state) {
            app.apps[app.current.app].state = params[4];
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].state) {
            app.apps[app.current.app].tabs[app.current.tab].state = params[4];
            app.apps[app.current.app].active = app.current.tab;
        }
        else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state) {
            app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state = params[4];
            app.apps[app.current.app].active = app.current.tab;
            app.apps[app.current.app].tabs[app.current.tab].active = app.current.view;
        }
        app.current.page = parseInt(params[5]);
        app.current.filter = params[6];
        app.current.search = params[7];
    } else {
        app.goto("Playback");
        return;
    }

    app.prepare();
    if (app.current.app == 'Playback') {
        sendAPI({"cmd":"MPD_API_GET_CURRENT_SONG"}, songChange);
    }    
    else if (app.current.app == 'Queue' ) {
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
        sendAPI({"cmd":"MPD_API_GET_PLAYLISTS","data": {"offset": app.current.page, "filter": app.current.filter}},parsePlaylists);
        doSetFilterLetter('#browsePlaylistsFilter');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Artist') {
        sendAPI({"cmd":"MPD_API_GET_ARTISTS","data": {"offset": app.current.page, "filter": app.current.filter}}, parseListDBtags);
        doSetFilterLetter('#BrowseDatabaseFilter');
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Album') {
        sendAPI({"cmd":"MPD_API_GET_ARTISTALBUMS","data": {"offset": app.current.page, "filter": app.current.filter, "albumartist": app.current.search}}, parseListDBtags);
        doSetFilterLetter('#BrowseDatabaseFilter');
    }    
    else if (app.current.app == 'Browse' && app.current.tab == 'Filesystem') {
        sendAPI({"cmd":"MPD_API_GET_FILESYSTEM","data": {"offset": app.current.page, "path": (app.current.search ? app.current.search : "/"), "filter": app.current.filter}}, parseFilesystem);
        // Don't add all songs from root
        if (app.current.search)
            document.getElementById('BrowseFilesystemAddAllSongs').removeAttribute('disabled');
        else
            document.getElementById('BrowseFilesystemAddAllSongs').setAttribute('disabled', 'disabled')
        // Create breadcrumb
        var breadcrumbs=['<li class="breadcrumb-item"><a data-uri="">root</a></li>'];
        var pathArray = app.current.search.split('/');
        var pathArrayLen = pathArray.length;
        var fullPath = '';
        for (var i = 0; i < pathArrayLen; i ++) {
            if (pathArrayLen -1 == i) {
                breadcrumbs.push('<li class="breadcrumb-item active">' + pathArray[i] + '</li>');
                break;
            }
            fullPath += pathArray[i];
            breadcrumbs.push('<li class="breadcrumb-item"><a data-uri="' + fullPath + '">' + pathArray[i] + '</a></li>');
            fullPath += '/';
        }
        var elBrowseBreadcrumb=document.getElementById('BrowseBreadcrumb');
        elBrowseBreadcrumb.innerHTML = breadcrumbs.join('');
        var breadcrumbItems = elBrowseBreadcrumb.getElementsByTagName('a');
        var breadcrumbItemsLen = breadcrumbItems.length;
        for (var i = 0; i < breadcrumbItemsLen; i ++) {
            breadcrumbItems[i].addEventListener('click', function() {
	        app.goto('Browse', 'Filesystem', undefined, '0/' + app.current.filter + '/' + this.getAttribute('data-uri'));
            }, false);
        }
        doSetFilterLetter('#BrowseFilesystemFilter');
    }
    else if (app.current.app == 'Search') {
        if (app.last.app != app.current.app) {
            if (app.current.search != '')
                document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML=
                    '<tr><td><span class="material-icons">search</span></td>' +
                    '<td colspan="5">Searching...</td></tr>';
            else
                setPagination(app.current.page);        
                
            document.getElementById('searchstr2').value=app.current.search;
        }

        if (app.current.search.length >= 2) {
           sendAPI({"cmd":"MPD_API_SEARCH", "data": { "mpdtag": app.current.filter, "offset": app.current.page, "searchstr": app.current.search}}, parseSearch);
        } else {
          document.getElementById('SearchList').getElementsByTagName('tbody')[0].innerHTML = '';
          document.getElementById('searchAddAllSongs').setAttribute('disabled','disabled');  
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
        app.goto("Playback");
    }

    app.last.app = app.current.app;
    app.last.tab = app.current.tab;
    app.last.view = app.current.view;
};

$(document).ready(function(){
    getSettings();
    sendAPI({"cmd":"MPD_API_GET_OUTPUTNAMES"},parseOutputnames);

    webSocketConnect();

    volumeBar=$('#volumebar').slider();
    volumeBar.slider('setValue',0);
    volumeBar.slider('on','slideStop', function(value){
      sendAPI({"cmd":"MPD_API_SET_VOLUME","data": {"volume":value}});
    });

    progressBar=$('#progressbar').slider();
    progressBar.slider('setValue',0);
    
    progressBar.slider('on','slideStop', function(value){
        if(current_song && current_song.currentSongId >= 0) {
            var seekVal = Math.ceil(current_song.totalTime*(value/100));
            sendAPI({"cmd":"MPD_API_SET_SEEK", "data": {"songid":current_song.currentSongId,"seek":seekVal}});
        }
    });

    $('#about').on('shown.bs.modal', function () {
      sendAPI({"cmd":"MPD_API_GET_STATS"},parseStats);
    })
    
    $('#settings').on('shown.bs.modal', function () {
      sendAPI({"cmd":"MPD_API_GET_SETTINGS"},parseSettings);      
      document.getElementById('settingsFrm').classList.remove('was-validated');
      document.getElementById('inputCrossfade').classList.remove('is-invalid');
      document.getElementById('inputMixrampdb').classList.remove('is-invalid');
      document.getElementById('inputMixrampdelay').classList.remove('is-invalid');
    })

    $('#addstream').on('shown.bs.modal', function () {
        $('#streamurl').focus();
    })
    
    $('#addstream form').on('submit', function (e) {
        addStream();
    });
    
    $('#mainMenu').on('shown.bs.dropdown', function () {
        $('#search > input').val('');
        $('#search > input').focus();
    });
     
    add_filter('#BrowseFilesystemFilterLetters');
    add_filter('#BrowseDatabaseFilterLetters');
    add_filter('#BrowsePlaylistsFilterLetters');

    document.getElementById('BrowseFilesystemAddAllSongs').addEventListener('click',function() {
        sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": { "uri": app.current.search}});
    },false);
    
    $('#cardBrowseNavFilesystem').on('click', function (e) {
        app.goto('Browse','Filesystem');
        e.preventDefault();
    });

    $('#cardBrowseNavDatabase').on('click', function (e) {
        app.goto('Browse','Database');
        e.preventDefault();
    });

    $('#btnBrowseDatabaseArtist').on('click', function (e) {
        app.goto('Browse','Database','Artist');
        e.preventDefault();
    });

    $('#cardBrowseNavPlaylists').on('click', function (e) {
        app.goto('Browse','Playlists');
        e.preventDefault();
    });

    $('#cardBrowseNavFilesystem').on('click', function (e) {
        app.goto('Browse','Filesystem');
        e.preventDefault();
    });

    $('#navPlayback').on('click', function (e) {
        app.goto('Playback');
        e.preventDefault();
    });

    $('#navQueue').on('click', function (e) {
        app.goto('Queue');
        e.preventDefault();
    });

    $('#navBrowse').on('click', function (e) {
        app.goto('Browse');
        e.preventDefault();
    });

    $('#navSearch').on('click', function (e) {
        app.goto('Search');
        e.preventDefault();
    });
    
    window.addEventListener('hashchange', app.route, false);
});

function webSocketConnect() {
    if (typeof MozWebSocket != "undefined") {
        socket = new MozWebSocket(get_appropriate_ws_url());
    } else {
        socket = new WebSocket(get_appropriate_ws_url());
    }

    try {
        socket.onopen = function() {
            console.log("connected");
            showNotification('Connected to myMPD','','','success');
            $('#modalConnectionError').modal('hide');    
            app.route();
        }

        socket.onmessage = function got_packet(msg) {
            if(msg.data === last_state || msg.data.length == 0)
                return;
            try {
              var obj = JSON.parse(msg.data);
            } catch(e) {
              console.log('Invalid JSON data received: '+ msg.data);
            }

            switch (obj.type) {
                case 'state':
                    parseState(obj);
                    break;
                case 'disconnected':
                    showNotification('myMPD lost connection to MPD','','','danger');
                    break;
                case 'update_queue':
                    if(app.current.app === 'Queue')
                        getQueue();
                    break;
                case "song_change":
                    songChange(obj);
                    break;
                case 'error':
                    showNotification(obj.data,'','','danger');
                default:
                    break;
            }
        }

        socket.onclose = function(){
            console.log('disconnected');
            $('#modalConnectionError').modal('show');
            setTimeout(function() {
               console.log('reconnect');
               webSocketConnect();
            },3000);
        }

    } catch(exception) {
        alert('<p>Error' + exception);
    }

}

function get_appropriate_ws_url()
{
    var pcol;
    var u = document.URL;
    var separator;

    if (u.substring(0, 5) == "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) == "http")
            u = u.substr(7);
    }

    u = u.split('#');
    if (/\/$/.test(u[0])) {
        separator = "";
    } else {
        separator = "/";
    }
    return pcol + u[0] + separator + "ws";
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

function parseSettings(obj) {
  if (obj.data.random)
     $('#btnrandom').addClass("active").attr("aria-pressed","true");
  else
     $('#btnrandom').removeClass("active").attr("aria-pressed","false");

  if (obj.data.consume)
     $('#btnconsume').addClass("active").attr("aria-pressed","true");
  else
     $('#btnconsume').removeClass("active").attr("aria-pressed","false");

  if (obj.data.single)
     $('#btnsingle').addClass("active").attr("aria-pressed","true");
  else
     $('#btnsingle').removeClass("active").attr("aria-pressed","false");

  if (obj.data.repeat)
     $('#btnrepeat').addClass("active").attr("aria-pressed","true");
  else
     $('#btnrepeat').removeClass("active").attr("aria-pressed","false");

  if (obj.data.crossfade != undefined)
     $('#inputCrossfade').removeAttr('disabled').val(obj.data.crossfade);
  else
     $('#inputCrossfade').attr('disabled', 'disabled');
                    
  if (obj.data.mixrampdb != undefined)
     $('#inputMixrampdb').removeAttr('disabled').val(obj.data.mixrampdb);
  else
     $('#inputMixrampdb').attr('disabled', 'disabled');
                    
  if (obj.data.mixrampdelay != undefined)
     $('#inputMixrampdelay').removeAttr('disabled').val(obj.data.mixrampdelay);
  else
     $('#inputMixrampdb').attr('disabled', 'disabled');
                    
  $("#selectReplaygain").val(obj.data.replaygain);

  if (notificationsSupported()) {
      if (obj.data.notificationWeb) {
         $('#btnnotifyWeb').addClass("active").attr("aria-pressed","true");

         Notification.requestPermission(function (permission) {
            if(!('permission' in Notification))
                Notification.permission = permission;
            if (permission === 'granted') {
                $('#btnnotifyWeb').addClass("active").attr("aria-pressed","true");
            } else {
                $('#btnnotifyWeb').removeClass("active").attr("aria-pressed","false");
                obj.data.notificationWeb=0;
            }
         });         
      }
      else
         $('#btnnotifyWeb').removeClass("active").attr("aria-pressed","false");
  } else {
      $('#btnnotifyWeb').addClass("disabled");
      $('#btnnotifyWeb').removeClass("active").attr("aria-pressed","false");
  }
    
  if (obj.data.notificationPage)
      $('#btnnotifyPage').addClass("active").attr("aria-pressed","true");
  else
      $('#btnnotifyPage').removeClass("active").attr("aria-pressed","false");
  
  settings=obj.data;
  setLocalStream(obj.data.mpdhost,obj.data.streamport);
}

function getSettings() {
    sendAPI({"cmd":"MPD_API_GET_SETTINGS"}, parseSettings);
}

function parseOutputnames(obj) {
  $('#btn-outputs-block button').remove();
  if ( Object.keys(obj.data).length ) {
     $.each(obj.data, function(id, name){
        var btn = $('<button id="btnoutput'+id+'" class="btn btn-secondary btn-block" onclick="toggleoutput(this, '+id+')">'+
                    '<span class="material-icons float-left">volume_up</span> '+name+'</button>');
        btn.appendTo($('#btn-outputs-block'));
      });
  } else {
     $('#btn-outputs-block').addClass('hide');
  }
  /* remove cache, since the buttons have been recreated */
  last_outputs = '';
}

function parseState(obj) {
    updatePlayIcon(obj);
    updateVolumeIcon(obj.data.volume);

    if(JSON.stringify(obj) === JSON.stringify(last_state))
        return;

    current_song.totalTime  = obj.data.totalTime;
    current_song.currentSongId = obj.data.currentsongid;
    var total_minutes = Math.floor(obj.data.totalTime / 60);
    var total_seconds = obj.data.totalTime - total_minutes * 60;
    var elapsed_minutes = Math.floor(obj.data.elapsedTime / 60);
    var elapsed_seconds = obj.data.elapsedTime - elapsed_minutes * 60;

    volumeBar.slider('setValue', obj.data.volume);
    var progress = Math.floor(100 * obj.data.elapsedTime / obj.data.totalTime);
    progressBar.slider('setValue', progress);

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
        sendAPI({"cmd":"MPD_API_GET_CURRENT_SONG"}, songChange);
                    
    last_state = obj;
                    
    $.each(obj.data.outputs, function(id, enabled){
        if (enabled)
            $('#btnoutput'+id).addClass("active")
        else
            $('#btnoutput'+id).removeClass("active");
    });
                    
    last_outputs = obj.data.outputs;                    
}

function getQueue() {
    if (app.current.search.length >= 2) 
        sendAPI({"cmd":"MPD_API_SEARCH_QUEUE", "data": {"mpdtag":app.current.filter, "offset":app.current.page,"searchstr": app.current.search}}, parseQueue);
    else
        sendAPI({"cmd":"MPD_API_GET_QUEUE", "data": {"offset": app.current.page}}, parseQueue);
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
        row.innerHTML = '<td>' + (obj.data[i].pos + 1) + '</td>' +
                        '<td>' + obj.data[i].title + '</td>' +
                        '<td>' + obj.data[i].artist + '</td>' + 
                        '<td>' + obj.data[i].album + '</td>' +
                        '<td>' + duration + '</td>' +
                        '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td></tr>';
        if (i < tr.length)
            tr[i].replaceWith(row); 
        else 
            tbody.append(row);  
        
        tr[i].getElementsByTagName('a')[0].addEventListener('click', function(event) {
          event.stopPropagation();
          event.preventDefault();
          showMenu(this);
        },false);
            
        tr[i].addEventListener('click', function() {
            sendAPI({"cmd":"MPD_API_PLAY_TRACK","data": {"track":this.getAttribute('data-trackid')}});
        },false);
    }
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
        tr[i].remove();
    }                    
                        
    if (obj.type == 'queuesearch' && nrItems == 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">No results, please refine your search!</td></tr>';
    }
    else if (obj.type == 'queue' && nrItems == 0) {
        tbody.innerHTML = '<tr><td><span class="material-icons">error_outline</span></td>' +
                          '<td colspan="5">Empty queue</td></tr>';
    }

    setPagination(obj.totalEntities);
}

function parseSearch(obj) {
    if (app.current.app !== 'Search')
        return;
    document.getElementById('panel-heading-search').innerHTML=obj.totalEntities + ' Songs found';
    if (obj.totalEntities > 0)
        document.getElementById('searchAddAllSongs').removeAttribute('disabled');
    else
        document.getElementById('searchAddAllSongs').setAttribute('disabled','disabled');                    
    parseFilesystem(obj);
}

function parseFilesystem(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Filesystem' && app.current.app !== 'Search')
        return;
    var nrItems = obj.data.length;;
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
                      '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>';
                break;
            case 'song':
                var minutes = Math.floor(obj.data[i].duration / 60);
                var seconds = obj.data[i].duration - minutes * 60;
                row.innerHTML = '<td><span class="material-icons">music_note</span></td>' + 
                      '<td>' + obj.data[i].title + '</td>' +
                      '<td>' + obj.data[i].artist + '</td>' + 
                      '<td>' + obj.data[i].album  + '</td>' +
                      '<td>' + minutes + ':' + (seconds < 10 ? '0' : '') + seconds +
                      '</td><td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>';
                break;
            case 'plist':
                row.innerHTML = '<td><span class="material-icons">list</span></td>' +
                      '<td colspan="4">' + obj.data[i].name + '</td>' +
                      '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>';
                break;
        }
        if (i < tr.length)
            tr[i].replaceWith(row); 
        else 
            tbody.append(row);
            
        tr[i].getElementsByTagName('a')[0].addEventListener('click', function(event) {
          event.stopPropagation();
          event.preventDefault();
          showMenu(this);
        },false);
            
        tr[i].addEventListener('click', function() {
            switch(this.getAttribute('data-type')) {
                case 'dir':
                    app.goto('Browse', 'Filesystem', undefined, '0/' + app.current.filter +'/' + decodeURI(this.getAttribute("data-uri")));
                    break;
                case 'song':
                    appendQueue('song', decodeURI(this.getAttribute("data-uri")), this.getAttribute("data-name"));
                    break;
                case 'plist':
                    appendQueue('plist', decodeURI(this.getAttribute("data-uri")), this.getAttribute("data-name"));
                    break;
            }
        },false);
    }
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
        tr[i].remove();
    }
    
    setPagination(obj.totalEntities);
                    
    if (nrItems == 0)
        tbody.innerHTML='<tr><td><span class="material-icons">error_outline</span></td>' +
            '<td colspan="5">No results</td></tr>';
        
}

function parsePlaylists(obj) {
    if (app.current.app !== 'Browse' && app.current.tab !== 'Playlists')
        return;
        
    var nrItems = obj.data.length;
    var tbody = document.getElementById(app.current.app+app.current.tab+'List').getElementsByTagName('tbody')[0];
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
                        '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>';
        if (i < tr.length)
            tr[i].replaceWith(row); 
        else 
            tbody.append(row);
            
        tr[i].getElementsByTagName('a')[0].addEventListener('click', function(event) {
          event.stopPropagation();
          event.preventDefault();
          showMenu(this);
        },false);
            
        tr[i].addEventListener('click', function() {
            appendQueue('plist', decodeURI(this.getAttribute("data-uri")), this.getAttribute("data-name"));
        },false);
    }
    var tr_length=tr.length - 1;
    for (var i = tr_length; i >= nrItems; i --) {
        tr[i].remove();
    }

    setPagination(obj.totalEntities);
    
    if (nrItems == 0) 
        tbody.innerHTML='<tr><td><span class="material-icons">error_outline</span></td>' +
                        '<td colspan="5">No playlists found.</td></tr>'
    
}

function parseListDBtags(obj) {
    if(app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Artist') return;
  
    if (obj.tagtype == 'AlbumArtist') {
        $('#BrowseDatabaseAlbumCards').addClass('hide');
        $('#BrowseDatabaseArtistList').removeClass('hide');
        $('#btnBrowseDatabaseArtist').addClass('hide');
        var nrItems = obj.data.length;
        var tbody = document.getElementById(app.current.app+app.current.tab+app.current.view+'List').getElementsByTagName('tbody')[0];
        var tr = tbody.getElementsByTagName('tr');
        for (var i = 0; i < nrItems; i ++) {
            var uri = encodeURI(obj.data[i].value);
            if (tr[i])
                if (tr[i].getAttribute('data-uri') == uri)
                    continue;
            var row = document.createElement('tr');
            row.setAttribute('data-uri', uri);
            row.innerHTML='<td><span class="material-icons">album</span></td>' +
                          '<td>' + obj.data[i].value + '</td>' +
//                          '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>' +
                          '</tr>';

            if (i < tr.length)
                tr[i].replaceWith(row); 
            else 
                tbody.append(row);
/*            
            tr[i].getElementsByTagName('a')[0].addEventListener('click', function(event) {
                event.stopPropagation();
                event.preventDefault();
                showMenu(this);
            },false);
*/            
            tr[i].addEventListener('click', function() {
                app.goto('Browse','Database','Album','0/-/'+this.getAttribute('data-uri'));
            },false);
        }
        var tr_length=tr.length - 1;
        for (var i = tr_length; i >= nrItems; i --) {
            tr[i].remove();
        }

        setPagination(obj.totalEntities);

        if (nrItems == 0) 
            tbody.innerHTML='<tr><td><span class="material-icons">error_outline</span></td>' +
                            '<td colspan="5">No entries found.</td></tr>'
                               
    } else if (obj.tagtype == 'Album') {
        $('#BrowseDatabaseArtistList').addClass('hide');
        $('#BrowseDatabaseAlbumCards').removeClass('hide');
        $('#btnBrowseDatabaseArtist').removeClass('hide');
        var nrItems = obj.data.length;
        var cardContainer = document.getElementById('BrowseDatabaseAlbumCards')
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
            card.innerHTML='<div class="card mb-4" id="card' + id + '">' +
                           ' <img class="card-img-top" src="" tabindex="0" data-trigger="focus">' +
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
    }
}

function parseListTitles(obj) {
  if(app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Album') 
      return;
  
  var id = genId(obj.album);
  var card = document.getElementById('card' + id)
  var tbody = card.getElementsByTagName('tbody')[0];
  var img = card.getElementsByTagName('img')[0];
  img.setAttribute('src', obj.cover);
  img.setAttribute('data-uri', obj.data[0].uri.replace(/\/[^\/]+$/,''));
  img.setAttribute('data-name', encodeURI(obj.album));
  img.setAttribute('data-type', 'album');
  
  var titleList=new Array();
  var nrItems = obj.data.length;
  for (var i = 0; i < nrItems; i ++) {
      titleList.push('<tr data-type="song" data-name="' + obj.data[i].title + '" data-uri="' + encodeURI(obj.data[i].uri) + '">' +
                 '<td>' + obj.data[i].track + '</td><td>' + obj.data[i].title + '</td>' +
                 '<td><a href="#" tabindex="0" data-trigger="focus" class="material-icons">playlist_add</a></td>' + 
                 '</tr>');
  }
  tbody.innerHTML=titleList.join('');
  
  img.addEventListener('click', function() {
//      appendQueue('song', decodeURI(this.getAttribute('data-uri')), this.getAttribute('data-name'));
      showMenu(this);
  }, false);

  var tr = tbody.getElementsByTagName('tr');
  var tr_length = tr.length;
  for (var i = 0; i < tr_length; i ++) {
      tr[i].addEventListener('click', function() {
          appendQueue('song', decodeURI(this.getAttribute('data-uri')), this.getAttribute('data-name'));
      }, false);
      tr[i].getElementsByTagName('a')[0].addEventListener('click', function(event) {
          event.stopPropagation();
          event.preventDefault();
          showMenu(this);
      },false);
  }
}

function setPagination(number) {
    var totalPages=Math.ceil(number / settings.max_elements_per_page);
    var cat=app.current.app+(app.current.tab==undefined ? '': app.current.tab);
    if (totalPages ==0 ) { totalPages = 1; }
        $('#'+cat+'PaginationTopPage').text((app.current.page / settings.max_elements_per_page + 1) + ' / ' + totalPages);
        $('#'+cat+'PaginationBottomPage').text((app.current.page / settings.max_elements_per_page + 1) + ' / ' + totalPages);
    if (totalPages > 1) {
        $('#'+cat+'PaginationTopPage').removeClass('disabled').removeAttr('disabled');
        $('#'+cat+'PaginationBottomPage').removeClass('disabled').removeAttr('disabled');
        $('#'+cat+'PaginationTopPages').empty();
        $('#'+cat+'PaginationBottomPages').empty();
        for (var i=0;i<totalPages;i++) {
            $('#'+cat+'PaginationTopPages').append('<button onclick="gotoPage('+(i * settings.max_elements_per_page)+',this,event)" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">'+(i+1)+'</button>');
            $('#'+cat+'PaginationBottomPages').append('<button onclick="gotoPage('+(i * settings.max_elements_per_page)+',this,event)" type="button" class="mr-1 mb-1 btn-sm btn btn-secondary">'+(i+1)+'</button>');
        }
    } else {
        $('#'+cat+'PaginationTopPage').addClass('disabled').attr('disabled','disabled');
        $('#'+cat+'PaginationBottomPage').addClass('disabled').attr('disabled','disabled');
    }
    
    if(number > app.current.page + settings.max_elements_per_page) {
        $('#'+cat+'PaginationTopNext').removeClass('disabled').removeAttr('disabled');
        $('#'+cat+'PaginationBottomNext').removeClass('disabled').removeAttr('disabled');
        $('#'+cat+'ButtonsBottom').removeClass('hide');
    } else {
        $('#'+cat+'PaginationTopNext').addClass('disabled').attr('disabled','disabled');
        $('#'+cat+'PaginationBottomNext').addClass('disabled').attr('disabled','disabled');
        $('#'+cat+'ButtonsBottom').addClass('hide');
    }
    
    if(app.current.page > 0) {
        $('#'+cat+'PaginationTopPrev').removeClass('disabled').removeAttr('disabled');
        $('#'+cat+'PaginationBottomPrev').removeClass('disabled').removeAttr('disabled');
    } else {
        $('#'+cat+'PaginationTopPrev').addClass('disabled').attr('disabled','disabled');
        $('#'+cat+'PaginationBottomPrev').addClass('disabled').attr('disabled','disabled');
    }
}

function appendQueue(type,uri,name) {
    switch(type) {
        case 'song':
            sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": {"uri": uri}});
            showNotification('"' + name + '" added','','','success');
            break;
        case 'plist':
            sendAPI({"cmd":"MPD_API_ADD_PLAYLIST", "data": {"plist": uri}});
            showNotification('"' + name + '" added','','','success');
            break;
    }
}

function showMenu(el) {
    var type = el.parentNode.parentNode.getAttribute('data-type');
    var uri = el.parentNode.parentNode.getAttribute('data-uri');
    if ((app.current.app == 'Browse' && app.current.tab == 'Filesystem') || app.current.app == 'Search' ||
        (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Album')) {
        $(el).popover({html:true, content:'<a class="dropdown-item" href="#" onclick="">Append to queue</a>' +
            '<a class="dropdown-item" href="#">Add after current playing song</a>' +
            '<a class="dropdown-item" href="#">Replace queue</a>' +
            ( type != 'plist' ? '<div class="dropdown-divider"></div><a class="dropdown-item" href="#">Add to playlist</a>' : '') +
            ( type != 'dir' ? '<div class="dropdown-divider"></div>' : '') +
            ( type == 'song' ? '<a class="dropdown-item" href="#">Songdetails</a>' : '') +
            ( type == 'plist' ? '<a class="dropdown-item" href="#">Show playlist</a>' : '')
        });
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists') {
        $(el).popover({html:true, content:'<a class="dropdown-item" href="#" onclick="">Append to queue</a>' +
            '<a class="dropdown-item" href="#">Add after current playing song</a>' +
            '<a class="dropdown-item" href="#">Replace queue</a>' +
            '<div class="dropdown-divider"></div>' +
            '<a class="dropdown-item" href="#">Show playlist</a>' +
            '<a class="dropdown-item" href="#">Rename playlist</a>' +
            '<a class="dropdown-item" href="#">Delete playlist</a>'
        });
    }
    else if (app.current.app == 'Queue') {
        $(el).popover({html:true, content:'<a class="dropdown-item" href="#" onclick="">Remove</a>' +
            '<a class="dropdown-item" href="#">Remove all upwards</a>' +
            '<a class="dropdown-item" href="#">Remove all downwards</a>' +
            '<div class="dropdown-divider"></div>' +
            '<a class="dropdown-item" href="#">Songdetails</a>'
        });
    }    
    $(el).popover('show');
}

function updateVolumeIcon(volume) {
    if (volume == -1) {
      $('#volumePrct').text('Volumecontrol disabled');
      $('#volumeControl').addClass('hide');      
    } else {
        $('#volumeControl').removeClass('hidden');
        $('#volumePrct').text(volume+' %');
        if(volume == 0) {
            $("#volume-icon").text("volume_off");
        } else if (volume < 50) {
            $("#volume-icon").text("volume_down");
        } else {
            $("#volume-icon").text("volume_up");
        }
    }
}

function updatePlayIcon(obj) {
    if(obj.data.state == 1) { // stop
        $('#btnPlay > span').text('play_arrow');
        playstate = 'stop';
    } else if(obj.data.state == 2) { // play
        $('#btnPlay > span').text('pause');
        playstate = 'play';
    } else { // pause
        $('#btnPlay > span').text('play_arrow');
	playstate = 'pause';
    }

    if (obj.data.nextsongpos == -1) {
        $('#btnNext').addClass('disabled').attr('disabled','disabled');
    } else {
        $('#btnNext').removeClass('disabled').removeAttr('disabled');
    }
    
    if (obj.data.songpos <= 0) {
        $('#btnPrev').addClass('disabled').attr('disabled','disabled');
    } else {
        $('#btnPrev').removeClass('disabled').removeAttr('disabled');
    }
    
    if (obj.data.queue_length == 0) {
        $('#btnPlay').addClass('disabled').attr('disabled','disabled');
    } else {
        $('#btnPlay').removeClass('disabled').removeAttr('disabled');
    }    
}

function sendAPI(request, callback) {
    $.ajax({url: "/api", contentType:"application/json", method: "POST", data: JSON.stringify(request), success: callback });
}

function updateDB(event) {
    sendAPI({"cmd":"MPD_API_UPDATE_DB"});
    showNotification('Updating MPD Database...','','','success');
    event.preventDefault();
}

function clickPlay() {
    if( playstate != 'play')
        sendAPI({"cmd":"MPD_API_SET_PLAY"});
    else
        sendAPI({"cmd":"MPD_API_SET_PAUSE"});
}

function clickStop() {
    sendAPI({"cmd":"MPD_API_SET_STOP"});
}

function clickPrev() {
    sendAPI({"cmd":"MPD_API_SET_PREV"});
}

function clickNext() {
    sendAPI({"cmd":"MPD_API_SET_NEXT"});
}

function setLocalStream(mpdhost,streamport) {
    var mpdstream = 'http://';
    if ( mpdhost == '127.0.0.1' || mpdhost == 'localhost')
        mpdstream += window.location.hostname;
    else
        mpdstream += mpdhost;
    mpdstream += ':'+streamport+'/';
    settings.mpdstream=mpdstream;
}

function delQueueSong(tr,event) {
    event.stopPropagation();
    if ( $('#btntrashmodeup').hasClass('active') ) {
        sendAPI({"cmd":"MPD_API_RM_RANGE", "data": {"start":0, "end": (tr.index() + 1)}});
    } else if ( $('#btntrashmodesingle').hasClass('active') ) {
        sendAPI({"cmd":"MPD_API_RM_TRACK", "data": { "track": tr.attr('trackid')}});
    } else if ( $('#btntrashmodedown').hasClass('active') ) {
        sendAPI({"cmd":"MPD_API_RM_RANGE", "data": {"start": tr.index(), "end":-1}});
    };
}

function delPlaylist(tr) {
    sendAPI({"cmd":"MPD_API_RM_PLAYLIST","data": {"plist": decodeURI(tr.attr("data-uri"))}});
    tr.remove();
}

function confirmSettings() {
    var formOK=true;
    if (!$('#inputCrossfade').is(':disabled')) {
      var value=parseInt($('#inputCrossfade').val());
      if (!isNaN(value)) {
        $('#inputCrossfade').val(value);
      } else {
        document.getElementById('inputCrossfade').classList.add('is-invalid');
        formOK=false;
      }
    }
    if (!$('#inputMixrampdb').is(':disabled')) {
      var value=parseFloat($('#inputMixrampdb').val());
      if (!isNaN(value)) {
        $('#inputMixrampdb').val(value);
      } else {
        document.getElementById('inputMixrampdb').classList.add('is-invalid');
        formOK=false;
      } 
    }
    if (!$('#inputMixrampdelay').is(':disabled')) {
      if ($('#inputMixrampdelay').val() == 'nan') $('#inputMixrampdelay').val('-1');
      var value=parseFloat($('#inputMixrampdelay').val());
      if (!isNaN(value)) {
        $('#inputMixrampdelay').val(value);
      } else {
        document.getElementById('inputMixrampdelay').classList.add('is-invalid');
        formOK=false;
      }
    }
    if (formOK == true) {
      sendAPI({"cmd":"MPD_API_SET_SETTINGS", "data": {
        "consume": ($('#btnconsume').hasClass('active') ? 1 : 0),
        "random":  ($('#btnrandom').hasClass('active') ? 1 : 0),
        "single":  ($('#btnsingle').hasClass('active') ? 1 : 0),
        "repeat":  ($('#btnrepeat').hasClass('active') ? 1 : 0),
        "replaygain": $('#selectReplaygain').val(),
        "crossfade": $('#inputCrossfade').val(),
        "mixrampdb": $('#inputMixrampdb').val(),
        "mixrampdelay": $('#inputMixrampdelay').val(),
        "notificationWeb": ($('#btnnotifyWeb').hasClass('active') ? 1 : 0),
        "notificationPage": ($('#btnnotifyPage').hasClass('active') ? 1 : 0)
      }},getSettings);
      $('#settings').modal('hide');
    } else {
      document.getElementById('settingsFrm').classList.add('was-validated');
    }
}

function toggleoutput(button, id) {
    sendAPI({"cmd":"MPD_API_TOGGLE_OUTPUT", "data": {"output": id, "state": (button.classList.contains('active') ? 0 : 1)}});
}


$('#search > input').keypress(function (event) {
   if ( event.which == 13 ) {
     $('#mainMenu > a').dropdown('toggle');
   }
});

$('#search').submit(function () {
    app.goto('Search',undefined,undefined,app.current.page + '/Any Tag/' + $('#search > input').val());
    return false;
});

$('#search2').submit(function () {
    return false;
});

function addAllFromSearch() {
    if (app.current.search.length >= 2) {
      sendAPI({"cmd":"MPD_API_SEARCH_ADD","data":{"filter": app.current.filter,"searchstr": + app.current.search}});
      var rowCount = parseInt(document.getElementById('panel-heading-search').innerText);
      showNotification('Added '+rowCount+' songs from search','','','success');
    }
}

$('#searchstr2').keyup(function (event) {
    app.goto('Search', undefined, undefined, app.current.page + '0/' + app.current.filter + '/' + this.value);
});

$('#searchtags2 > button').on('click',function (e) {
    app.goto(app.current.app, app.current.tab, app.current.view, '0/' + this.innerText + '/' + app.current.search);
});

$('#searchqueuestr').keyup(function (event) {
    app.goto(app.current.app, app.current.tab, app.current.view, '0/' + app.current.filter + '/' + this.value);
});

$('#searchqueuetag > button').on('click',function (e) {
    app.goto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + this.innerText + '/' + app.current.search);
});

$('#searchqueue').submit(function () {
    return false;
});

function scrollToTop() {
    document.body.scrollTop = 0; // For Safari
    document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera
}

function gotoPage(x, element, event) {
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
    app.goto(app.current.app, app.current.tab, app.current.view, app.current.page + '/' + app.current.filter + '/' + app.current.search);
    event.preventDefault();
}

function addStream() {
    if($('#streamurl').val().length > 0) {
        sendAPI({"cmd":"MPD_API_ADD_TRACK","data":{"uri":$('#streamurl').val()}});
    }
    $('#streamurl').val("");
    $('#addstream').modal('hide');
}

function saveQueue() {
    if($('#playlistname').val().length > 0) {
        sendAPI({"cmd":"MPD_API_SAVE_QUEUE","data":{"plist":$('#playlistname').val()}});
    }
    $('#savequeue').modal('hide');
}

function showNotification(notificationTitle,notificationText,notificationHtml,notificationType) {
    if (settings.notificationWeb == 1) {
      var notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
      setTimeout(function(notification) {
        notification.close();
      }, 3000, notification);    
    } 
    if (settings.notificationPage == 1) {
      $.notify({ title: notificationTitle, message: notificationHtml},{ type: notificationType, offset: { y:60, x:20 },
        template: '<div data-notify="container" class="alert alert-{0}" role="alert">' +
		'<span data-notify="title">{1}</span> ' +
		'<span data-notify="message">{2}</span>' +
		'</div>' 
      });
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

    document.getElementById('album-cover').style.backgroundImage = 'url("'+obj.data.cover+'")';

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
    showNotification(obj.data.title,textNotification,htmlNotification,'success');
    last_song = cur_song;
}

        
$(document).keydown(function(e){
    if (e.target.tagName == 'INPUT') {
        return;
    }
    switch (e.which) {
        case 37: //left
            sendAPI({"cmd":"MPD_API_SET_PREV"});
            break;
        case 39: //right
            sendAPI({"cmd":"MPD_API_SET_NEXT"});
            break;
        case 32: //space
            clickPlay();
            break;
        default:
            return;
    }
    e.preventDefault();
});

function setFilterLetter(filter) {
  app.goto(app.current.app,app.current.tab,app.current.view, '0/'+filter+'/'+app.current.search);
}

function doSetFilterLetter(x) {
    $(x+'Letters > button').removeClass('active');
    if (app.current.filter == '0') {
        $(x).text('Filter: #');
        $(x+'Letters > button').each(function() {
            if ($(this).text() == '#') {
                $(this).addClass('active');
            }
        });
    } else if (app.current.filter != '-') {
        $(x).text('Filter: '+app.current.filter);
        $(x+'Letters > button').each(function() {
            if ($(this).text() == app.current.filter) {
                $(this).addClass('active');
            }
        });
    } else {
        $(x).text('Filter');
    }
}

function add_filter (x) {
    $(x).append('<button class="mr-1 mb-1 btn btn-sm btn-secondary" onclick="setFilterLetter(\'-\');">'+
        '<span class="material-icons" style="font-size:14px;">delete</span></button>');
    $(x).append('<button class="mr-1 mb-1 btn btn-sm btn-secondary" onclick="setFilterLetter(\'0\');">#</button>');
    for (i = 65; i <= 90; i++) {
        var c = String.fromCharCode(i);
        $(x).append('<button class="mr-1 mb-1 btn-sm btn btn-secondary" onclick="setFilterLetter(\'' + c + '\');">' + c + '</button>');
    }
}

function chVolume (increment) {
 var aktValue=volumeBar.slider('getValue');
 var newValue=aktValue+increment;
 if (newValue<0) { newValue=0; }
 else if (newValue > 100) { newValue=100; }
 volumeBar.slider('setValue',newValue);
 sendAPI({"cmd":"MPD_API_SET_VOLUME", "data": {"volume":newValue}});
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