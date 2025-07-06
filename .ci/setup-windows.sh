#!/bin/sh -ex

# Resource/dependency URLs
# Qt mirrors can be volatile and slow, so we list 2
#QT_HOST="http://mirrors.ocf.berkeley.edu/qt/"
QT_HOST="http://qt.mirror.constant.com/"
QT_URL_VER=$(echo "$QT_VER" | sed "s/\.//g")
QT_VER_MSVC_UP=$(echo "${QT_VER_MSVC}" | tr '[:lower:]' '[:upper:]')
QT_PREFIX="online/qtsdkrepository/windows_x86/desktop/qt${QT_VER_MAIN}_${QT_URL_VER}/qt${QT_VER_MAIN}_${QT_URL_VER}/qt.qt${QT_VER_MAIN}.${QT_URL_VER}."
QT_PREFIX_2="win64_${QT_VER_MSVC}_64/${QT_VER}-0-${QT_DATE}"
QT_SUFFIX="-Windows-Windows_11_23H2-${QT_VER_MSVC_UP}-Windows-Windows_11_23H2-X86_64.7z"
QT_BASE_URL="${QT_HOST}${QT_PREFIX}${QT_PREFIX_2}qtbase${QT_SUFFIX}"
QT_DECL_URL="${QT_HOST}${QT_PREFIX}${QT_PREFIX_2}qtdeclarative${QT_SUFFIX}"
QT_TOOL_URL="${QT_HOST}${QT_PREFIX}${QT_PREFIX_2}qttools${QT_SUFFIX}"
QT_MM_URL="${QT_HOST}${QT_PREFIX}addons.qtmultimedia.${QT_PREFIX_2}qtmultimedia${QT_SUFFIX}"
QT_SVG_URL="${QT_HOST}${QT_PREFIX}${QT_PREFIX_2}qtsvg${QT_SUFFIX}"
LLVMLIBS_URL="https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win-${LLVM_VER}/llvmlibs_mt.7z"
VULKAN_SDK_URL="https://www.dropbox.com/scl/fi/sjjh0fc4ld281pjbl2xzu/VulkanSDK-${VULKAN_VER}-Installer.exe?rlkey=f6wzc0lvms5vwkt2z3qabfv9d&dl=1"
CCACHE_URL="https://github.com/ccache/ccache/releases/download/v4.11.2/ccache-4.11.2-windows-x86_64.zip"

DEP_URLS="         \
    $QT_BASE_URL   \
    $QT_DECL_URL   \
    $QT_TOOL_URL   \
    $QT_MM_URL     \
    $QT_SVG_URL    \
    $LLVMLIBS_URL  \
    $VULKAN_SDK_URL\
    $CCACHE_URL"

# CI doesn't make a cache dir if it doesn't exist, so we do it manually
[ -d "$DEPS_CACHE_DIR" ] || mkdir "$DEPS_CACHE_DIR"

# Pull all the submodules except llvm, since it is built separately and we just download that build
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init --depth=1 --jobs=8 $(awk '/path/ && !/FAudio/ && !/llvm/ && !/feralinteractive/ { print $3 }' .gitmodules)

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
    *qt*) checksum=$(curl -fL "${url}.sha1"); algo="sha1"; outDir="$QTDIR/" ;;
    *llvm*) checksum=$(curl -fL "${url}.sha256"); algo="sha256"; outDir="./build/lib_ext/Release-x64" ;;
    *ccache*) checksum=$CCACHE_SHA; algo="sha256"; outDir="$CCACHE_BIN_DIR" ;;
    *Vulkan*)
        # Vulkan setup needs to be run in batch environment
        # Need to subshell this or else it doesn't wait
        download_and_verify "$url" "$VULKAN_SDK_SHA" "sha256" "$fileName"
        cp "$DEPS_CACHE_DIR/$fileName" .
        _=$(echo "$fileName --accept-licenses --default-answer --confirm-command install" | cmd)
        continue
    ;;
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
