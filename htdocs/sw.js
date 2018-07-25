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
    console.log('The service worker is being installed.');
    event.waitUntil(preCache());
});

self.addEventListener('fetch', function(event) {
    if (!navigator.onLine) {
        event.respondWith(fromCache(event.request));
    } 
    else {
        var url = event.request.url.replace(/^https?:\/\/[^\/]+/, '');
        if (urlsToCache.includes(url)) {
            event.respondWith(fromCache(event.request));
            event.waitUntil(update(event.request));
        }
    }
});

function preCache() {
    return caches.open(CACHE).then(function(cache) {
        return cache.addAll(urlsToCache);
    });
}

function fromCache(request) {
    return caches.open(CACHE).then(function(cache) {
        return cache.match(request).then(function(matching) {
            return matching || Promise.reject('no-match');
        });
    });
}

function update(request) {
    return caches.open(CACHE).then(function(cache) {
        return fetch(request).then(function(response) {
            return cache.put(request, response);
        });
    });
}

self.addEventListener('activate', function(event) {
    var cacheWhitelist = ['myMPD-cache-v3.3.0'];
    event.waitUntil(
        caches.keys().then(function(cacheNames) {
            return Promise.all(
                cacheNames.map(function(cacheName) {
                    if (cacheWhitelist.indexOf(cacheName) === -1)
                        return caches.delete(cacheName);
                })
            );
        })
    );
});
