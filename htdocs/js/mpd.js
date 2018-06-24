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
var isTouch = Modernizr.touch ? 1 : 0;
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
                                    "views":   { "Artist": { "state": "0/-/" },
                                                 "Album":  { "state": "0/-/" }
                                               }
                                           }
                           }
                         },
             "Search": 	 { "state": "0/Any Tag/" }
           };
           
app.current = { "app": "Playback", "tab": undefined, "view": undefined, "page": 0, "filter": "", "search": "" };
app.last = { "app": undefined, "tab": undefined, "view": undefined };

app.prepare=function() {
  if (app.current.app != app.last.app || app.current.tab != app.last.tab || app.current.view != app.last.view) {
    //Hide all cards + nav
    $('#navbar-bottom > div').removeClass('active');
    $('#cardPlayback').addClass('hide');
    $('#cardQueue').addClass('hide');
    $('#cardBrowse').addClass('hide');
    $('#cardSearch').addClass('hide');
    $('#panel-heading-browse > ul > li > a').removeClass('active');
    $('#cardBrowsePlaylists').addClass('hide');
    $('#cardBrowseDatabase').addClass('hide');
    $('#cardBrowseFilesystem').addClass('hide');
    //show active card + nav
    $('#card'+app.current.app).removeClass('hide');
    $('#nav'+app.current.app).addClass('active');
    if (app.current.tab != undefined) {
      $('#card'+app.current.app+app.current.tab).removeClass('hide');
      $('#card'+app.current.app+'Nav'+app.current.tab).addClass('active');    
    }
  }
}

