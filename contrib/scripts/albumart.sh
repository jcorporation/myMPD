#!/bin/bash
#
# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

# Variables for customization
ALBUMART_REGEX="cover|folder"    # filenames to process
NEW_ALBUMART_NAME="cover.webp"   # the new name for the albumart
NEW_ALBUMART_DIM="500"           # albumart width/height in px
THUMB_NAME="cover-sm.webp"       # the name for the albumart thumbnail
THUMB_DIM="175"                  # thumbnail width/height in px
RETAIN_ORIGINAL="0"              # set it to "1" to retain the original image, "0" to remove
FULL_SIZE_NAME="cover-xl.webp"   # name for the unresized albumart, set it to "" to do not keep unresized albumart

# do not edit after this line

NEW_ALBUMART_SIZE="${NEW_ALBUMART_DIM}x${NEW_ALBUMART_DIM}"
THUMB_SIZE="${THUMB_DIM}x${THUMB_DIM}"

TASK="$1"
MUSIC_DIR="$2"

for DEP in convert wget mid3v2
do
    if [ "$(command -v "$DEP")" = "" ]
    then
        echo "Missing dependency: $DEP"
        exit 1
    fi
done

print_usage() {
    echo "Usage: $0 <check|download|missing|resize|> <music_directory>"
    echo ""
    echo "Albumart download uses coverartarchive.org (works only with the \"MusicBrainz Album Id\" ID3v2 tag)"
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

resize_albumart() {
    while read -r IMAGE
    do
        IMAGE_PATH=$(dirname "$IMAGE")
        IMAGE_NAME=$(basename "$IMAGE")
        NEW_IMAGE_FULL_PATH="$IMAGE_PATH/$NEW_ALBUMART_NAME"
        NEW_THUMB_FULL_PATH="$IMAGE_PATH/$THUMB_NAME"

        [ "$IMAGE_NAME" = "$THUMB_NAME" ] && continue      # skip thumbnails
        [ "$IMAGE_NAME" = "$FULL_SIZE_NAME" ] && continue  # skip full size albumart

        # convert old images to webp format
        if [ "$IMAGE_NAME" != "$NEW_ALBUMART_NAME" ]
        then
            if [ ! -s "$NEW_IMAGE_FULL_PATH" ]
            then
                echo "Converting $IMAGE to webp"
                if convert "$IMAGE" "$NEW_IMAGE_FULL_PATH"
                then
                    [ "$RETAIN_ORIGINAL" -eq 0 ] && rm -f "$IMAGE"
                    IMAGE="$NEW_IMAGE_FULL_PATH"
                else
                    echo "Error converting image \"$IMAGE\""
                    continue
                fi
            fi
        fi
        # resize albumart only if it is bigger
        CUR_SIZE=$(get_image_size "$IMAGE")
        CUR_WIDTH=${CUR_SIZE%%x*}
        CUR_HEIGHT=${CUR_SIZE#*x}
        if [ "$CUR_WIDTH" -gt "$NEW_ALBUMART_DIM" ] || [ "$CUR_HEIGHT" -gt "$NEW_ALBUMART_DIM" ]
        then
            if [ -n "$FULL_SIZE_NAME" ]
            then
                if ! cp "$IMAGE" "$IMAGE_PATH/$FULL_SIZE_NAME"
                then
                    continue
                fi
            fi
            resize_image "$IMAGE" "$NEW_ALBUMART_DIM"
            # remove thumbnail - it seems the cover was updated
            rm -f "$NEW_THUMB_FULL_PATH"
        fi
        # check for thumbnail
        if [ ! -s "$NEW_THUMB_FULL_PATH" ]
        then
            if cp "$IMAGE" "$NEW_THUMB_FULL_PATH"
            then
                resize_image "$NEW_THUMB_FULL_PATH" "$THUMB_DIM"
            fi
        fi
    done < <(find "$MUSIC_DIR" -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png"  -o -iname "*.webp" \) | grep -i -E "$ALBUMART_REGEX")
}

check_albumart_sizes() {
    while read -r IMAGE
    do
        IMAGE_NAME=$(basename "$IMAGE")
        SIZE=$(get_image_size "$IMAGE")
        case "$IMAGE_NAME" in
            "$NEW_ALBUMART_NAME")
                [ "$SIZE" != "$NEW_ALBUMART_SIZE" ] && echo "$IMAGE size is $SIZE, should be $NEW_ALBUMART_SIZE"
            ;;
            "$THUMB_NAME")
                [ "$SIZE" != "$THUMB_SIZE" ] && echo "$IMAGE size is $SIZE, should be $THUMB_SIZE"
            ;;
            "$FULL_SIZE_NAME")
                WIDTH=${SIZE%%x*}
                HEIGHT=${SIZE#*x}
                [ "$WIDTH" -lt "$NEW_ALBUMART_DIM" ] && echo "$IMAGE width is $WIDTH, should be ge $NEW_ALBUMART_DIM"
                [ "$HEIGHT" -lt "$NEW_ALBUMART_DIM" ] && echo "$IMAGE height is $HEIGHT, should be ge $NEW_ALBUMART_DIM"
            ;;
        esac
    done < <(find "$MUSIC_DIR" -type f \( -name "$THUMB_NAME" -o -name "$NEW_ALBUMART_NAME" -o -name "$FULL_SIZE_NAME" \))
}

download_albumart() {
    while read -r DIR
    do
        IMAGE_FULL_PATH="${DIR}/${NEW_ALBUMART_NAME}"
        if [ -s "$IMAGE_FULL_PATH" ]
        then
            SIZE=$(get_image_size "$IMAGE_FULL_PATH")
            WIDTH=${SIZE%%x*}
            HEIGHT=${SIZE#*x}
            if [ "$WIDTH" -ge "$NEW_ALBUMART_DIM" ] && [ "$HEIGHT" -ge "$NEW_ALBUMART_DIM" ]
            then
                # albumart already existent and big enough
                continue
            fi
        fi
        FIRST=$(find "$DIR" -maxdepth 1 -type f -name \*.mp3 | head -1)
        [ "$FIRST" = "" ] && continue
        MBID=$(mid3v2 "$FIRST" 2>/dev/null | grep "^TXXX=MusicBrainz Album Id=" | cut -d= -f3)
        if [ "$MBID" = "" ]
        then
            echo "No MBID found for \"$FIRST\""
            continue
        fi
        echo -n "Download albumart for $DIR: "
        if wget -q "https://coverartarchive.org/release/$MBID/front" -O "$IMAGE_FULL_PATH.tmp"
        then
            convert "$IMAGE_FULL_PATH.tmp" "$IMAGE_FULL_PATH.tmp.webp"
            rm -f "$IMAGE_FULL_PATH.tmp"
            mv "$IMAGE_FULL_PATH.tmp.webp" "$IMAGE_FULL_PATH"
            SIZE=$(get_image_size "$IMAGE_FULL_PATH")
            echo "$SIZE"
        else
            rm -f "$IMAGE_FULL_PATH.tmp"
            echo "failed"
        fi
    done < <(find "$MUSIC_DIR" -type d)
}

check_albumart_missing() {
    while read -r DIR
    do
        MUSIC_FILE=$(find "$DIR" -maxdepth 1 -type f -iname \*.mp3 | head -1)
        if [ "$MUSIC_FILE" != "" ]
        then
            [ -f "$DIR/${NEW_ALBUMART_NAME}" ] || echo "Missing coverimage: $DIR"
            [ -f "$DIR/${THUMB_NAME}" ] || echo "Missing thumbnail: $DIR"
        fi
    done < <(find "$MUSIC_DIR" -type d)
}

[ -z "$MUSIC_DIR" ] && print_usage

case "$TASK" in
    check)
        check_albumart_sizes "$MUSIC_DIR"
        ;;
    download)
        download_albumart "$MUSIC_DIR"
        ;;
    missing)
        check_albumart_missing "$MUSIC_DIR"
        ;;
    resize)
        resize_albumart "$MUSIC_DIR"
        ;;
    *)
        print_usage
        ;;
esac
