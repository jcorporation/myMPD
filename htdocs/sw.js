var CACHE = 'myMPD-cache-v3.3.0';
var urlsToCache = [
    '/',
    '/player.html',
    '/css/bootstrap.min.css',
    '/css/mpd.css',
    '/js/bootstrap-native-v4.min.js',
    '/js/mpd.js',
    '/js/player.js',
    '/assets/appicon-167.png',
    '/assets/appicon-192.png',
    '/assets/appicon-512.png',
    '/assets/coverimage-httpstream.png',
    '/assets/coverimage-notavailable.png',
    '/assets/favicon.ico',
    '/assets/MaterialIcons-Regular.eot',
    '/assets/MaterialIcons-Regular.ttf',
    '/assets/MaterialIcons-Regular.woff',
    '/assets/MaterialIcons-Regular.woff2'
];

self.addEventListener('install', function(event) {
    event.waitUntil(
        caches.open(CACHE).then(function(cache) {
            return cache.addAll(urlsToCache);
        })
    );
});

self.addEventListener('fetch', function(event) {
  event.respondWith(
    caches.match(event.request).then(function(response) {
        if (response)
            return response
        else
            return fetch(event.request);
      }
    )
  );    
});

self.addEventListener('activate', function(event) {
    event.waitUntil(
        caches.keys().then(function(cacheNames) {
            return Promise.all(
                cacheNames.map(function(cacheName) {
                    if (cacheName != CACHE)
                        return caches.delete(cacheName);
                })
            );
        })
    );
});
