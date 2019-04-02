var CACHE = 'myMPD-cache-v5.3.0';
var urlsToCache = [
    '/',
    '/player.html',
    '/css/bootstrap.min.css',
    '/css/mympd.min.css',
    '/js/bootstrap-native-v4.min.js',
    '/js/mympd.min.js',
    '/js/player.min.js',
    '/js/keymap.min.js',
    '/assets/appicon-167.png',
    '/assets/appicon-192.png',
    '/assets/appicon-512.png',
    '/assets/coverimage-httpstream.png',
    '/assets/coverimage-notavailable.png',
    '/assets/coverimage-loading.png',
    '/assets/favicon.ico',
    '/assets/MaterialIcons-Regular.woff2'
];

var ignoreRequests = new RegExp('(' + [
  '/api',
  '/ws',
  '/library\/(.*)',
  '/pics\/(.*)'].join('(\/?)|\\') + ')$')

self.addEventListener('install', function(event) {
    event.waitUntil(
        caches.open(CACHE).then(function(cache) {
            return cache.addAll(urlsToCache);
        })
    );
});

self.addEventListener('fetch', function(event) {
    if (event.request.url.match('^http://')) {
        return false;
    }
    if (ignoreRequests.test(event.request.url)) {
        return false;
    }
    event.respondWith(
        caches.match(event.request).then(function(response) {
            if (response) {
                return response
            }
            else {
                return fetch(event.request);
            }
        })
    );    
});

self.addEventListener('activate', function(event) {
    event.waitUntil(
        caches.keys().then(function(cacheNames) {
            return Promise.all(
                cacheNames.map(function(cacheName) {
                    if (cacheName != CACHE) {
                        return caches.delete(cacheName);
                    }
                })
            );
        })
    );
});
