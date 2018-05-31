/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/ympd
   
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
var last_state;
var last_outputs;
var current_app;
var pagination = 0;
var browsepath = "";
var lastSongTitle = "";
var current_song = new Object();
var MAX_ELEMENTS_PER_PAGE = 100;
var isTouch = Modernizr.touch ? 1 : 0;
var playstate = "";
var progressBar;
var volumeBar;
var coverImageFile = "";

var app = $.sammy(function() {

    function prepare() {
        $('#navbar-bottom > div').removeClass('active');
        $('#cardPlayback').addClass('hide');
        $('#cardQueue').addClass('hide');
        $('#cardBrowse').addClass('hide');
        $('#cardSearch').addClass('hide');
        $('#searchqueue > input').val('');
        $('#cardBrowsePlaylists').addClass('hide');
        $('#cardBrowseDatabase').addClass('hide');
        $('#cardBrowseFilesystem').addClass('hide');
        $('#cardBrowseNavPlaylists').removeClass('active');
        $('#cardBrowseNavDatabase').removeClass('active');
        $('#cardBrowseNavFilesystem').removeClass('active');
        pagination = 0;
        browsepath = '';
    }

    this.get (/\#\/playing\//, function() {
        prepare();
        current_app = 'nowplaying';
        $('#cardPlayback').removeClass('hide');
        $('#navPlayback').addClass('active');
    });    

    this.get(/\#\/queue\/(\d+)\/([^\/]+)\/(.*)/, function() {
        current_app = 'queue';
        pagination = parseInt(this.params['splat'][0]);
        var mpdtag = this.params['splat'][1];
        var searchstr = this.params['splat'][2];
        
        if ($('#cardQueue').hasClass('hide')) {
          prepare();
          if (searchstr == '') {
            setPagination(pagination);        
          }
          $('#searchtags2 > button').each(function() {
            if ($(this).text == mpdtag) { $(this).removeClass('btn-secondary').addClass('btn-success'); }
          }); 
          $('#cardQueue').removeClass('hide');
          $('#navQueue').addClass('active');
        }
        if (searchstr.length >= 3) {
          socket.send('MPD_API_SEARCH_QUEUE,' + mpdtag + ','+pagination+',' + searchstr);        
        }
        else {
          socket.send('MPD_API_GET_QUEUE,'+pagination);
        }
    });

    this.get(/\#\/browse\/playlists\/(\d+)/, function() {
        prepare();
        browsepath = this.params['splat'][1];
        pagination = parseInt(this.params['splat'][0]);
        current_app = 'browsePlaylists';
        $('#navBrowse').addClass('active');
        $('#cardBrowse').removeClass('hide');
        $('#cardBrowsePlaylists').removeClass('hide');
        $('#cardBrowseNavPlaylists').addClass('active');
        $('#browsePlaylistsList').find("tr:gt(0)").remove();
        socket.send('MPD_API_GET_PLAYLISTS,'+pagination);
    });
    
    this.get(/\#\/browse\/database\/(\d+)/, function() {
        prepare();
        browsepath = this.params['splat'][1];
        pagination = parseInt(this.params['splat'][0]);
        current_app = 'browseDatabase';
        $('#navBrowse').addClass('active');
        $('#cardBrowse').removeClass('hide');
        $('#cardBrowseDatabase').removeClass('hide');
        $('#cardBrowseNavDatabase').addClass('active');
    });

    this.get(/\#\/browse\/filesystem\/(\d+)\/(.*)/, function() {
        prepare();
        browsepath = this.params['splat'][1];
        pagination = parseInt(this.params['splat'][0]);
        current_app = 'browseFilesystem';
        $('#navBrowse').addClass('active');
        $('#cardBrowse').removeClass('hide');
        $('#cardBrowseFilesystem').removeClass('hide');
        $('#cardBrowseNavFilesystem').addClass('active');
        $('#browseBreadcrumb').empty().append("<li class=\"breadcrumb-item\"><a uri=\"\">root</a></li>");
        socket.send('MPD_API_GET_BROWSE,'+pagination+','+(browsepath ? browsepath : "/"));
        // Don't add all songs from root
        var add_all_songs = $('#add-all-songs');
        if (browsepath) {
            add_all_songs.off(); // remove previous binds
            add_all_songs.on('click', function() {
                socket.send('MPD_API_ADD_TRACK,'+browsepath);
            });
            add_all_songs.removeClass('hide');
        } else {
            add_all_songs.addClass('hide');
        }

        var path_array = browsepath.split('/');
        var full_path = "";
        $.each(path_array, function(index, chunk) {
            if(path_array.length - 1 == index) {
                $('#browseBreadcrumb').append("<li class=\"breadcrumb-item active\">"+ chunk + "</li>");
                return;
            }

            full_path = full_path + chunk;
            $('#browseBreadcrumb').append("<li class=\"breadcrumb-item\"><a uri=\"" + full_path + "\">"+chunk+"</a></li>");
            full_path += "/";
        });
    });

    this.get(/\#\/search\/(\d+)\/([^\/]+)\/(.*)/, function() {
        current_app = 'search';
        pagination = parseInt(this.params['splat'][0]);
        var mpdtag = this.params['splat'][1];
        var searchstr = this.params['splat'][2];
        
        if ($('#cardSearch').hasClass('hide')) {
          prepare();
          if (searchstr != '') {
            $('#searchList > tbody').append(
                "<tr><td><span class=\"material-icons\">search</span></td>" +
                "<td colspan=\"3\">Searching</td>" +
                "<td></td><td></td></tr>");
          }
          else {
            setPagination(pagination);        
          }
          $('#search > input').val(searchstr);
          $('#searchstr2').val(searchstr);
          $('#searchtags2 > button').each(function() {
            if ($(this).text == mpdtag) { $(this).removeClass('btn-secondary').addClass('btn-success'); }
          }); 
          $('#cardSearch').removeClass('hide');
          $('#navSearch').addClass('active');
        }
        if (searchstr.length >= 3) {
          socket.send('MPD_API_SEARCH,' + mpdtag + ','+pagination+',' + searchstr);
        } else {
          $('#searchList > tbody').empty();
        }
    });

    this.get("/", function(context) {
        context.redirect("#/playing/");
    });
    
});

