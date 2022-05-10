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
# Load lyrics from karaoketexty.sk if lyrics weren't found in the lyrics directory
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


artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [
    artist.removeprefix('the ')
    for artist in artists.split(',')
]

title = re.sub('\(.*\)', '', title)

for artist in artists:
    r = requests.get(f'https://www.karaoketexty.sk/search?sid=outxq&q={artist}-{title}')
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text, 'html5lib')
    results = soup.findAll(attrs={'class': 'h2_search'})[1]
    matches = [x.a for x in results.findAllNext('span', attrs={'class': 'searchresrow_songs'})]
    matches = [x for x in matches if re.search(title, normalize_parameter(x.text).split(' - ')[1])]
    if not matches:
        print("Lyrics not found :(", file=sys.stderr)
        exit(1)

    # first match is a good match
    song_url = f'https://www.karaoketexty.sk{matches[0].get("href")}'
    r = requests.get(song_url)
    r.raise_for_status()
    soup = bs4.BeautifulSoup(r.text)
    lyrics_block = soup.find(attrs={'class': 'lyrics_cont'}).findAll(attrs={'class': 'para_row'})
    lyrics_paragraphs = [x.findAll('span') for x in lyrics_block]
    lyrics = ''.join([x.text for x in soup.find(attrs={'class': 'lyrics_cont'}).findAll(attrs={'class': 'para_row'})])
    print('\n'.join(['\n'.join([x.text for x in x]) for x in lyrics_paragraphs]))
