function initLocalplayer() {
   document.getElementById('alertLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);
    
    document.getElementById('errorLocalPlayback').getElementsByTagName('a')[0].addEventListener('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        clickCheckLocalPlayerState(event);
    }, false);

    document.getElementById('localPlayer').addEventListener('click', function(event) {
        event.stopPropagation();
    });
    
    document.getElementById('localPlayer').addEventListener('canplay', function() {
        logDebug('localPlayer event: canplay');
        document.getElementById('alertLocalPlayback').classList.add('hide');
        document.getElementById('errorLocalPlayback').classList.add('hide');
    });

    document.getElementById('localPlayer').addEventListener('error', function() {
        logError('localPlayer event: error');
        document.getElementById('errorLocalPlayback').classList.remove('hide');
    });
}

function setLocalPlayerUrl() {
    if (window.location.protocol === 'https:') {
        document.getElementById('infoLocalplayer').classList.remove('hide');
        document.getElementById('selectStreamMode').options[0].setAttribute('data-phrase','HTTPS Port');
    }
    else {
        document.getElementById('infoLocalplayer').classList.add('hide');
        document.getElementById('selectStreamMode').options[0].setAttribute('data-phrase','HTTP Port');
    }
    if (settings.streamUrl === '') {
        settings.mpdstream = window.location.protocol + '//';
        if (settings.mpdHost.match(/^127\./) !== null || settings.mpdHost === 'localhost' || settings.mpdHost.match(/^\//) !== null) {
            settings.mpdstream += window.location.hostname;
        }
        else {
            settings.mpdstream += settings.mpdHost;
        }
        settings.mpdstream += ':' + settings.streamPort + '/';
    } 
    else {
        settings.mpdstream = settings.streamUrl;
    }
    const localPlayer = document.getElementById('localPlayer');
    if (localPlayer.src !== settings.mpdstream) {
        localPlayer.pause();
        localPlayer.src = settings.mpdstream;
        localPlayer.load();
        setTimeout(function() {
            checkLocalPlayerState();
        }, 500);
    }

}

function clickCheckLocalPlayerState(event) {
    const el = event.target;
    el.classList.add('disabled');
    const parent = document.getElementById('localPlayer').parentNode;
    document.getElementById('localPlayer').remove();
    let localPlayer = document.createElement('audio');
    localPlayer.setAttribute('preload', 'none');
    localPlayer.setAttribute('controls', '');
    localPlayer.setAttribute('id', 'localPlayer');
    localPlayer.classList.add('mx-4');
    parent.appendChild(localPlayer);
    setLocalPlayerUrl();
    setTimeout(function() {
        el.classList.remove('disabled');
        localPlayer.play();
    }, 500);
}

function checkLocalPlayerState() {
    const localPlayer = document.getElementById('localPlayer');
    document.getElementById('errorLocalPlayback').classList.add('hide');
    document.getElementById('alertLocalPlayback').classList.add('hide');
    if (localPlayer.networkState === 0) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
        document.getElementById('alertLocalPlayback').classList.remove('hide');
    }
    else if (localPlayer.networkState >=1) {
        logDebug('localPlayer networkState: ' + localPlayer.networkState);
    }
    if (localPlayer.networkState === 3) {
        logError('localPlayer networkState: ' + localPlayer.networkState);
        document.getElementById('errorLocalPlayback').classList.remove('hide');
    }
}