app.goto=function(a,t,v,s) {
   var hash='';
   if (app.apps[a].tabs) {
     if (t == undefined) t = app.apps[a].active;
     if (app.apps[a].tabs[t].views) {
       if (v == undefined) v = app.apps[a].tabs[t].active;
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
    var hash=decodeURI(location.hash);
    if (params=hash.match(/^\#\/(\w+)\/?(\w+)?\/?(\w+)?\!((\d+)\/([^\/]+)\/(.*))$/))
    {
      app.current.app = params[1];
      app.current.tab = params[2];
      app.current.view = params[3];
      if (app.apps[app.current.app].state) {
          app.apps[app.current.app].state = params[4];
      }
      else if (app.apps[app.current.app].tabs[app.current.tab].state) {
          app.apps[app.current.app].tabs[app.current.tab].state = params[4];
           app.apps[app.current.app].active=app.current.tab;
      }
      else if (app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state) {
          app.apps[app.current.app].tabs[app.current.tab].views[app.current.view].state = params[4];
          app.apps[app.current.app].active=app.current.tab;
          app.apps[app.current.app].tabs[app.current.tab].active=app.current.view;
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
        sendAPI({"cmd":"MPD_API_GET_CURRENT_SONG"},songChange);
    }    
    else if (app.current.app == 'Queue' ) {
        if (app.last.app != app.current.app) {
          if (app.current.search.length < 2) {
            setPagination(app.current.page);        
          }
          $('#searchqueuetag > button').each(function() {
            $(this).removeClass('active');
            if ($(this).text() == app.current.filter) { 
                $(this).addClass('active');
                $('#searchqueuetagdesc').text($(this).text());
            }
          }); 
        }
        getQueue();
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Playlists') {
        sendAPI({"cmd":"MPD_API_GET_PLAYLISTS","data": {"offset": app.current.page, "filter": app.current.filter}},parsePlaylists);
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Artist') {
        sendAPI({"cmd":"MPD_API_GET_ARTISTS","data": {"offset": app.current.page, "filter": app.current.filter}},parseListDBtags);
    }
    else if (app.current.app == 'Browse' && app.current.tab == 'Database' && app.current.view == 'Album') {
        sendAPI({"cmd":"MPD_API_GET_ARTISTALBUMS","data": {"offset": app.current.page, "filter": app.current.filter, "albumartist": app.current.search}},parseListDBtags);
    }    
    else if (app.current.app == 'Browse' && app.current.tab == 'Filesystem') {
        $('#BrowseBreadcrumb').empty().append('<li class="breadcrumb-item"><a uri="">root</a></li>');
        sendAPI({"cmd":"MPD_API_GET_FILESYSTEM","data": {"offset": app.current.page,"path":(app.current.search ? app.current.search : "/"),"filter": app.current.filter}},parseFilesystem);
        // Don't add all songs from root
        var add_all_songs = $('#browseFilesystemAddAllSongs');
        if (app.current.search) {
            add_all_songs.off(); // remove previous binds
            add_all_songs.on('click', function() {
                sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": { "uri": app.current.search}});
            });
            add_all_songs.removeAttr('disabled').removeClass('disabled');
        } else {
            add_all_songs.attr('disabled','disabled').addClass('disabled');
        }

        var path_array = app.current.search.split('/');
        var full_path = "";
        $.each(path_array, function(index, chunk) {
            if(path_array.length - 1 == index) {
                $('#BrowseBreadcrumb').append("<li class=\"breadcrumb-item active\">"+ chunk + "</li>");
                return;
            }
            full_path = full_path + chunk;
            $('#BrowseBreadcrumb').append("<li class=\"breadcrumb-item\"><a uri=\"" + full_path + "\">"+chunk+"</a></li>");
            full_path += "/";
        });
    }
    else if (app.current.app == 'Search') {
        if (app.last.app != app.current.app) {
          if (app.current.search != '') {
            $('#SearchList > tbody').append(
                "<tr><td><span class=\"material-icons\">search</span></td>" +
                "<td colspan=\"3\">Searching</td>" +
                "<td></td><td></td></tr>");
          }
          else {
            setPagination(app.current.page);        
          }
          $('#searchstr2').val(app.current.search);
        }
        $('#searchtags2 > button').each(function() {
              $(this).removeClass('active');
          if ($(this).text() == app.current.filter) { 
              $(this).addClass('active'); 
              $('#searchtags2desc').text($(this).text());
          }
        });
        if (app.current.search.length >= 2) {
           sendAPI({"cmd":"MPD_API_SEARCH", "data": { "mpdtag": app.current.filter,"offset":app.current.page,"searchstr":app.current.search}},parseSearch);
        } else {
          $('#SearchList > tbody').empty();
          $('#searchAddAllSongs').attr('disabled','disabled').addClass('disabled');  
        }
    }
    else {
        app.goto("Playback");
    }

    app.last.app=app.current.app;
    app.last.tab=app.current.tab;
    app.last.view=app.current.view;
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
    
    window.addEventListener("hashchange", app.route, false);
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
  $('#mpdstats_artists').text(obj.data.artists);
  $('#mpdstats_albums').text(obj.data.albums);
  $('#mpdstats_songs').text(obj.data.songs);
  $('#mpdstats_dbplaytime').text(beautifyDuration(obj.data.dbplaytime));
  $('#mpdstats_playtime').text(beautifyDuration(obj.data.playtime));
  $('#mpdstats_uptime').text(beautifyDuration(obj.data.uptime));
  var d = new Date(obj.data.dbupdated * 1000);
  $('#mpdstats_dbupdated').text(d.toUTCString());
  $('#mympdVersion').text(obj.data.mympd_version);
  $('#mpdVersion').text(obj.data.mpd_version);
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
  sendAPI({"cmd":"MPD_API_GET_SETTINGS"},parseSettings);
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

                    volumeBar.slider('setValue',obj.data.volume);
                    var progress = Math.floor(100*obj.data.elapsedTime/obj.data.totalTime);
                    progressBar.slider('setValue',progress);

                    var counterText=elapsed_minutes + ":" + 
                        (elapsed_seconds < 10 ? '0' : '') + elapsed_seconds + " / " +
                        total_minutes + ":" + (total_seconds < 10 ? '0' : '') + total_seconds;
                        
                    $('#counter').text(counterText);

                    if (last_state) {
                      $('#QueueList > tbody > tr[trackid='+last_state.data.currentsongid+'] > td').eq(4).text(last_state.data.totalTime);
                      $('#QueueList > tbody > tr[trackid='+last_state.data.currentsongid+'] > td').eq(0).removeClass('material-icons').text(last_state.data.songpos);
                    }
                    $('#QueueList > tbody > tr').removeClass('active').removeClass("font-weight-bold");
                        
                    $('#QueueList > tbody > tr[trackid='+obj.data.currentsongid+'] > td').eq(4).text(counterText);
                    $('#QueueList > tbody > tr[trackid='+obj.data.currentsongid+'] > td').eq(0).addClass('material-icons').text('play_arrow');
                    $('#QueueList > tbody > tr[trackid='+obj.data.currentsongid+']').addClass('active').addClass("font-weight-bold");
                    
                    //Get current song on queue change for http streams
                    if (last_state == undefined || obj.data.queue_version != last_state.data.queue_version) {
                        sendAPI({"cmd":"MPD_API_GET_CURRENT_SONG"},songChange);
                    }
                    
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
          sendAPI({"cmd":"MPD_API_SEARCH_QUEUE", "data": {"mpdtag":app.current.filter, "offset":app.current.page,"searchstr": app.current.search}},parseQueue);
        else
          sendAPI({"cmd":"MPD_API_GET_QUEUE", "data": {"offset": app.current.page}},parseQueue);
}

function parseQueue(obj) {
                    if(app.current.app !== 'Queue')
                        return;
                    $('#panel-heading-queue').empty();
                    if (obj.totalEntities > 0) {
                        $('#panel-heading-queue').text(obj.totalEntities+' Songs');
                    }
                    if (typeof(obj.totalTime) != undefined && obj.totalTime > 0 ) {
                        $('#panel-heading-queue').append(' – ' + beautifyDuration(obj.totalTime));
                    }

                    var nrItems=0;
                    var tr=document.getElementById(app.current.app+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
                    for (var song in obj.data) {
                        nrItems++;
                        var minutes = Math.floor(obj.data[song].duration / 60);
                        var seconds = obj.data[song].duration - minutes * 60;
                        
                        var row="<tr trackid=\"" + obj.data[song].id + "\"><td>" + (obj.data[song].pos + 1) + "</td>" +
                                "<td>"+ obj.data[song].title +"</td>" +
                                "<td>"+ obj.data[song].artist +"</td>" + 
                                "<td>"+ obj.data[song].album +"</td>" +
                                "<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
                        "</td><td></td></tr>";
                        if (nrItems <= tr.length) { if ($(tr[nrItems-1]).attr('trackid')!=obj.data[song].id) $(tr[nrItems-1]).replaceWith(row); } 
                        else { $('#'+app.current.app+'List > tbody').append(row); }
                    }
                    for (var i=tr.length;i>nrItems;i--) {
                         $(tr[tr.length-1]).remove();
                    }
                    
                    if (obj.type == 'queuesearch' && nrItems == 0) {
                        $('#QueueList > tbody').append(
                               "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                               "<td colspan=\"3\">No results, please refine your search!</td>" +
                               "<td></td><td></td></tr>"
                        );
                    }
                    setPagination(obj.totalEntities);

                    if ( isTouch ) {
                        $('#QueueList > tbody > tr > td:last-child').append(
                                '<a class="pull-right btn-group-hover color-darkgrey" href="#/Queue!' + app.current.page + '/'+app.current.filter+'/'+app.current.search + '" '+
                                    'onclick="delQueueSong($(this).parents(\'tr\'),event);">' +
                                '<span class="material-icons">delete</span></a>');
                    } else {
                        $('#QueueList > tbody > tr').on({
                            mouseover: function(){
                                var doomed = $(this);
                                if ( $('#btntrashmodeup').hasClass('active') )
                                    doomed = $('#QueueList > tbody > tr:lt(' + ($(this).index() + 1) + ')');
                                if ( $('#btntrashmodedown').hasClass('active') )
                                    doomed = $('#QueueList > tbody > tr:gt(' + ($(this).index() - 1) + ')');
                                $.each(doomed, function(){
                                if($(this).children().last().has('a').length == 0)
                                    $(this).children().last().append(
                                        '<a class="pull-right btn-group-hover color-darkgrey" href="#/Queue!'+ app.current.page + '/' +app.current.filter+'/'+app.current.search + '" ' +
                                            'onclick="delQueueSong($(this).parents(\'tr\'),event);">' +
                                        '<span class="material-icons">delete</span></a>')
                                .find('a').fadeTo('fast',1);
                                });
                            },
                            mouseleave: function(){
                                var doomed = $(this);
                                if ( $('#btntrashmodeup').hasClass('active') )
                                    doomed = $("#QueueList > tbody > tr:lt(" + ($(this).index() + 1) + ")");
                                if ( $('#btntrashmodedown').hasClass('active') )
                                    doomed = $("#QueueList > tbody > tr:gt(" + ($(this).index() - 1) + ")");
                                $.each(doomed, function(){$(this).children().last().find("a").stop().remove();});
                            }
                        });
                    };

                    $('#QueueList > tbody > tr').on({
                        click: function() {
                            $('#queueList > tbody > tr').removeClass('active');
                            sendAPI({"cmd":"MPD_API_PLAY_TRACK","data": {"track":$(this).attr('trackid')}});
                            $(this).addClass('active');
                        },
                    });
}

function parseSearch(obj) {
    if(app.current.app !== 'Search')
                        return;
                    $('#panel-heading-search').text(obj.totalEntities + ' Songs found');
                    if (obj.totalEntities > 0) {
                        $('#searchAddAllSongs').removeAttr('disabled').removeClass('disabled');
                    } else {
                        $('#searchAddAllSongs').attr('disabled','disabled').addClass('disabled');                    
                    }
    parseFilesystem(obj);
}

function parseFilesystem(obj) {
                    if(app.current.app !== 'Browse' && app.current.tab !== 'Filesystem' && app.current.app !== 'Search')
                        return;
                    
                    /* The use of encodeURI() below might seem useless, but it's not. It prevents
                     * some browsers, such as Safari, from changing the normalization form of the
                     * URI from NFD to NFC, breaking our link with MPD.
                     */
                    var nrItems=0;
                    var tr=document.getElementById(app.current.app+(app.current.tab==undefined ? '' : app.current.tab)+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
                    for (var item in obj.data) {
                        nrItems++;
                        var row='';
                        var uri='';
                        switch(obj.data[item].type) {
                            case 'directory':
                                uri=encodeURI(obj.data[item].dir);
                                row ='<tr uri="' + uri + '" class="dir">' +
                                    '<td><span class="material-icons">folder_open</span></td>' +
                                    '<td colspan="3"><a>' + basename(obj.data[item].dir) + '</a></td>' +
                                    '<td></td><td></td></tr>';
                                break;
                            case 'song':
                                var minutes = Math.floor(obj.data[item].duration / 60);
                                var seconds = obj.data[item].duration - minutes * 60;
                                uri=encodeURI(obj.data[item].uri);
                                row ='<tr uri="' + uri  + '" class="song">' +
                                    '<td><span class="material-icons">music_note</span></td>' + 
                                    '<td>' + obj.data[item].title  + '</td>' +
                                    '<td>' + obj.data[item].artist + '</td>' + 
                                    '<td>' + obj.data[item].album  + '</td>' +
                                    '<td>' + minutes + ':' + (seconds < 10 ? '0' : '') + seconds +
                                    '</td><td></td></tr>';
                                break;
                            case 'playlist':
                                uri=encodeURI(obj.data[item].plist);
                                row ='<tr uri="' + uri + '" class="plist">' +
                                    '<td><span class="material-icons">list</span></td>' +
                                    '<td colspan="3"><a>' + basename(obj.data[item].plist) + '</a></td>' +
                                    '<td></td><td></td></tr>';
                                break;
                        }
                        if (nrItems <= tr.length) { if ($(tr[nrItems-1]).attr('uri') != uri) $(tr[nrItems-1]).replaceWith(row); } 
                        else { $('#'+app.current.app+(app.current.tab==undefined ? '' : app.current.tab)+'List > tbody').append(row); }
                    }
                    for (var i=tr.length;i>nrItems;i--) {
                        $(tr[tr.length-1]).remove();
                    }
                    setPagination(obj.totalEntities);
                    
                    if (nrItems == 0) {
                       $('#'+app.current.app+app.current.tab+'List > tbody').append(
                           '<tr><td><span class="material-icons">error_outline</span></td>' +
                           '<td colspan="3">No results</td>' +
                           '<td></td><td></td></tr>');
                    }

                    function appendClickableIcon(appendTo, onClickAction, glyphicon) {
                        $(appendTo).html(
                            '<a role="button" class="pull-right btn-group-hover">' +
                            '<span class="material-icons">' + glyphicon + '</span></a>')
                            .find('a').click(function(e) {
                                e.stopPropagation();
                                sendAPI({"cmd":onClickAction,"data":{ "uri":decodeURI($(this).parents("tr").attr("uri"))}});
                                showNotification('"' + $('td:nth-last-child(3)', $(this).parents('tr')).text() + '" added','','','success');
                            });
                    }

                    if ( isTouch ) {
                        appendClickableIcon($('#'+app.current.app+(app.current.tab == undefined ? '' : app.current.tab )+'List > tbody > tr.dir > td:last-child'), 'MPD_API_ADD_TRACK', 'playlist_add');
                        appendClickableIcon($('#'+app.current.app+(app.current.tab == undefined ? '' : app.current.tab )+'List > tbody > tr.song > td:last-child'), 'MPD_API_ADD_TRACK', 'playlist_add');
                    } else {
                        $('#'+app.current.app+(app.current.tab == undefined ? '' : app.current.tab )+'List > tbody > tr').on({
                            mouseenter: function() {
                                if($(this).is(".dir")) 
                                    appendClickableIcon($(this).children().last(), 'MPD_API_ADD_TRACK', 'playlist_add');
                                else if($(this).is(".song"))
                                    appendClickableIcon($(this).children().last(), 'MPD_API_ADD_TRACK', 'playlist_add');
                            },
                            mouseleave: function(){
                                $(this).children().last().find("a").stop().remove();
                            }
                        });
                    };
                    $('#'+app.current.app+(app.current.tab == undefined ? '' : app.current.tab )+'List > tbody > tr').on({
                        click: function() {
                            switch($(this).attr('class')) {
                                case 'dir':
                                    app.current.page = 0;
                                    app.current.search = $(this).attr("uri");
                                    $("#BrowseFilesystemList > a").attr("href", '#/Browse/Filesystem!'+app.current.page+'/'+app.current.filter+'/'+app.current.search);
                                    app.goto('Browse','Filesystem',undefined,app.current.page+'/'+app.current.filter+'/'+app.current.search);
                                    break;
                                case 'song':
                                    sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": {"uri": decodeURI($(this).attr("uri"))}});
                                    showNotification('"' + $('td:nth-last-child(5)', this).text() + '" added','','','success');
                                    break;
                                case 'plist':
                                    sendAPI({"cmd":"MPD_API_ADD_PLAYLIST", "data": {"plist": decodeURI($(this).attr("uri"))}});
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                                    break;
                            }
                        }
                    });

                    $('#BrowseBreadcrumb > li > a').on({
			click: function() {
		        	app.current.page = 0;
				app.current.search = $(this).attr("uri");
				$("#BrowseFilesystemList > a").attr("href", '#/Browse/Filesystem!'+app.current.page+'/'+app.current.filter+'/'+app.current.search);
				app.goto('Browse','Filesystem',undefined,app.current.page+'/'+app.current.filter+'/'+app.current.search);
			}
                    });
    doSetFilterLetter('#BrowseFilesystemFilter');
}

function parsePlaylists(obj) {
                    if(app.current.app !== 'Browse' && app.current.tab !== 'Playlists')
                        return;
                    var nrItems=0;
                    var tr=document.getElementById(app.current.app+app.current.tab+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
                    for (var item in obj.data) {
                        nrItems++;
                        var d = new Date(obj.data[item].last_modified * 1000);
                        var uri=encodeURI(obj.data[item].plist);
                        var row='<tr uri="' + uri + '">' +
                                '<td><span class="material-icons">list</span></td>' +
                                '<td><a>' + basename(obj.data[item].plist) + '</a></td>' +
                                '<td>'+d.toUTCString()+'</td><td></td></tr>';
                        if (nrItems <= tr.length) { if ($(tr[nrItems-1]).attr('uri') != uri) $(tr[nrItems-1]).replaceWith(row); } 
                        else { $('#'+app.current.app+app.current.tab+'List > tbody').append(row); }
                    }
                    for (var i=tr.length;i>nrItems;i--) {
                         $(tr[tr.length-1]).remove();
                    }
                    setPagination(obj.totalEntities);
                    if ( isTouch ) {
                        $('#'+app.current.app+app.current.tab+'List > tbody > tr > td:last-child').append(
                                '<a class="pull-right btn-group-hover color-darkgrey" href="#/Browse/Playlists!' + app.current.page + '/'+app.current.filter+'/'+app.current.search+'" '+
                                'onclick="delPlaylist($(this).parents(\'tr\'));">' +
                                '<span class="material-icons">delete</span></a>');
                    } else {
                        $('#'+app.current.app+app.current.tab+'List > tbody > tr').on({
                            mouseover: function(){
                                if($(this).children().last().has('a').length == 0)
                                    $(this).children().last().append(
                                        '<a class="pull-right btn-group-hover color-darkgrey" href="#/Browse/Playlists!' + app.current.page + '/'+app.current.filter+'/'+app.current.search+'" '+
                                        'onclick="delPlaylist($(this).parents(\'tr\'));">' +
                                        '<span class="material-icons">delete</span></a>');
                            },
                            mouseleave: function(){
                                var doomed = $(this);
                                $(this).children().last().find("a").stop().remove();
                            }
                        });
                    };
                    $('#'+app.current.app+app.current.tab+'List > tbody > tr').on({
                        click: function() {
                                    sendAPI({"cmd":"MPD_API_ADD_PLAYLIST", "data": { "plist": decodeURI($(this).attr('uri'))}});
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                        }
                    });
                    if (nrItems == 0) {
                        $('#'+app.current.app+app.current.tab+'List > tbody').append(
                               '<tr><td><span class="material-icons">error_outline</span></td>' +
                               '<td colspan="3">No playlists found.</td>' +
                               '<td></td><td></td></tr>'
                        );
                    }
  doSetFilterLetter('#browsePlaylistsFilter');
}

function parseListDBtags(obj) {
  if(app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Artist') return;
  
                    if (obj.tagtype == 'AlbumArtist') {
                        $('#BrowseDatabaseAlbumCards').addClass('hide');
                        $('#BrowseDatabaseArtistList').removeClass('hide');
                        $('#btnBrowseDatabaseArtist').addClass('hide');
                        var nrItems=0;
                        var tr=document.getElementById(app.current.app+app.current.tab+app.current.view+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
                        for (var item in obj.data) {
                            nrItems++;
                            var uri=encodeURI(obj.data[item].value);
                            var row='<tr uri="' + uri  + '">' +
                                '<td><span class="material-icons">album</span></td>' +
                                '<td><a>' + obj.data[item].value + '</a></td></tr>';
                            if (nrItems <= tr.length) { if ($(tr[nrItems-1]).attr('uri')!=uri) $(tr[nrItems-1]).replaceWith(row); } 
                            else { $('#'+app.current.app+app.current.tab+app.current.view+'List > tbody').append(row); }
                        
                        }
                        for (var i=tr.length;i>nrItems;i--) {
                            $(tr[tr.length-1]).remove();
                        }
                        setPagination(obj.totalEntities);
                        $('#'+app.current.app+app.current.tab+app.current.view+'List > tbody > tr').on({
                            click: function() {
                                app.goto('Browse','Database','Album','0/-/'+$(this).attr('uri'));
                            }
                        });
                        if (nrItems == 0) {
                            $('#'+app.current.app+app.current.tab+app.current.view+'List > tbody').append(
                               '<tr><td><span class="material-icons">error_outline</span></td>' +
                               '<td colspan="3">No entries found.</td>' +
                               '<td></td><td></td></tr>'
                            );
                        }
                    } else if (obj.tagtype == 'Album') {
                        $('#BrowseDatabaseArtistList').addClass('hide');
                        $('#BrowseDatabaseAlbumCards').removeClass('hide');
                        $('#btnBrowseDatabaseArtist').removeClass('hide');
                        var nrItems=0;
                        var cards=document.getElementById('BrowseDatabaseAlbumCards').querySelectorAll('.col-md');
                        for (var item in obj.data) {
                          nrItems++;
                          var id=genId(obj.data[item].value);
                          var card='<div class="col-md mr-0" id="'+id+'"><div class="card mb-4" id="card'+id+'">'+
                                   ' <img class="card-img-top" src="" alt="">'+
                                   ' <div class="card-body">'+
                                   '  <h5 class="card-title">'+obj.searchstr+'</h5>'+
                                   '  <h4 class="card-title">'+obj.data[item].value+'</h4>'+
                                   '  <table class="table table-sm table-hover" id="tbl'+id+'"><tbody></tbody></table'+
                                   ' </div>'+
                                   '</div></div>';
                          if (nrItems <= cards.length) { if (cards[nrItems-1].id != id) $(cards[nrItems-1]).replaceWith(card); } 
                          else { $('#BrowseDatabaseAlbumCards').append(card); }
                          if (nrItems > cards.length || cards[nrItems-1].id != id)
                            sendAPI({"cmd":"MPD_API_GET_ARTISTALBUMTITLES", "data": { "albumartist": obj.searchstr, "album": obj.data[item].value}},parseListTitles);
                        }
                        for (var i=cards.length;i>nrItems;i--) {
                            $(cards[i-1]).remove();
                        }
                        setPagination(obj.totalEntities);
                    }
   doSetFilterLetter('#BrowseDatabaseFilter');
}

function parseListTitles(obj) {
  if(app.current.app !== 'Browse' && app.current.tab !== 'Database' && app.current.view !== 'Album') return;
                    var id=genId(obj.album);
                    var album=$('#card'+id+' > div > table > tbody');
                    $('#card'+id+' > img').attr('src',obj.cover)
                        .attr('uri',obj.data[0].uri.replace(/\/[^\/]+$/,''))
                        .attr('data-album',encodeURI(obj.album));
                    var titleList='';
                    for (var item in obj.data) {
                        titleList+='<tr uri="' + encodeURI(obj.data[item].uri) + '" class="song">'+
                            '<td>'+obj.data[item].track+'</td><td>'+obj.data[item].title+'</td></tr>';
                    }
                    album.append(titleList);
                    $('#card'+id+' > img').on({
                        click: function() {
                                    sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": { "track": decodeURI($(this).attr('uri'))}});
                                    showNotification('"'+decodeURI($(this).attr('data-album'))+'" added','','','success');
                        }
                    });
                    
                    $('#tbl'+id+' > tbody > tr').on({
                        click: function() {
                                    sendAPI({"cmd":"MPD_API_ADD_TRACK", "data": { "track": decodeURI($(this).attr('uri'))}});
                                    showNotification('"' + $('td:nth-last-child(1)', this).text() + '" added','','','success');
                        }
                    });
}

function setPagination(number) {
    var totalPages=Math.ceil(number / settings.max_elements_per_page);
    var cat=app.current.app+(app.current.tab==undefined ? '': app.current.tab);
    if (totalPages==0) { totalPages=1; }
        $('#'+cat+'PaginationTopPage').text('Page '+(app.current.page / settings.max_elements_per_page + 1)+' / '+totalPages);
        $('#'+cat+'PaginationBottomPage').text('Page '+(app.current.page / settings.max_elements_per_page + 1)+' / '+totalPages);
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
    sendAPI({"cmd":"MPD_API_RM_PLAYLIST","data": {"plist": decodeURI(tr.attr("uri"))}});
    tr.remove();
}

function basename(path) {
    return path.split('/').reverse()[0];
}

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
    sendAPI({"cmd":"MPD_API_TOGGLE_OUTPUT", "data": {"output": id, "state": ($(button).hasClass('active') ? 0 : 1)}});
}

$('#trashmodebtns > button').on('click', function(e) {
    $('#trashmodebtns').children('button').removeClass('active');
    $(this).addClass('active');
});

        

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
      var rowCount = $('#SearchList >tbody >tr').length;
      showNotification('Added '+rowCount+' songs from search','','','success');
    }
}

$('#searchstr2').keyup(function (event) {
  app.current.page=0;
  app.current.search=$(this).val();
  app.goto('Search',undefined,undefined,app.current.page + '/' + app.current.filter + '/' + app.current.search);
});

$('#searchtags2 > button').on('click',function (e) {
  $('#searchtags2 > button').removeClass('active');
  $(this).removeClass('btn-secondary').addClass('active');
  app.current.filter=$(this).text();
  app.goto(app.current.app,app.current.tab,app.current.view,app.current.page + '/' + app.current.filter + '/' + app.current.search);
});

$('#searchqueuestr').keyup(function (event) {
  app.current.page=0;
  app.current.search=$(this).val();
  app.goto(app.current.app,app.current.tab,app.current.view,app.current.page + '/' + app.current.filter + '/' + app.current.search);
});

$('#searchqueuetag > button').on('click',function (e) {
  $('#searchqueuetag > button').removeClass('active');
  $(this).removeClass('btn-secondary').addClass('active');
  app.current.filter=$(this).text();
  $('#searchqueuetagdesc').text(app.current.filter);
  app.goto(app.current.app,app.current.tab,app.current.view,app.current.page + '/' + app.current.filter + '/' + app.current.search);
});

$('#searchqueue').submit(function () {
    return false;
});

$('#searchqueue').submit(function () {
    return false;
});

function scrollToTop() {
    document.body.scrollTop = 0; // For Safari
    document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera
}

function gotoPage(x,element,event) {
    switch (x) {
        case "next":
            app.current.page += settings.max_elements_per_page;
            break;
        case "prev":
            app.current.page -= settings.max_elements_per_page;
            if(app.current.page <= 0)
               app.current.page = 0;
            break;
        default:
            app.current.page = x;
    }
    app.goto(app.current.app,app.current.tab,app.current.view,app.current.page+'/'+app.current.filter+'/'+app.current.search);
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
      $.notify({ title: notificationTitle, message: notificationHtml},{ type: notificationType, offset: { y: 60, x:20 },
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
    if (last_song == obj.data.title+obj.data.artist+obj.data.album+obj.data.uri+obj.data.currentsongid) 
        return;
    var textNotification = '';
    var htmlNotification = '';
    var pageTitle = 'myMPD: ';

    $('#album-cover').css('backgroundImage','url("'+obj.data.cover+'")');

    if(typeof obj.data.artist != 'undefined' && obj.data.artist.length > 0 && obj.data.artist != '-') {
        textNotification += obj.data.artist;
        htmlNotification += '<br/>' + obj.data.artist;
        pageTitle += obj.data.artist + ' - ';
        $('#artist').text(obj.data.artist);
    } else {
        $('#artist').text('');
    }
    if(typeof obj.data.album != 'undefined' && obj.data.album.length > 0 && obj.data.album != '-') {
        textNotification += ' - ' + obj.data.album;
        htmlNotification += '<br/>' + obj.data.album;
        $('#album').text(obj.data.album);
    }
    else {
        $('#album').text('');
    }
    if(typeof obj.data.title != 'undefined' && obj.data.title.length > 0) {
        pageTitle += obj.data.title;
        $('#currenttrack').text(obj.data.title);
    } else {
        $('#currenttrack').text('');
    }
    document.title = pageTitle;
    showNotification(obj.data.title,textNotification,htmlNotification,'success');
    last_song = obj.data.title+obj.data.artist+obj.data.album+obj.data.uri+obj.data.currentsongid;
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