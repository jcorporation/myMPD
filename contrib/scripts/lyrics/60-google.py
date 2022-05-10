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
# Load lyrics from google.com if lyrics weren't found in the lyrics directory
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


base_url = 'http://www.google.com/'
artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [
    artist.removeprefix('the ')
    for artist in artists.split(',')
]

title = re.sub('\(.*\)', '', title)

for artist in artists:
    artist = artist.replace(' ', '+')
    title = title.replace(' ', '+')
    r = requests.get(f'{base_url}/search?q={artist}+{title}+lyrics',
                     headers={'client': "google-csbe"})
    try:
        r.raise_for_status()
    except:
        exit(1)
    soup = bs4.BeautifulSoup(r.text, 'html5lib')
    results = [x for x in soup.select('div') if len(x.select('div')) == 0]
    # element with the longest text node is usually a match
    results.sort(key=lambda n: len(n.text))
    print(results[-1].text)
