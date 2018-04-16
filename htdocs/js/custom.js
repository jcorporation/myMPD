var div=document.createElement('div');
div.id='album-cover';
div.style='background-size:cover;border:1px solid black;border-radius:5px;overflow:hidden;float:left;margin-right:20px;width:120px;height:120px;background-color:#eee;';

var pb=document.querySelector('.panel-body');
pb.insertBefore(div,pb.childNodes[0]);

document.getElementById('btnlove').parentNode.style.display='none';
document.getElementById('player').parentNode.style.display='none';

function changeCover(obj) {
 if (obj.data.artist && obj.data.album) {
  var coverImg=obj.data.album_artist.replace(/ /g,'_')+'-'+obj.data.album.replace(/ /g,'_')+'.jpg';
  document.getElementById('album-cover').style.backgroundImage='url("/covers/'+coverImg+'")';
 }
 else {
  document.getElementById('album-cover').style.backgroundImage='';
 }
}
