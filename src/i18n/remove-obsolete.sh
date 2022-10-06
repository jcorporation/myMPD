#!/bin/sh
#
# Removes a phrase from all translation files

# save script path
SCRIPTPATH=$(dirname "$(realpath "$0")")

OBSOLETE=$1

if [ -z "$OBSOLETE" ]
then
    echo "Usage: $0 <phrase to remove>"
    exit 1
fi

cd "$SCRIPTPATH" || exit 1

for F in json/*-*.json
do
    cp "$F" "$F.tmp"
    grep -v -P "^\s+\"$OBSOLETE\":\s+\"" "$F.tmp" > "$F"
    rm -f "$F.tmp"
done
