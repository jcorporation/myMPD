"use strict";
/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

function navigateGrid(grid, keyCode) {
    let handled = false;
    if (keyCode === 'Enter') {
        if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
            if (app.current.tag === 'Album') {
                appGoto('Browse', 'Database', 'Detail', '0','Album','AlbumArtist', 
                    decodeURI(grid.getAttribute('data-album')),
                    decodeURI(grid.getAttribute('data-albumartist')));
            }
            else {
                app.current.search = '';
                document.getElementById('searchDatabaseStr').value = '';
                appGoto(app.current.app, app.current.card, undefined, '0', 'Album', 'AlbumArtist', 'Album',
                    '(' + app.current.tag + ' == \'' + decodeURI(grid.getAttribute('data-tag')) + '\')');
            }
            handled = true;
        }
        else if (app.current.app === 'Home') {
            const href = event.target.getAttribute('data-href');
            if (href !== null) {
               parseCmd(event, href);
            }
        }
    }
    else if (keyCode === ' ') {
        if (app.current.app === 'Browse' && app.current.tab === 'Database' && app.current.view === 'List') {
            if (app.current.tag === 'Album') {
                showMenu(grid.getElementsByClassName('card-footer')[0], event);
            }
            handled = true;
        }
    }
    else if (keyCode === 'ArrowDown' || keyCode === 'ArrowUp') {
        const cur = grid;
        const next = keyCode === 'ArrowDown' ? (grid.parentNode.nextElementSibling !== null ? grid.parentNode.nextElementSibling.firstChild : null)
                                             : (grid.parentNode.previousElementSibling !== null ? grid.parentNode.previousElementSibling.firstChild : null);
        if (next !== null) {
            next.focus();
            cur.classList.remove('selected');
            next.classList.add('selected');
            handled = true;
            scrollFocusIntoView();
        }
    }
    else if (keyCode === 'Escape') {
        const cur = grid;
        cur.blur();
        cur.classList.remove('selected');
        handled = true;
    }
    if (handled === true) {
        event.stopPropagation();
        event.preventDefault();
    }
}
