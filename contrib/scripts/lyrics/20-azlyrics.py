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
# Load lyrics from azlyrics.com if lyrics weren't found in the lyrics directory
#
import re
import sys
import urllib.request

import requests
import html

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

def html_elements_to_text(s):
    return s \
        .replace("<br>", "") \
        .replace("<i>", "").replace("</i>", "") \
        .replace("<b>", "").replace("</b>", "") \
        .replace("<div>", "") \
        .replace("</div>", "")

def html_to_text(s):
    return html.unescape(html_elements_to_text(s))

artists = normalize_parameter(sys.argv[1])
title = normalize_parameter(sys.argv[2])
artists = [
    artist.removeprefix('the ')
    for artist in artists.split(',')
]
artists = [
    re.sub("[^A-Za-z0-9]+", "", artist)
    for artist in artists
]
title = re.sub('\(.*\)', '', title)
title = re.sub("[^A-Za-z0-9]+", "", title)

for artist in artists:
    url = "http://azlyrics.com/lyrics/" + artist + "/" + title + ".html"

    try:
        r = urllib.request.urlopen(url)
        response = r.read().decode()
        start = response.find("that. -->")
        end = response.find("<!-- MxM")
        lyrics = response[start + 9 : end]
        lyrics = (
            html_to_text(lyrics).strip()
        )
        print(lyrics)
        exit(0)
    except urllib.error.HTTPError:
        if artist is artists[-1]:
            print("Lyrics not found :(", file=sys.stderr)
            exit(1)
    except Exception as e:
        if artist is artists[-1]:
            print("Unknown error: ", e, file=sys.stderr)
            exit(2)
