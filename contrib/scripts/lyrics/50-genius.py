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
# Load lyrics from genius.com if lyrics weren't found in the lyrics directory
#
import bs4
import re
import requests
import sys

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


base_url = 'https://genius.com/'
artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [
    artist
    for artist in artists.split(',')
]

title = re.sub('\(.*\)', '', title)

for artist in artists:
    title = title.replace(' ', '-').replace("'", "")
    artist = artist.replace(' ', '-').replace("'", "")
    r = requests.get(f'{base_url}{artist}-{title}-lyrics')
    try:
        r.raise_for_status()
    except:
        exit(1)
    soup = bs4.BeautifulSoup(r.text, 'html5lib')
    lyrics = [x.getText('\n') for x in soup.find(attrs={'data-lyrics-container': 'true'})]
    print(re.sub(r'\n\n+', '\n', ''.join([x.replace('', '\n') if not x else x for x in lyrics])))
