#!/bin/sh -ex

# Resource/dependency URLs
CCACHE_URL="https://github.com/ccache/ccache/releases/download/v4.11.2/ccache-4.11.2-windows-x86_64.zip"

DEP_URLS="         \
    $CCACHE_URL"

# CI doesn't make a cache dir if it doesn't exist, so we do it manually
[ -d "$DEPS_CACHE_DIR" ] || mkdir "$DEPS_CACHE_DIR"

# Pull the llvm submodule
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 -- 3rdparty/llvm

# Git bash doesn't have rev, so here it is
rev()
{
    echo "$1" | awk '{ for(i = length($0); i != 0; --i) { a = a substr($0, i, 1); } } END { print a }'
}

# Usage: download_and_verify url checksum algo file
# Check to see if a file is already cached, and the checksum matches. If not, download it.
# Tries up to 3 times
download_and_verify()
{
    url="$1"
    correctChecksum="$2"
    algo="$3"
    fileName="$4"

    for _ in 1 2 3; do
        [ -e "$DEPS_CACHE_DIR/$fileName" ] || curl -fLo "$DEPS_CACHE_DIR/$fileName" "$url"
        fileChecksum=$("${algo}sum" "$DEPS_CACHE_DIR/$fileName" | awk '{ print $1 }')
        [ "$fileChecksum" = "$correctChecksum" ] && return 0
    done

    return 1;
}

# Some dependencies install here
[ -d "./build/lib_ext/Release-x64" ] || mkdir -p "./build/lib_ext/Release-x64"

for url in $DEP_URLS; do
    # Get the filename from the URL and remove query strings (?arg=something).
    fileName="$(rev "$(rev "$url" | cut -d'/' -f1)" | cut -d'?' -f1)"
    [ -z "$fileName" ] && echo "Unable to parse url: $url" && exit 1

    # shellcheck disable=SC1003
    case "$url" in
    *ccache*) checksum=$CCACHE_SHA; algo="sha256"; outDir="$CCACHE_BIN_DIR" ;;
    *) echo "Unknown url resource: $url"; exit 1 ;;
    esac

    download_and_verify "$url" "$checksum" "$algo" "$fileName"
    7z x -y "$DEPS_CACHE_DIR/$fileName" -aos -o"$outDir"
done

# Setup ccache tool
[ -d "$CCACHE_DIR" ] || mkdir -p "$(cygpath -u "$CCACHE_DIR")"
CCACHE_SH_DIR=$(cygpath -u "$CCACHE_BIN_DIR")
mv "$CCACHE_SH_DIR"/ccache-*/* "$CCACHE_SH_DIR"
cp "$CCACHE_SH_DIR"/ccache.exe "$CCACHE_SH_DIR"/cl.exe
