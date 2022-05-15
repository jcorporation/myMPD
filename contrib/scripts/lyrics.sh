#!/bin/bash
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

PLUGIN_DIR=$(dirname "$0")
PLUGINS=$(echo "$PLUGIN_DIR"/lyrics/*.py)
DIRECTORY=$1

print_usage() {
    echo "Usage: $0 <directory|file>"
    echo ""
    echo "This script uses the lyrics plugins from ncmpc:"
    for PLUGIN in $PLUGINS
    do
        echo "  $(basename "$PLUGIN")"
    done
    echo ""
    exit 1
}

[ -z "$DIRECTORY" ] && print_usage

download_lyrics() {
    local MEDIA_FILE=$1
    local LYRICS_FILE
    LYRICS_FILE="$(dirname "$FILE")/$(basename "$FILE" .mp3).txt"
    [ -s "$LYRICS_FILE" ] && return
    local ARTIST
    ARTIST=$(mid3v2 "$MEDIA_FILE" 2>/dev/null | grep "^TPE1=")
    local TITLE
    TITLE=$(mid3v2 "$MEDIA_FILE" 2>/dev/null | grep "^TIT2=")

    ARTIST=${ARTIST#*=}
    TITLE=${TITLE#*=}

    echo "Download lyrics for \"$ARTIST - $TITLE\""
    TEXT=""
    for PLUGIN in $PLUGINS
    do
        PLUGIN_NAME=$(basename "$PLUGIN")
        echo "Trying $PLUGIN_NAME"
        if TEXT=$($PLUGIN "$ARTIST" "$TITLE" 2>/dev/null)
        then
            [[ $TEXT =~ "<!-- bot ban -->" ]] && continue # azlyrics block
            if [ "$TEXT" != "" ]
            then
                {
                    echo "#--"
                    echo "#Lyrics from $PLUGIN_NAME"
                    echo "#--"
                    echo "$TEXT"
                    echo "#--"
                    echo ""
                } >> "$LYRICS_FILE"
            fi
        fi
    done
    [ -s "$LYRICS_FILE" ] || rm -f "$LYRICS_FILE"
}

if [ -f "$DIRECTORY" ]
then
    download_lyrics "$DIRECTORY"
elif [ -d "$DIRECTORY" ]
then
    while read -r FILE
    do
        download_lyrics "$FILE"
    done < <(find "$DIRECTORY" -name \*.mp3)
fi