$(document).ready(function(){
    webSocketConnect();

    volumeBar=$('#volumebar').slider();
    volumeBar.slider('setValue',0);
    volumeBar.slider('on','slideStop', function(value){
      socket.send("MPD_API_SET_VOLUME,"+value);
    });

    progressBar=$('#progressbar').slider();
    progressBar.slider('setValue',0);
    
    progressBar.slider('on','slideStop', function(value){
        if(current_song && current_song.currentSongId >= 0) {
            var seekVal = Math.ceil(current_song.totalTime*(value/100));
            socket.send("MPD_API_SET_SEEK,"+current_song.currentSongId+","+seekVal);
        }
    });

    $('#about').on('shown.bs.modal', function () {
        socket.send("MPD_API_GET_STATS");
    })
    
    $('#settings').on('shown.bs.modal', function () {
        socket.send("MPD_API_GET_SETTINGS");
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
     
    if(!notificationsSupported())
        $('#btnnotifyWeb').addClass("disabled");
    else
        if (Cookies.get('notificationWeb') === 'true')
            $('#btnnotifyWeb').removeClass('btn-secondary').addClass("btn-success")
    
    if (Cookies.get('notificationPage') === 'true')
        $('#btnnotifyPage').removeClass('btn-secondary').addClass("btn-success")

    add_filter();
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
            /* emit request for mympd settings */
            socket.send('MPD_API_GET_SETTINGS');            
            /* emit initial request for output names */
            socket.send('MPD_API_GET_OUTPUTS');
            showNotification('Connected to myMPD','','','success');
            $('#modalConnectionError').modal('hide');    
            app.run();
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
                case 'queuesearch':
                //Do the same as queue
                case 'queue':
                    if(current_app !== 'queue')
                        break;
                    $('#panel-heading-queue').empty();
                    if (obj.totalEntities > 0) {
                        $('#panel-heading-queue').text(obj.totalEntities+' Songs');
                    }
                    if (typeof(obj.totalTime) != undefined && obj.totalTime > 0 ) {
                        $('#panel-heading-queue').append(' – ' + beautifyDuration(obj.totalTime));
                    }

                    var nrItems=0;
                    var tr=document.getElementById(current_app+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
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
                        if (nrItems <= tr.length) { $(tr[nrItems-1]).replaceWith(row); } 
                        else { $('#'+current_app+'List > tbody').append(row); }
                    }
                    for (var i=tr.length;i>nrItems;i--) {
                         $(tr[tr.length-1]).remove();
                    }
                    
                    if (obj.type == 'queuesearch' && nrItems == 0) {
                        $('#queueList > tbody').append(
                               "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                               "<td colspan=\"3\">No results, please refine your search!</td>" +
                               "<td></td><td></td></tr>"
                        );
                    }
                    setPagination(obj.totalEntities);

                    if ( isTouch ) {
                        $('#queueList > tbody > tr > td:last-child').append(
                                    '<a class="pull-right btn-group-hover color-darkgrey" href="#/queue/' + pagination + '" '+
                                        'onclick="delQueueSong($(this).parents(\'tr\'));">' +
                                '<span class="material-icons">delete</span></a>');
                    } else {
                        $('#queueList > tbody > tr').on({
                            mouseover: function(){
                                var doomed = $(this);
                                if ( $('#btntrashmodeup').hasClass('btn-success') )
                                    doomed = $('#queueList > tbody > tr:lt(' + ($(this).index() + 1) + ')');
                                if ( $('#btntrashmodedown').hasClass('btn-success') )
                                    doomed = $('#queueList > tbody > tr:gt(' + ($(this).index() - 1) + ')');
                                $.each(doomed, function(){
                                if($(this).children().last().has("a").length == 0)
                                    $(this).children().last().append(
                                        '<a class="pull-right btn-group-hover color-darkgrey" href="#/queue/' + pagination + '" ' +
                                            'onclick="delQueueSong($(this).parents(\'tr\'));">' +
                                        '<span class="material-icons">delete</span></a>')
                                .find('a').fadeTo('fast',1);
                                });
                            },
                            mouseleave: function(){
                                var doomed = $(this);
                                if ( $('#btntrashmodeup').hasClass('btn-success') )
                                    doomed = $("#queueList > tbody > tr:lt(" + ($(this).index() + 1) + ")");
                                if ( $('#btntrashmodedown').hasClass('btn-success') )
                                    doomed = $("#queueList > tbody > tr:gt(" + ($(this).index() - 1) + ")");
                                $.each(doomed, function(){$(this).children().last().find("a").stop().remove();});
                            }
                        });
                    };

                    $('#queueList > tbody > tr').on({
                        click: function() {
                            $('#queueList > tbody > tr').removeClass('active');
                            socket.send('MPD_API_PLAY_TRACK,'+$(this).attr('trackid'));
                            $(this).addClass('active');
                        },
                    });
                    break;
                case 'playlists':
                    if(current_app !== 'browsePlaylists')
                        break;
                    var nrItems=0;
                    for (var item in obj.data) {
                        nrItems++;
                        var d = new Date(obj.data[item].last_modified * 1000);
                        $('#'+current_app+'List > tbody').append(
                                    '<tr uri="' + encodeURI(obj.data[item].plist) + '">' +
                                    '<td><span class="material-icons">list</span></td>' +
                                    '<td><a>' + basename(obj.data[item].plist) + '</a></td>' +
                                    '<td>'+d.toUTCString()+'</td><td></td></tr>'
                        );
                    }
                    setPagination(obj.totalEntities);
                    if ( isTouch ) {
                        $('#'+current_app+'List > tbody > tr > td:last-child').append(
                                '<a class="pull-right btn-group-hover color-darkgrey" href="#/browse/playlists/' + pagination + '" '+
                                'onclick="delPlaylist($(this).parents(\'tr\'));">' +
                                '<span class="material-icons">delete</span></a>');
                    } else {
                        $('#'+current_app+'List > tbody > tr').on({
                            mouseover: function(){
                                if($(this).children().last().has('a').length == 0)
                                    $(this).children().last().append(
                                        '<a class="pull-right btn-group-hover color-darkgrey" href="#/browse/playlists/' + pagination + '" '+
                                        'onclick="delPlaylist($(this).parents(\'tr\'));">' +
                                        '<span class="material-icons">delete</span></a>');
                            },
                            mouseleave: function(){
                                var doomed = $(this);
                                $(this).children().last().find("a").stop().remove();
                            }
                        });
                    };
                    $('#'+current_app+'List > tbody > tr').on({
                        click: function() {
                                    socket.send("MPD_API_ADD_PLAYLIST," + decodeURI($(this).attr("uri")));
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                        }
                    });
                    if (nrItems == 0) {
                        $('#'+current_app+'List > tbody').append(
                               "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                               "<td colspan=\"3\">No playlists found.</td>" +
                               "<td></td><td></td></tr>"
                        );
                    }
                    break;
                case 'search':
                    $('#panel-heading-search').text(obj.totalEntities + ' Songs found');
                case 'browse':
                    if(current_app !== 'browseFilesystem' && current_app !== 'search')
                        break;
                    
                    /* The use of encodeURI() below might seem useless, but it's not. It prevents
                     * some browsers, such as Safari, from changing the normalization form of the
                     * URI from NFD to NFC, breaking our link with MPD.
                     */
                    var nrItems=0;
                    var tr=document.getElementById(current_app+'List').getElementsByTagName('tbody')[0].getElementsByTagName('tr');
                    for (var item in obj.data) {
                        nrItems++;
                        var row='';
                        switch(obj.data[item].type) {
                            case 'directory':
                                row='<tr uri="' + encodeURI(obj.data[item].dir) + '" class="dir">' +
                                    '<td><span class="material-icons">folder_open</span></td>' +
                                    '<td colspan="3"><a>' + basename(obj.data[item].dir) + '</a></td>' +
                                    '<td></td><td></td></tr>';
                                break;
                            case 'song':
                                var minutes = Math.floor(obj.data[item].duration / 60);
                                var seconds = obj.data[item].duration - minutes * 60;
                                row='<tr uri="' + encodeURI(obj.data[item].uri) + '" class="song">' +
                                    '<td><span class="material-icons">music_note</span></td>' + 
                                    '<td>' + obj.data[item].title  + '</td>' +
                                    '<td>' + obj.data[item].artist + '</td>' + 
                                    '<td>' + obj.data[item].album  + '</td>' +
                                    '<td>' + minutes + ':' + (seconds < 10 ? '0' : '') + seconds +
                                    '</td><td></td></tr>';
                                break;
                        }
                        if (nrItems <= tr.length) { $(tr[nrItems-1]).replaceWith(row); } 
                        else { $('#'+current_app+'List > tbody').append(row); }
                    }
                    for (var i=tr.length;i>nrItems;i--) {
                        $(tr[tr.length-1]).remove();
                    }
                    setPagination(obj.totalEntities);
                    
                    if (current_app == 'search')
                       if (nrItems == 0) {
                           $('#'+current_app+'List > tbody').append(
                               "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                               "<td colspan=\"3\">No results, please refine your search!</td>" +
                               "<td></td><td></td></tr>"
                           );
                    }

                    function appendClickableIcon(appendTo, onClickAction, glyphicon) {
                        $(appendTo).append(
                            '<a role="button" class="pull-right btn-group-hover">' +
                            '<span class="material-icons">' + glyphicon + '</span></a>')
                            .find('a').click(function(e) {
                                e.stopPropagation();
                                socket.send(onClickAction + "," + decodeURI($(this).parents("tr").attr("uri")));
                                showNotification('"' + $('td:nth-last-child(3)', $(this).parents('tr')).text() + '" added','','','success');
                            });
                    }

                    if ( isTouch ) {
                        appendClickableIcon($('#'+current_app+'List > tbody > tr.dir > td:last-child'), 'MPD_API_ADD_TRACK', 'playlist_add');
                        appendClickableIcon($('#'+current_app+'List > tbody > tr.song > td:last-child'), 'MPD_API_ADD_TRACK', 'playlist_add');
                    } else {
                        $('#'+current_app+'List > tbody > tr').on({
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
                    $('#'+current_app+'List > tbody > tr').on({
                        click: function() {
                            switch($(this).attr('class')) {
                                case 'dir':
                                    pagination = 0;
                                    browsepath = $(this).attr("uri");
                                    $("#browseFilesystemList > a").attr("href", '#/browse/filesystem/'+pagination+'/'+browsepath);
                                    app.setLocation('#/browse/filesystem/'+pagination+'/'+browsepath);
                                    break;
                                case 'song':
                                    socket.send("MPD_API_ADD_TRACK," + decodeURI($(this).attr("uri")));
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                                    break;
                            }
                        }
                    });

                    $('#browseBreadcrumb > li > a').on({
			click: function() {
		        	pagination = 0;
				browsepath = $(this).attr("uri");
				$("#browseFilesystemList > a").attr("href", '#/browse/filesystem/'+pagination+'/'+browsepath);
				app.setLocation('#/browse/filesystem/'+pagination+'/'+browsepath);
			}
                    });

                    break;
                case 'state':
                    updatePlayIcon(obj.data.state);
                    updateVolumeIcon(obj.data.volume);

                    if(JSON.stringify(obj) === JSON.stringify(last_state))
                        break;

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
                      $('#queueList > tbody > tr[trackid='+last_state.data.currentsongid+'] > td').eq(4).text(last_state.data.totalTime);
                      $('#queueList > tbody > tr[trackid='+last_state.data.currentsongid+'] > td').eq(0).removeClass('material-icons').text(last_state.data.songpos);
                    }
                    $('#queueList > tbody > tr').removeClass('active').css("font-weight", "");
                        
                    $('#queueList > tbody > tr[trackid='+obj.data.currentsongid+'] > td').eq(4).text(counterText);
                    $('#queueList > tbody > tr[trackid='+obj.data.currentsongid+'] > td').eq(0).addClass('material-icons').text('play_arrow');
                    $('#queueList > tbody > tr[trackid='+obj.data.currentsongid+']').addClass('active').css("font-weight", "bold");

                    
                    last_state = obj;
                    break;
                case 'outputnames':
                    $('#btn-outputs-block button').remove();
                    if ( Object.keys(obj.data).length ) {
		        $.each(obj.data, function(id, name){
                            var btn = $('<button id="btnoutput'+id+'" class="btn btn-secondary btn-block" onclick="toggleoutput(this, '+id+')">'+
                              '<span class="material-icons" style="float:left;">volume_up</span> '+name+'</button>');
                            btn.appendTo($('#btn-outputs-block'));
                        });
		    } else {
                        $('#btn-outputs-block').addClass('hide');
		    }
                    /* remove cache, since the buttons have been recreated */
                    last_outputs = '';
                    break;
                case 'outputs':
                    if(JSON.stringify(obj) === JSON.stringify(last_outputs))
                        break;
                    $.each(obj.data, function(id, enabled){
                        if (enabled)
                            $('#btnoutput'+id).removeClass('btn-secondary').addClass("btn-success")
                        else
                            $('#btnoutput'+id).removeClass("btn-success").addClass("btn-secondary");
                    });
                    last_outputs = obj;
                    break;
                case 'disconnected':
                    showNotification('myMPD lost connection to MPD','','','danger');
                    break;
                case 'update_queue':
                    if(current_app === 'queue') {
                        socket.send('MPD_API_GET_QUEUE,'+pagination);
                    }
                    break;
                case "song_change":
                    songChange(obj.data.title, obj.data.artist, obj.data.album, obj.data.uri);
                    break;
                case 'settings':
                    if (!isNaN(obj.data.max_elements_per_page)) {
                        MAX_ELEMENTS_PER_PAGE=obj.data.max_elements_per_page;
                    }
                    if(obj.data.random)
                        $('#btnrandom').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btnrandom').removeClass("btn-success").addClass("btn-secondary");

                    if(obj.data.consume)
                        $('#btnconsume').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btnconsume').removeClass("btn-success").addClass("btn-secondary");

                    if(obj.data.single)
                        $('#btnsingle').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btnsingle').removeClass("btn-success").addClass("btn-secondary");

                    if(obj.data.crossfade != undefined) {
                        $('#inputCrossfade').removeAttr('disabled');
                        $('#inputCrossfade').val(obj.data.crossfade);
                    } else {
                        $('#inputCrossfade').attr('disabled', 'disabled');
                    }
                    
                    if(obj.data.mixrampdb != undefined) {
                        $('#inputMixrampdb').removeAttr('disabled');
                        $('#inputMixrampdb').val(obj.data.mixrampdb);
                    } else {
                        $('#inputMixrampdb').attr('disabled', 'disabled');
                    }
                    
                    if(obj.data.mixrampdelay != undefined) {
                        $('#inputMixrampdelay').removeAttr('disabled');
                        $('#inputMixrampdelay').val(obj.data.mixrampdelay);
                    } else {
                        $('#inputMixrampdb').attr('disabled', 'disabled');
                    }
                    
                    if(obj.data.repeat)
                        $('#btnrepeat').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btnrepeat').removeClass("btn-success").addClass("btn-secondary");
                    
                    $("#selectReplaygain").val(obj.data.replaygain);

                    setLocalStream(obj.data.mpdhost,obj.data.streamport);
                    coverImageFile=obj.data.coverimage;
                    break;
                case 'mpdstats':
                    $('#mpdstats_artists').text(obj.data.artists);
                    $('#mpdstats_albums').text(obj.data.albums);
                    $('#mpdstats_songs').text(obj.data.songs);
                    $('#mpdstats_dbplaytime').text(beautifyDuration(obj.data.dbplaytime));
                    $('#mpdstats_playtime').text(beautifyDuration(obj.data.playtime));
                    $('#mpdstats_uptime').text(beautifyDuration(obj.data.uptime));
                    var d = new Date(obj.data.dbupdated * 1000);
                    $('#mpdstats_dbupdated').text(d.toUTCString());
                    $('#mympdVersion').text(obj.data.mympd_version);
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

    /*
    /* We open the websocket encrypted if this page came on an
    /* https:// url itself, otherwise unencrypted
    /*/

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

function setPagination(number) {
    var totalPages=Math.ceil(number / MAX_ELEMENTS_PER_PAGE);
    if (totalPages==0) { totalPages=1; }
        $('#'+current_app+'PaginationTopPage').text('Page '+(pagination / MAX_ELEMENTS_PER_PAGE + 1)+' / '+totalPages);
        $('#'+current_app+'PaginationBottomPage').text('Page '+(pagination / MAX_ELEMENTS_PER_PAGE + 1)+' / '+totalPages);
    if (totalPages > 1) {
        $('#'+current_app+'PaginationTopPage').removeClass('disabled');
        $('#'+current_app+'PaginationBottomPage').removeClass('disabled');
        $('#'+current_app+'PaginationTopPages').empty();
        $('#'+current_app+'PaginationBottomPages').empty();
        for (var i=0;i<totalPages;i++) {
            $('#'+current_app+'PaginationTopPages').append('<button onclick="gotoPage('+(i * MAX_ELEMENTS_PER_PAGE)+',this,event)" type="button" class="mr-1 mb-1 btn btn-secondary">'+(i+1)+'</button>');
            $('#'+current_app+'PaginationBottomPages').append('<button onclick="gotoPage('+(i * MAX_ELEMENTS_PER_PAGE)+',this,event)" type="button" class="mr-1 mb-1 btn btn-secondary">'+(i+1)+'</button>');
        }
    } else {
        $('#'+current_app+'PaginationTopPage').addClass('disabled');
        $('#'+current_app+'PaginationBottomPage').addClass('disabled');
    }
    
    if(number > pagination + MAX_ELEMENTS_PER_PAGE) {
        $('#'+current_app+'PaginationTopNext').removeClass('disabled');
        $('#'+current_app+'PaginationBottomNext').removeClass('disabled');
        $('#'+current_app+'ButtonsBottom').removeClass('hide');
    } else {
        $('#'+current_app+'PaginationTopNext').addClass('disabled');
        $('#'+current_app+'PaginationBottomNext').addClass('disabled');
        $('#'+current_app+'ButtonsBottom').addClass('hide');
    }
    
    if(pagination > 0) {
        $('#'+current_app+'PaginationTopPrev').removeClass('disabled');
        $('#'+current_app+'PaginationBottomPrev').removeClass('disabled');
    } else {
        $('#'+current_app+'PaginationTopPrev').addClass('disabled');
        $('#'+current_app+'PaginationBottomPrev').addClass('disabled');
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

function updatePlayIcon(state) {
    $("#play-icon").text('play_arrow');

    if(state == 1) { // stop
        $("#play-icon").text('play_arrow');
        playstate = 'stop';
    } else if(state == 2) { // play
        $("#play-icon").text('pause');
        playstate = 'play';
    } else { // pause
        $("#play-icon").text('play_arrow');
	playstate = 'pause';
    }
}

function updateDB() {
    socket.send('MPD_API_UPDATE_DB');
    showNotification('Updating MPD Database...','','','success');
}

function clickPlay() {
    if( playstate != 'play')
        socket.send('MPD_API_SET_PLAY');
    else
        socket.send('MPD_API_SET_PAUSE');
}

function setLocalStream(mpdhost,streamport) {
    var mpdstream = 'http://';
    if ( mpdhost == '127.0.0.1' || mpdhost == 'localhost')
        mpdstream += window.location.hostname;
    else
        mpdstream += mpdhost;
    mpdstream += ':'+streamport+'/';
    Cookies.set('mpdstream', mpdstream, { expires: 424242 });
}

function delQueueSong(tr) {
    if ( $('#btntrashmodeup').hasClass('btn-success') ) {
        socket.send('MPD_API_RM_RANGE,0,' + (tr.index() + 1));
        tr.remove();
    } else if ( $('#btntrashmodesingle').hasClass('btn-success') ) {
        socket.send('MPD_API_RM_TRACK,' + tr.attr('trackid'));
        tr.remove();
    } else if ( $('#btntrashmodedown').hasClass('btn-success') ) {
        socket.send('MPD_API_RM_RANGE,' + tr.index() + ',-1');
        tr.remove();
    };
}

function delPlaylist(tr) {
    socket.send("MPD_API_RM_PLAYLIST," + decodeURI(tr.attr("uri")));
    tr.remove();
}

function basename(path) {
    return path.split('/').reverse()[0];
}

function toggleBtn(btn) {
    if ($(btn).hasClass('btn-success')) {
      $(btn).removeClass('btn-success').addClass('btn-secondary');
    }
    else {
      $(btn).removeClass('btn-secondary').addClass('btn-success');
    }
}

$('#btnrandom').on('click', function (e) { 
    toggleBtn(this);
});
$('#btnconsume').on('click', function (e) {
    toggleBtn(this);
});
$('#btnsingle').on('click', function (e) {
    toggleBtn(this);
});

$('#btnrepeat').on('click', function (e) {
    toggleBtn(this);
});

function confirmSettings() {
    var formOK=true;
    if (!$('#inputCrossfade').is(':disabled')) {
      var value=parseInt($('#inputCrossfade').val());
      if (!isNaN(value)) {
        $('#inputCrossfade').val(value);
      } else {
        $('#inputCrossfade').popover({"content":"Must be a number","trigger":"manual"});
        $('#inputCrossfade').popover('show');
        $('#inputCrossfade').focus();
        formOK=false;
      }
    }
    if (!$('#inputMixrampdb').is(':disabled')) {
      value=parseFloat($('#inputMixrampdb').val());
      if (!isNaN(value)) {
        $('#inputMixrampdb').val(value);
      } else {
        $('#inputMixrampdb').popover({"content":"Must be a number","trigger":"manual"});
        $('#inputMixrampdb').popover('show');
        $('#inputMixrampdb').focus();
        formOK=false;
      } 
    }
    if (!$('#inputMixrampdelay').is(':disabled')) {
      value=parseFloat($('#inputMixrampdelay').val());
      if (!isNaN(value)) {
        $('#inputMixrampdelay').val(value);
      } else {
        $('#inputMixrampdelay').popover({"content":"Must be a number","trigger":"manual"});
        $('#inputMixrampdelay').popover('show');
        $('#inputMixrampdelay').focus();
        formOK=false;
      }
    }
    if (formOK == true) {
      socket.send("MPD_API_TOGGLE_CONSUME," + ($('#btnconsume').hasClass('btn-success') ? 1 : 0));
      socket.send("MPD_API_TOGGLE_RANDOM," + ($('#btnrandom').hasClass('btn-success') ? 1 : 0));    
      socket.send("MPD_API_TOGGLE_SINGLE," + ($('#btnsingle').hasClass('btn-success') ? 1 : 0));
      socket.send("MPD_API_TOGGLE_REPEAT," + ($('#btnrepeat').hasClass('btn-success') ? 1 : 0));
      socket.send("MPD_API_SET_REPLAYGAIN," + $('#selectReplaygain').val());
      if (!$('#inputCrossfade').is(':disabled'))
          socket.send("MPD_API_SET_CROSSFADE," + $('#inputCrossfade').val());
      if (!$('#inputMixrampdb').is(':disabled'))
          socket.send("MPD_API_SET_MIXRAMPDB," + $('#inputMixrampdb').val());
      if (!$('#inputMixrampdelay').is(':disabled'))
          socket.send("MPD_API_SET_MIXRAMPDELAY," + $('#inputMixrampdelay').val());      
      $('#settings').modal('hide');
    }
}

function toggleoutput(button, id) {
    socket.send("MPD_API_TOGGLE_OUTPUT,"+id+"," + ($(button).hasClass('btn-success') ? 0 : 1));
}

$('#trashmodebtns > button').on('click', function(e) {
    $('#trashmodebtns').children("button").removeClass("btn-success").addClass('btn-secondary');
    $(this).removeClass("btn-secondary").addClass("btn-success");
});

$('#btnnotifyWeb').on('click', function (e) {
    if(Cookies.get('notificationWeb') === 'true') {
        Cookies.set('notificationWeb', false, { expires: 424242 });
        $('#btnnotifyWeb').removeClass('btn-success').addClass('btn-secondary');
    } else {
        Notification.requestPermission(function (permission) {
            if(!('permission' in Notification)) {
                Notification.permission = permission;
            }

            if (permission === 'granted') {
                Cookies.set('notificationWeb', true, { expires: 424242 });
                $('#btnnotifyWeb').removeClass('btn-secondary').addClass('btn-success');
            }
        });
    }
});

$('#btnnotifyPage').on('click', function (e) {
    if(Cookies.get("notificationPage") === 'true') {
        Cookies.set("notificationPage", false, { expires: 424242 });
        $('#btnnotifyPage').removeClass('btn-success').addClass('btn-secondary');
    } else {
        Cookies.set('notificationPage', true, { expires: 424242 });
        $('#btnnotifyPage').removeClass('btn-secondary').addClass('btn-success');
    }
});

$('#search > input').keypress(function (event) {
   if ( event.which == 13 ) {
     $('#mainMenu > a').dropdown('toggle');
   }
});

$('#search').submit(function () {
    doSearch($('#search > input').val());
    return false;
});

function doSearch(searchstr) {
   var mpdtag='Any Tag';
   $('#searchtags2 > button').each(function() {
     if ($(this).hasClass('btn-success')) { mpdtag=$(this).text(); }
   });
   app.setLocation('#/search/' + pagination + '/' + mpdtag + '/' + searchstr);
}

$('#searchstr2').keyup(function (event) {
  pagination=0;
  doSearch($(this).val());
});

$('#searchtags2 > button').on('click',function (e) {
  $('#searchtags2 > button').removeClass('btn-success').addClass('btn-secondary');
  $(this).removeClass('btn-secondary').addClass('btn-success');
  doSearch($('#searchstr2').val());  
});

$('#searchqueuestr').keyup(function (event) {
  pagination=0;
  doQueueSearch();
});

$('#searchqueuetag > button').on('click',function (e) {
  $('#searchqueuetag > button').removeClass('btn-success').addClass('btn-secondary');
  $(this).removeClass('btn-secondary').addClass('btn-success');
  doQueueSearch();  
});

function doQueueSearch() {
   var searchstr=$('#searchqueuestr').val();
   var mpdtag='Any Tag';
   $('#searchqueuetag > button').each(function() {
     if ($(this).hasClass('btn-success')) { mpdtag=$(this).text(); }
   });
   app.setLocation('#/queue/' + pagination + '/' + mpdtag + '/' + searchstr);
}

$('#searchqueue').submit(function () {
    return false;
});

function scrollToTop() {
    document.body.scrollTop = 0; // For Safari
    document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera
}

function gotoPage(x,element,event) {
    if ($(element).hasClass('disabled')) {
        event.preventDefault();
        return;
    }
    switch (x) {
        case "next":
            pagination += MAX_ELEMENTS_PER_PAGE;
            break;
        case "prev":
            pagination -= MAX_ELEMENTS_PER_PAGE;
            if(pagination <= 0)
               pagination = 0;
            break;
        default:
            pagination = x;
    }

    switch(current_app) {
        case "queue":
            if ($('#searchqueuestr').val().length >=3) {
              doQueueSearch();
            } else {
              app.setLocation('#/queue/'+pagination);
            }
            break;
        case "search":
            doSearch($('#searchstr2').val());
            break;
        case "browseFilesystem":
            app.setLocation('#/browse/filesystem/'+pagination+'/'+browsepath);
            break;
        case "browsePlaylists":
            app.setLocation('#/browse/playlists/'+pagination);
            break;
        case "browseDatabase":
            app.setLocation('#/browse/database/'+pagination);
            break;            
    }
    event.preventDefault();
}

function addStream() {
    if($('#streamurl').val().length > 0) {
        socket.send('MPD_API_ADD_TRACK,'+$('#streamurl').val());
    }
    $('#streamurl').val("");
    $('#addstream').modal('hide');
}

function saveQueue() {
    if($('#playlistname').val().length > 0) {
        socket.send('MPD_API_SAVE_QUEUE,'+$('#playlistname').val());
    }
    $('#savequeue').modal('hide');
}

function showNotification(notificationTitle,notificationText,notificationHtml,notificationType) {
    if (Cookies.get('notificationWeb') === 'true') {
      var notification = new Notification(notificationTitle, {icon: 'assets/favicon.ico', body: notificationText});
      setTimeout(function(notification) {
        notification.close();
      }, 3000, notification);    
    } 
    if (Cookies.get('notificationPage') === 'true') {
      $.notify({ title: notificationTitle, message: notificationHtml},{ type: notificationType, offset: { y: 60, x:20 },
        template: '<div data-notify="container" class="col-xs-11 col-sm-3 alert alert-{0}" role="alert">' +
		'<button type="button" aria-hidden="true" class="close" data-notify="dismiss">×</button>' +
		'<span data-notify="icon"></span> ' +
		'<span data-notify="title">{1}</span> ' +
		'<span data-notify="message">{2}</span>' +
		'<div class="progress" data-notify="progressbar">' +
			'<div class="progress-bar progress-bar-{0}" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100" style="width: 0%;"></div>' +
		'</div>' +
		'<a href="{3}" target="{4}" data-notify="url"></a>' +
		'</div>' 
      });
    }
}

function notificationsSupported() {
    return "Notification" in window;
}

function songChange(title, artist, album, uri) {
    var textNotification = '';
    var htmlNotification = '';
    var pageTitle = 'myMPD: ';

    if (typeof uri != 'undefined' && uri.length > 0) {
        var coverImg='';
        if (uri.indexOf('http://') == 0 || uri.indexOf('https://') == 0 ) {
            coverImg='/assets/coverimage-httpstream.png';
        } else if (coverImageFile != '') {
            coverImg='/library/'+uri.replace(/\/[^\/]+$/,'\/'+coverImageFile);
        } else {
            coverImg='/assets/coverimage-notavailable.png';
        }
        $('#album-cover').css('backgroundImage','url("'+coverImg+'"),url("/assets/coverimage-notavailable.png")');
    }
    if(typeof artist != 'undefined' && artist.length > 0 && artist != '-') {
        textNotification += artist;
        htmlNotification += '<br/>' + artist;
        pageTitle += artist + ' - ';
        $('#artist').text(artist);
    } else {
        $('#artist').text('');
    }
    if(typeof album != 'undefined' && album.length > 0 && album != '-') {
        textNotification += ' - ' + album;
        htmlNotification += '<br/>' + album;
        $('#album').text(album);
    }
    else {
        $('#album').text('');
    }
    if(typeof title != 'undefined' && title.length > 0) {
        pageTitle += title;
        $('#currenttrack').text(title);
    } else {
        $('#currenttrack').text('');
    }
    document.title = pageTitle;
    showNotification(title,textNotification,htmlNotification,'success');
}

        
$(document).keydown(function(e){
    if (e.target.tagName == 'INPUT') {
        return;
    }
    switch (e.which) {
        case 37: //left
            socket.send('MPD_API_SET_PREV');
            break;
        case 39: //right
            socket.send('MPD_API_SET_NEXT');
            break;
        case 32: //space
            clickPlay();
            break;
        default:
            return;
    }
    e.preventDefault();
});

function add_filter () {
    for (i = 65; i <= 90; i++) {
        var c = String.fromCharCode(i);
        $('#browseFilesystemGotoLetters').append('<a class="btn btn-secondary" onclick="gotoLetter(\'' + c + '\');" href="#/browse/filesystem/' + pagination + '/' + browsepath + '">' + c + '</a>');
    }
}

function chVolume (increment) {
 var aktValue=volumeBar.slider('getValue');
 var newValue=aktValue+increment;
 if (newValue<0) { newValue=0; }
 else if (newValue > 100) { newValue=100; }
 volumeBar.slider('setValue',newValue);
 socket.send("MPD_API_SET_VOLUME,"+newValue);
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