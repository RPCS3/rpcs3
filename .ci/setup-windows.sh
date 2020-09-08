#!/bin/sh -ex

# These are Azure specific, so we wrap them for portability
REPO_NAME="$BUILD_REPOSITORY_NAME"
REPO_BRANCH="$SYSTEM_PULLREQUEST_SOURCEBRANCH"
PR_NUMBER="$SYSTEM_PULLREQUEST_PULLREQUESTID"

# Resource/dependency URLs
# Qt mirrors can be volatile and slow, so we list 2
QT_HOST="http://mirrors.ocf.berkeley.edu/qt/"
#QT_HOST="http://qt.mirror.constant.com/"
QT_URL_VER=$(echo "$QT_VER" | sed "s/\.//g")
QT_VER_MSVC_UP=$(echo ${QT_VER_MSVC} | tr '[:lower:]' '[:upper:]')
QT_PREFIX="online/qtsdkrepository/windows_x86/desktop/qt${QT_VER_MAIN}_${QT_URL_VER}/qt.qt${QT_VER_MAIN}.${QT_URL_VER}.win64_${QT_VER_MSVC}_64/${QT_VER}-0-${QT_DATE}"
QT_SUFFIX="-Windows-Windows_10-${QT_VER_MSVC_UP}-Windows-Windows_10-X86_64.7z"
QT_BASE_URL="${QT_HOST}${QT_PREFIX}qtbase${QT_SUFFIX}"
QT_WINE_URL="${QT_HOST}${QT_PREFIX}qtwinextras${QT_SUFFIX}"
QT_DECL_URL="${QT_HOST}${QT_PREFIX}qtdeclarative${QT_SUFFIX}"
QT_TOOL_URL="${QT_HOST}${QT_PREFIX}qttools${QT_SUFFIX}"
LLVMLIBS_URL='https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win/llvmlibs_mt.7z'
GLSLANG_URL='https://github.com/RPCS3/glslang/releases/download/custom-build-win/glslanglibs_mt.7z'
VULKAN_SDK_URL="https://www.dropbox.com/s/kn005de7gsv5whe/VulkanSDK-1.2.154.1-Installer.exe"

DEP_URLS="         \
    $QT_BASE_URL   \
    $QT_WINE_URL   \
    $QT_DECL_URL   \
    $QT_TOOL_URL   \
    $LLVMLIBS_URL  \
    $GLSLANG_URL   \
    $VULKAN_SDK_URL"

# Azure pipelines doesn't make a cache dir if it doesn't exist, so we do it manually
[ -d "$CACHE_DIR" ] || mkdir "$CACHE_DIR"

# Pull all the submodules except llvm, since it is built separately and we just download that build
# Note: Tried to use git submodule status, but it takes over 20 seconds
# shellcheck disable=SC2046
git submodule -q update --init --depth 1 $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

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
        [ -e "$CACHE_DIR/$fileName" ] || curl -L -o "$CACHE_DIR/$fileName" "$url"
        fileChecksum=$("${algo}sum" "$CACHE_DIR/$fileName" | awk '{ print $1 }')
        [ "$fileChecksum" = "$correctChecksum" ] && return 0
        rm "$CACHE_DIR/$fileName"
    done

    return 1;
}

# Some dependencies install here
[ -d "./lib" ] || mkdir "./lib"

for url in $DEP_URLS; do
    # Get the filename from the URL. Breaks if urls have js args, so don't do that pls
    fileName="$(rev "$(rev "$url" | cut -d'/' -f1)")"
    [ -z "$fileName" ] && echo "Unable to parse url: $url" && exit 1

    # shellcheck disable=SC1003
    case "$url" in
    *qt*) checksum=$(curl -L "${url}.sha1"); algo="sha1"; outDir='C:\Qt\' ;;
    *llvm*) checksum=$(curl -L "${url}.sha256"); algo="sha256"; outDir="." ;;
    *glslang*) checksum=$(curl -L "${url}.sha256"); algo="sha256"; outDir="./lib/Release - LLVM-x64" ;;
    *Vulkan*)
        # Vulkan setup needs to be run in batch environment
        # Need to subshell this or else it doesn't wait
        download_and_verify "$url" "$VULKAN_SDK_SHA" "sha256" "$fileName"
        cp "$CACHE_DIR/$fileName" .
        _=$(echo "$fileName /S" | cmd)
        continue
    ;;
    *) echo "Unknown url resource: $url"; exit 1 ;;
    esac

    download_and_verify "$url" "$checksum" "$algo" "$fileName"
    7z x -y "$CACHE_DIR/$fileName" -aos -o"$outDir"
done

# Gather explicit version number and number of commits
COMM_TAG=$(awk '/version{.*}/ { printf("%d.%d.%d", $5, $6, $7) }' ./rpcs3/rpcs3_version.cpp)
COMM_COUNT=$(git rev-list --count HEAD)
COMM_HASH=$(git rev-parse --short=8 HEAD)

# Format the above into filenames
if [ -n "$PR_NUMBER" ]; then
    AVVER="${COMM_TAG}-${COMM_HASH}"
    BUILD="rpcs3-v${AVVER}_win64.7z"
else
    AVVER="${COMM_TAG}-${COMM_COUNT}"
    BUILD="rpcs3-v${AVVER}-${COMM_HASH}_win64.7z"
fi

# BRANCH is used for experimental build warnings for pr builds, used in main_window.cpp.
# BUILD is the name of the release artifact
# AVVER is used for GitHub releases, it is the version number.
BRANCH="${REPO_NAME}/${REPO_BRANCH}"
echo "BRANCH=$BRANCH" > .ci/azure-vars.env
echo "BUILD=$BUILD" >> .ci/azure-vars.env
echo "AVVER=$AVVER" >> .ci/azure-vars.env
