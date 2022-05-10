#!/usr/bin/python3
#
#  Copyright 2004-2021 The Music Player Daemon Project
#  http://www.musicpd.org/
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

#
# Load lyrics from supermusic.cz if lyrics weren't found in the lyrics directory
#
import re
import sys
import urllib.request

import requests
import html
import bs4

try:
    # using the "unidecode" library if installed
    # (https://pypi.org/project/Unidecode/ or package
    # "python3-unidecode" on Debian)
    from unidecode import unidecode
except ImportError:
    # Dummy fallback (don't crash if "unidecode" is not installed)
    def unidecode(s):
        return s


def normalize_parameter(s):
    return unidecode(s).lower()


artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [
    artist.removeprefix('the ')
    for artist in artists.split(',')
]

title = re.sub('\(.*\)', '', title)
base_url = 'https://supermusic.cz/'

for artist in artists:
    r = requests.post(f'{base_url}najdi.php',
                      data={'hladane': f'"{title}"', 'typhladania': 'piesen', 'fraza': 'off'})
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, 'html5lib')
    results = soup.select('.clanok a[target]')
    # drop melody, notes and guitar tabs from results
    results = [x for x in results if '- taby' not in x.nextSibling and
                                  '- melodia' not in x.nextSibling and
                                  '- preklad' not in x.nextSibling and
                                  '- noty' not in x.nextSibling]
    # filter on artist name
    results = [x for x in results if normalize_parameter(x.text) == artist]
    if not results:
        print("Lyrics not found :(", file=sys.stderr)
        exit(1)
    song_url = f'{base_url}{results[0]["href"]}'
    song_id = re.search(r'idpiesne=(.*)', song_url).group(1)
    r = requests.get(f'{base_url}export.php?idpiesne={song_id}&typ=TXT')
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, 'html5lib')
    print(''.join([x.text for x in soup.select_one('td[valign="top"]') if x.text != 'SŤAHUJ']))
