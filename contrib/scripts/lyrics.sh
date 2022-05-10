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
    FILE=$1
    LYRICS_FILE=$2
    ARTIST=$(mid3v2 "$FILE" 2>/dev/null | grep "^TPE1=")
    TITLE=$(mid3v2 "$FILE" 2>/dev/null | grep "^TIT2=")

    ARTIST=${ARTIST#*=}
    TITLE=${TITLE#*=}

    echo "Download lyrics for \"$ARTIST - $TITLE\""
    TEXT=""
    for PLUGIN in $PLUGINS
    do
        echo "Trying $(basename "$PLUGIN")"
        TEXT=$($PLUGIN "$ARTIST" "$TITLE" 2>/dev/null)
        if [ "$?" -eq 0 ]
        then
            [[ $TEXT =~ "<!-- bot ban -->" ]] && continue # azlyrics block
            [ "$TEXT" != "" ] && break;
        fi
    done
    if [ "$TEXT" != "" ]
    then
        echo "Saving lyrics to \"$LYRICS_FILE\""
        echo "$TEXT" > "$LYRICS_FILE"
    fi
}

if [ -f "$DIRECTORY" ]
then
    LYRICS_FILE="$(dirname "$DIRECTORY")/$(basename "$DIRECTORY" .mp3).txt"
    [ -f "$LYRICS_FILE" ] || download_lyrics "$DIRECTORY" "$LYRICS_FILE"
elif [ -d "$DIRECTORY" ]
then
    while read -r FILE
    do
        LYRICS_FILE="$(dirname "$FILE")/$(basename "$FILE" .mp3).txt"
        [ -f "$LYRICS_FILE" ] || download_lyrics "$FILE" "$LYRICS_FILE"
    done < <(find "$DIRECTORY" -name \*.mp3)
fi
