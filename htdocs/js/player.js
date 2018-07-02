var player = document.getElementById('player');
player.src = decodeURI(location.hash).replace(/^#/,'');
console.log('playing mpd stream: ' + player.src);
player.load();
player.play();
