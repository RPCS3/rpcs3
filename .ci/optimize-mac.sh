#!/bin/sh

file_path=$(find "$1/Contents/MacOS" -type f -print0 | head -n 1)

if [ -z "$file_path" ]; then
    echo "No executable file found in $1/Contents/MacOS" >&2
    exit 1
fi


target_architecture="$(lipo "$file_path" -archs)"

if [ -z "$target_architecture" ]; then
    exit 1
fi

# shellcheck disable=SC3045
find "$1" -type f -print0 | while IFS= read -r -d '' file; do
    echo Thinning "$file" -> "$target_architecture"
    lipo "$file" -thin "$target_architecture" -output "$file" || true
done
