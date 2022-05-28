#!/bin/bash
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

URI="http://webservice.fanart.tv/v3/music/"
OUT_PATH="/var/lib/mympd/pics/Artist"
DIM="500"

for DEP in convert wget mid3v2
do
    if [ "$(command -v "$DEP")" = "" ]
    then
        echo "Missing dependency: $DEP"
        exit 1
    fi
done

print_usage() {
    echo "Usage: $0 <mediafile>"
    echo ""
    echo "Mediafile must have the id3v2 tags \"MusicBrainz Artist Id\" and \"TPE1\"."
    echo "Set fanart.tv API Key with: export API_KEY=\"<key>\""
    echo ""
    exit 1
}

get_image_size() {
    identify -format "%wx%h" "$1"
}

resize_image() {
    RESIZE_FILE="$1"
    [ -s "$RESIZE_FILE" ] || return 1
    TO_DIM="$2"
    TO_SIZE="${TO_DIM}x${TO_DIM}"
    # get actual size
    CUR_SIZE=$(get_image_size "$RESIZE_FILE")
    if [ "${CUR_SIZE}" != "${TO_SIZE}" ]
    then
        echo "Resizing $RESIZE_FILE from $CUR_SIZE to $TO_SIZE"
        if convert "$RESIZE_FILE" -resize "$TO_SIZE^" -gravity center -extent "$TO_SIZE" "$RESIZE_FILE.resize"
        then
            mv "$RESIZE_FILE.resize" "$RESIZE_FILE"
        else
            echo "Error resizing $RESIZE_FILE"
            rm -f "$RESIZE_FILE.resize"
            return 1
        fi
    fi
    return 0
}

download() {
    local MBID=$1
    local NAME=$2
    echo "Downloading artistart for \"$NAME\""
    THUMBS=$(wget -q "${URI}${MBID}?api_key=${API_KEY}" -O - | jq ".artistthumb")
    if [ "$THUMBS" == "null" ]
    then
        echo "Not found."
        return 1
    fi
    URL=$(jq -r ".[] | .url" <<< "$THUMBS" | head -1)
    if [ "$URL" != "" ] && wget -q "$URL" -O "$OUT_PATH/$NAME.tmp"
    then
        convert "$OUT_PATH/$NAME.tmp" "$OUT_PATH/$NAME.webp"
        rm -f "$OUT_PATH/$NAME.tmp"
        resize_image "$OUT_PATH/$NAME.webp" "$DIM"
    else
        rm -f "$OUT_PATH/$NAME.tmp"
        echo "Not found."
    fi
}

MEDIAFILE=$1
if [ -z "$MEDIAFILE" ] || [ -z "$API_KEY" ]
then
    print_usage
fi

if [ ! -f "$MEDIAFILE" ]
then
    echo "$MEDIAFILE not found."
    exit 1
fi

ID3_MBID=$(mid3v2 "$MEDIAFILE" 2>/dev/null | grep "^TXXX=MusicBrainz Artist Id=" | cut -d= -f3)
ID3_NAME=$(mid3v2 "$MEDIAFILE" 2>/dev/null | grep "^TPE1=" | cut -d= -f2)
if [ -z "$ID3_MBID" ] || [ -z "$ID3_NAME" ]
then
    echo "\"$MEDIAFILE\": MusicBrainz Artist Id or TPE1 id3v2 tag not found."
fi

mkdir -p "$OUT_PATH"
if [ -s "$OUT_PATH/$ID3_NAME.webp" ]
then
    echo "Artistart for \"$ID3_NAME\" already exists."
else
    download "$ID3_MBID" "$ID3_NAME"
fi
