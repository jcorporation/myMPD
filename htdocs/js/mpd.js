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
var MAX_ELEMENTS_PER_PAGE = 5;
var isTouch = Modernizr.touch ? 1 : 0;
var filter = "";
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
        $('.pagination').addClass('hide');
        $('#searchqueue > input').val('');
        pagination = 0;
        browsepath = '';
    }

    this.get (/\#\/playing\//, function() {
        prepare();
        current_app = 'nowplaying';
        $('#cardPlayback').removeClass('hide');
        $('#navPlayback').addClass('active');
    });    

    this.get(/\#\/queue\/(\d+)/, function() {
        prepare();
        current_app = 'queue';
        $('#navQueue').addClass('active');
        $('#cardQueue').removeClass('hide');
        $('#panel-heading-queue').empty();
        pagination = parseInt(this.params['splat'][0]);
        $('#queueList').find("tr:gt(0)").remove();
        socket.send('MPD_API_GET_QUEUE,'+pagination);        
    });

    this.get(/\#\/browse\/(\d+)\/(.*)/, function() {
        prepare();
        browsepath = this.params['splat'][1];
        pagination = parseInt(this.params['splat'][0]);
        current_app = 'browse';
        $('#navBrowse').addClass('active');
        $('#cardBrowse').removeClass('hide');
        $('#browseList').find("tr:gt(0)").remove();
        $('#browseBreadcrumb').empty().append("<li class=\"breadcrumb-item\"><a uri=\"\" onclick=\"set_filter('')\">root</a></li>");
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
        $('#panel-heading-browse').text("Browse database: /"+browsepath);

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

    this.get(/\#\/search\/(.*)/, function() {

        prepare();
        current_app = 'search';
        $('#cardSearch').removeClass('hide');
        var searchstr = this.params['splat'][0];

        $('#search > input').val(searchstr);
        socket.send('MPD_API_SEARCH,' + searchstr);
        $('#searchList').find("tr:gt(0)").remove();
        $('#searchList > tbody').append(
            "<tr><td><span class=\"material-icons\">search</span></td>" +
            "<td colspan=\"3\">Searching</td>" +
            "<td></td><td></td></tr>");
        $('#panel-heading-search').text("Search: "+searchstr);
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
            showNotification('Connected to myMPD','','','success');
            $('#modalConnectionError').modal('hide');    
            app.run();
            /* emit request for mympd options */
            socket.send('MPD_API_GET_OPTIONS');            
            /* emit initial request for output names */
            socket.send('MPD_API_GET_OUTPUTS');

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
                    
                    if (obj.totalSongs > 0) {
                        $('#panel-heading-queue').text(obj.totalSongs+' Songs');
                    }
                    if (typeof(obj.totalTime) != undefined && obj.totalTime > 0 ) {
                        var days = Math.floor(obj.totalTime / 86400);
                        var hours = Math.floor(obj.totalTime / 3600) - days * 24;
                        var minutes = Math.floor(obj.totalTime / 60) - hours * 60 - days * 1440;
                        var seconds = obj.totalTime - days * 86400 - hours * 3600 - minutes * 60;
                        
                        $('#panel-heading-queue').append(' – ' +
                            (days > 0 ? days + '\u2009d ' : '') +
                            (hours > 0 ? hours + '\u2009h ' + (minutes < 10 ? '0' : '') : '') +
                            minutes + '\u2009m ' + (seconds < 10 ? '0' : '') + seconds + '\u2009s');
                    }

                    $('#queueList > tbody').empty();
                    var nrItems=0;
                    for (var song in obj.data) {
                        nrItems++;
                        var minutes = Math.floor(obj.data[song].duration / 60);
                        var seconds = obj.data[song].duration - minutes * 60;

                        $('#queueList > tbody').append(
                            "<tr trackid=\"" + obj.data[song].id + "\"><td>" + (obj.data[song].pos + 1) + "</td>" +
                                "<td>"+ obj.data[song].title +"</td>" +
                                "<td>"+ obj.data[song].artist +"</td>" + 
                                "<td>"+ obj.data[song].album +"</td>" +
                                "<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
                        "</td><td></td></tr>");
                    }
                    if (obj.type == 'queuesearch' && nrItems == 0) {
                        $('#queueList > tbody').append(
                               "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                               "<td colspan=\"3\">No results, please refine your search!</td>" +
                               "<td></td><td></td></tr>"
                        );
                    }
                    totalPages=Math.ceil(obj.totalSongs / MAX_ELEMENTS_PER_PAGE);
                    if (totalPages==0) { totalPages=1; }
                    $('#queuePaginationTopPage').text('Page '+(pagination / MAX_ELEMENTS_PER_PAGE + 1)+' / '+totalPages);
                    $('#queuePaginationBottomPage').text('Page '+(pagination / MAX_ELEMENTS_PER_PAGE + 1)+' / '+totalPages);
                    if (totalPages > 1) {
                        $('#queuePaginationTopPage').removeClass('disabled');
                        $('#queuePaginationBottomPage').removeClass('disabled');
                        $('#queuePaginationTopPages').empty();
                        $('#queuePaginationBottomPages').empty();
                        for (var i=0;i<totalPages;i++) {
                            $('#queuePaginationTopPages').append('<button onclick="gotoPage('+(i * MAX_ELEMENTS_PER_PAGE)+',this,event)" type="button" class="mr-1 mb-1 btn btn-secondary">'+(i+1)+'</button>');
                            $('#queuePaginationBottomPages').append('<button onclick="gotoPage('+(i * MAX_ELEMENTS_PER_PAGE)+',this,event)" type="button" class="mr-1 mb-1 btn btn-secondary">'+(i+1)+'</button>');
                        }
                    } else {
                        $('#queuePaginationTopPage').addClass('disabled');
                        $('#queuePaginationBottomPage').addClass('disabled');
                    }
                    if(obj.totalSongs > pagination + MAX_ELEMENTS_PER_PAGE) {
                        $('#queuePaginationTopNext').removeClass('disabled');
                        $('#queuePaginationBottomNext').removeClass('disabled');
                    } else {
                        $('#queuePaginationTopNext').addClass('disabled');
                        $('#queuePaginationBottomNext').addClass('disabled');                            
                    }
                    if(pagination > 0) {
                        $('#queuePaginationTopPrev').removeClass('disabled');
                        $('#queuePaginationBottomPrev').removeClass('disabled');
                    } else {
                        $('#queuePaginationTopPrev').addClass('disabled');
                        $('#queuePaginationBottomPrev').addClass('disabled');
                    }

                    if ( isTouch ) {
                        $('#queueList > tbody > tr > td:last-child').append(
                                    "<a class=\"pull-right btn-group-hover color-darkgrey\" href=\"#/\" " +
                                        "onclick=\"trash($(this).parents('tr'));\">" +
                                "<span class=\"material-icons\">delete</span></a>");
                    } else {
                        $('#queueList > tbody > tr').on({
                            mouseover: function(){
                                var doomed = $(this);
                                if ( $('#btntrashmodeup').hasClass('btn-success') )
                                    doomed = $("#queueList > tbody > tr:lt(" + ($(this).index() + 1) + ")");
                                if ( $('#btntrashmodedown').hasClass('btn-success') )
                                    doomed = $("#queueList > tbody > tr:gt(" + ($(this).index() - 1) + ")");
                                $.each(doomed, function(){
                                if($(this).children().last().has("a").length == 0)
                                    $(this).children().last().append(
                                        "<a class=\"pull-right btn-group-hover color-darkgrey\" href=\"#/\" " +
                                            "onclick=\"trash($(this).parents('tr'));\">" +
                                    "<span class=\"material-icons\">delete</span></a>")
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
                case 'search':
                    $('#searchList').find("tr:gt(0)").remove();
                case 'browse':
                    if(current_app !== 'browse' && current_app !== 'search')
                        break;
                    
                    /* The use of encodeURI() below might seem useless, but it's not. It prevents
                     * some browsers, such as Safari, from changing the normalization form of the
                     * URI from NFD to NFC, breaking our link with MPD.
                     */
                    var nrItems=0;
                    for (var item in obj.data) {
                        nrItems++;
                        switch(obj.data[item].type) {
                            case 'directory':
                                var clazz = 'dir';
                                if (filter !== "") {
                                    var first = obj.data[item].dir[0];
                                    if (filter === "num" && isNaN(first)) {
                                        clazz += ' hide';
                                    } else if (filter >= "A" && filter <= "Z" && first.toUpperCase() !== filter) {
                                        clazz += ' hide';
                                    } else if (filter === "plist") {
                                        clazz += ' hide';
                                    }
                                }
                                $('#'+current_app+'List > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].dir) + "\" class=\"" + clazz + "\">" +
                                    "<td><span class=\"material-icons\">folder_open</span></td>" +
                                    "<td colspan=\"3\"><a>" + basename(obj.data[item].dir) + "</a></td>" +
                                    "<td></td><td></td></tr>"
                                );
                                break;
                            case 'playlist':
                                var clazz = 'plist';
                                if ( (filter !== "") && (filter !== "plist") ) {
                                    clazz += ' hide';
                                }
                                $('#'+current_app+'List > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].plist) + "\" class=\"" + clazz + "\">" +
                                    "<td><span class=\"material-icons\">list</span></td>" +
                                    "<td colspan=\"3\"><a>" + basename(obj.data[item].plist) + "</a></td>" +
                                    "<td></td><td></td></tr>"
                                );
                                break;
                            case 'song':
                                var minutes = Math.floor(obj.data[item].duration / 60);
                                var seconds = obj.data[item].duration - minutes * 60;

                                if (obj.data[item].artist == null) {
                                    var artist = "<td colspan=\"2\">";
                                } else {
                                    var artist = "<td>" + obj.data[item].artist +
                                                     "<span>" + obj.data[item].album + "</span></td><td>";
                                }

                                $('#'+current_app+'List > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].uri) + "\" class=\"song\">" +
                                    "<td><span class=\"material-icons\">music_note</span></td>" + 
                                    "<td>" + obj.data[item].title  + "</td>" +
                                    "<td>" + obj.data[item].artist + "</td>" + 
                                    "<td>" + obj.data[item].album  + "</td>" +
                                    "<td>" + minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
                                    "</td><td></td></tr>"
                                );
                                break;
                            case 'wrap':
                                if(current_app == 'browse') {
                                    $('#browseNext').removeClass('disabled');
                                    $('#browsePagination').removeClass('hide');
                                } else {
                                    $('#'+current_app+'List > tbody').append(
                                        "<tr><td><span class=\"material-icons\">error_outline</span></td>" +
                                        "<td colspan=\"3\">Too many results, please refine your search!</td>" +
                                        "<td></td><td></td></tr>"
                                    );
                                }
                                break;
                        }

                        if(pagination > 0) {
                            $('#browsePrev').removeClass('disabled');
                            $('#browsePagination').removeClass('hide');
                        }

                    }
                    
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
                                    $("#browse > a").attr("href", '#/browse/'+pagination+'/'+browsepath);
									$('#filter > a').attr("href", '#/browse/'+pagination+'/'+browsepath);
                                    app.setLocation('#/browse/'+pagination+'/'+browsepath);
									set_filter('');
                                    break;
                                case 'song':
                                    socket.send("MPD_API_ADD_TRACK," + decodeURI($(this).attr("uri")));
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                                    break;
                                case 'plist':
                                    socket.send("MPD_API_ADD_PLAYLIST," + decodeURI($(this).attr("uri")));
                                    showNotification('"' + $('td:nth-last-child(3)', this).text() + '" added','','','success');
                                    break;
                            }
                        }
                    });

                    $('#browseBreadcrumb > li > a').on({
			click: function() {
		        	pagination = 0;
				browsepath = $(this).attr("uri");
				$("#browse > a").attr("href", '#/browse/'+pagination+'/'+browsepath);
				$('#filter > a').attr("href", '#/browse/'+pagination+'/'+browsepath);
				app.setLocation('#/browse/'+pagination+'/'+browsepath);
				set_filter('');
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

                    if(obj.data.crossfade)
                        $('#btncrossfade').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btncrossfade').removeClass("btn-success").addClass("btn-secondary");

                    if(obj.data.repeat)
                        $('#btnrepeat').removeClass('btn-secondary').addClass("btn-success")
                    else
                        $('#btnrepeat').removeClass("btn-success").addClass("btn-secondary");

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
                case 'mpdoptions':
                    setLocalStream(obj.data.mpdhost,obj.data.streamport);
                    coverImageFile=obj.data.coverimage;
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

function updateVolumeIcon(volume) {
    $('#volumePrct').text(volume+' %');
    if(volume == 0) {
        $("#volume-icon").text("volume_off");
    } else if (volume < 50) {
        $("#volume-icon").text("volume_down");
    } else {
        $("#volume-icon").text("volume_up");
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

function trash(tr) {
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

function basename(path) {
    return path.split('/').reverse()[0];
}

$('#btnrandom').on('click', function (e) {
    socket.send("MPD_API_TOGGLE_RANDOM," + ($(this).hasClass('btn-success') ? 0 : 1));

});
$('#btnconsume').on('click', function (e) {
    socket.send("MPD_API_TOGGLE_CONSUME," + ($(this).hasClass('btn-success') ? 0 : 1));

});
$('#btnsingle').on('click', function (e) {
    socket.send("MPD_API_TOGGLE_SINGLE," + ($(this).hasClass('btn-success') ? 0 : 1));
});
$('#btncrossfade').on('click', function(e) {
    socket.send("MPD_API_TOGGLE_CROSSFADE," + ($(this).hasClass('btn-success') ? 0 : 1));
});
$('#btnrepeat').on('click', function (e) {
    socket.send("MPD_API_TOGGLE_REPEAT," + ($(this).hasClass('btn-success') ? 0 : 1));
});

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
    app.setLocation("#/search/"+$('#search > input').val());
    return false;
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
   
   if (searchstr.length >= 3) {
      socket.send('MPD_API_SEARCH_QUEUE,' + mpdtag + ','+pagination+',' + searchstr);
   }
   if (searchstr.length == 0) {
     socket.send('MPD_API_GET_QUEUE,0');
   }
}

$('#searchqueue').submit(function () {
    return false;
});

$('.page-link').on('click', function (e) {

    switch ($(this).text()) {
        case "Next":
            pagination += MAX_ELEMENTS_PER_PAGE;
            break;
        case "Previous":
            pagination -= MAX_ELEMENTS_PER_PAGE;
            if(pagination <= 0)
               pagination = 0;
            break;
    }

    switch(current_app) {
        case "queue":
            app.setLocation('#/queue/'+pagination);
            break;
        case "browse":
            app.setLocation('#/browse/'+pagination+'/'+browsepath);
            break;
    }
    e.preventDefault();
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
        case "browse":
            app.setLocation('#/browse/'+pagination+'/'+browsepath);
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
        if (uri.indexOf('http://') == 0) {
            coverImg='/assets/httpstream.png';
        } else {
            coverImg='/library/'+uri.replace(/\/[^\/]+$/,'\/'+coverImageFile);
        }
        $('#album-cover').css('backgroundImage','url("'+coverImg+'")');
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

function set_filter (c) {
    filter = c;
	$('#filter > a').removeClass('btn-success');
	$('#f' + c).addClass('btn-success');

    if (filter === "") {
    	$('#'+current_app+'List > tbody > tr').removeClass('hide');
	} else if (filter === "plist") {
    	$('#'+current_app+'List > tbody > tr.dir').addClass('hide');
    	$('#'+current_app+'List > tbody > tr.song').addClass('hide');
    	$('#'+current_app+'List > tbody > tr.plist').removeClass('hide');
    } else {
		$.each($('#'+current_app+'List > tbody > tr'), function(i, line) {
			var first = basename($(line).attr('uri'))[0];
			if ( $(line).hasClass('song') ) {
				first = $(line).children().eq(3).text()[0];
			}

			if (filter === "num") {
				if (!isNaN(first)) {
					$(line).removeClass('hide');
				} else {
					$(line).addClass('hide');
				}
			} else if (filter >= "A" && filter <= "Z") {
				if (first.toUpperCase() === filter) {
					$(line).removeClass('hide');
				} else {
					$(line).addClass('hide');
				}
			}
		});
	}
}

function add_filter () {
    $('#filter').append('<a class="btn btn-secondary" onclick="set_filter(\'\')" href="#/browse/'+pagination+'/'+browsepath+'">All</a>');
    $('#filter').append('<a class="btn btn-secondary" id="fnum" onclick="set_filter(\'num\')" href="#/browse/'+pagination+'/'+browsepath+'">#</a>');

    for (i = 65; i <= 90; i++) {
        var c = String.fromCharCode(i);
        $('#filter').append('<a class="btn btn-secondary" id="f' + c + '" onclick="set_filter(\'' + c + '\');" href="#/browse/' + pagination + '/' + browsepath + '">' + c + '</a>');
    }

    $('#filter').append('<a class="btn btn-secondary material-icons" id="fplist" onclick="set_filter(\'plist\')" href="#/browse/'+pagination+'/'+browsepath+'">list</a>');
}

function chVolume (increment) {
 var aktValue=volumeBar.slider('getValue');
 var newValue=aktValue+increment;
 if (newValue<0) { newValue=0; }
 else if (newValue > 100) { newValue=100; }
 volumeBar.slider('setValue',newValue);
 socket.send("MPD_API_SET_VOLUME,"+newValue);
}