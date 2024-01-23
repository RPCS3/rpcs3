#!/bin/sh
# This script copies the new app over the old app and launches it.
# This is required since invalidating the code signing of an app by
# replacing it while it is running can result in the app being killed.

if [ "$#" -ne 2 ]; then
    echo "Usage: update_helper.sh <new_app> <old_app>"
    exit 1
fi
new_app="$1/"
old_app="$2/"

cp -Rf -p "$new_app" "$old_app"
open -n -a "$2" --args --updating
