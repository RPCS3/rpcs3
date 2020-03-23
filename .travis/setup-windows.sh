#!/bin/sh -ex

# Git bash doesn't have rev, so here it is
rev()
{
    echo "$1" | awk '{ for(i = length($0); i != 0; --i) { a = a substr($0, i, 1); } } END { print a }'
}

# These are Azure specific, so we wrap them for portability
REPO_NAME="$BUILD_REPOSITORY_NAME"
REPO_BRANCH="$SYSTEM_PULLREQUEST_SOURCEBRANCH"
PR_NUMBER="$SYSTEM_PULLREQUEST_PULLREQUESTID"

# Resource/dependency URLs
# Qt mirrors can be volatile and slow, so we list 2
#QT_HOST="http://mirrors.ocf.berkeley.edu/qt/"
QT_HOST="http://qt.mirror.constant.com/"
QT_URL_VER=$(echo "$QT_VER" | sed "s/\.//g")
QT_PREFIX="online/qtsdkrepository/windows_x86/desktop/qt5_${QT_URL_VER}/qt.qt5.${QT_URL_VER}.win64_msvc2017_64/${QT_VER}-0-${QT_DATE}"
QT_SUFFIX="-Windows-Windows_10-MSVC2017-Windows-Windows_10-X86_64.7z"
QT_BASE_URL="${QT_HOST}${QT_PREFIX}qtbase${QT_SUFFIX}"
QT_WINE_URL="${QT_HOST}${QT_PREFIX}qtwinextras${QT_SUFFIX}"
QT_DECL_URL="${QT_HOST}${QT_PREFIX}qtdeclarative${QT_SUFFIX}"
QT_TOOL_URL="${QT_HOST}${QT_PREFIX}qttools${QT_SUFFIX}"
LLVMLIBS='https://github.com/RPCS3/llvm-mirror/releases/download/custom-build-win/llvmlibs_mt.7z'
GLSLANG='https://github.com/RPCS3/glslang/releases/download/custom-build-win/glslanglibs_mt.7z'
VULKAN_SDK_URL="https://sdk.lunarg.com/sdk/download/${VULKAN_VER}/windows/VulkanSDK-${VULKAN_VER}-Installer.exe"

QT_URLS="          \
    $QT_BASE_URL   \
    $QT_WINE_URL   \
    $QT_DECL_URL   \
    $QT_TOOL_URL"

# Azure pipelines doesn't make a cache dir if it doesn't exist, so we do it manually
[ -d "$CACHE_DIR" ] || mkdir "$CACHE_DIR"

# Pull all the submodules except llvm, since it is built separately and we just download that build
# Do not quote the subshell
# Note: Tried to use git submodule status, but it takes over 20 seconds
git submodule -q update --init $(awk '/path/ && !/llvm/ { print $3 }' .gitmodules)

# Download and extract the Qt packages
# Use caching to skip the download if we can
for url in $QT_URLS; do
    # Get the filename from the URL. Breaks if urls have js args, so don't do that pls
    fileName="$(rev "$(rev "$url" | cut -d'/' -f1)")"
    [ -z "$fileName" ] && echo "Unable to parse url: $url" && exit 1

    # Check to see if its already cached. If not, download it
    if [ ! -e "$CACHE_DIR/$fileName" ]; then
        curl -L -o "$CACHE_DIR/$fileName" "$url"
    fi

    7z x -y "$CACHE_DIR/$fileName" -o'C:\Qt\'
done

# Download other dependencies if they aren't cached
[ -e "$CACHE_DIR/llvmlibs.7z" ] || curl -L -o "$CACHE_DIR/llvmlibs.7z" "$LLVMLIBS"
[ -e "$CACHE_DIR/glslang.7z" ] || curl -L -o "$CACHE_DIR/glslang.7z" "$GLSLANG"
[ -e "$CACHE_DIR/vulkansdk.exe" ] || curl -L -o "$CACHE_DIR/vulkansdk.exe" "$VULKAN_SDK_URL"

vkChecksum=$(sha256sum "$CACHE_DIR/vulkansdk.exe" | awk '{ print $1 }')
[ "$vkChecksum" = "$VULKAN_SDK_SHA" ] || (echo "Bad Vulkan SDK" && exit 1)

# Extract the non-Qt dependencies from the cache into the proper build area
[ -d "./lib" ] || mkdir "./lib"
7z x -y "$CACHE_DIR"/llvmlibs.7z -aos -o"."
7z x -y "$CACHE_DIR"/glslang.7z -aos -o"./lib/Release - LLVM-x64"

# Vulkan setup needs to be run in batch environment
# Need to subshell this or else it doesn't wait
cp "$CACHE_DIR/vulkansdk.exe" .
toss=$(echo "vulkansdk.exe /S" | cmd)

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

# Export variables for later stages of the Azure pipeline
# Values done in this manner will appear as environment variables
# in later stages, but are not added to environment variables
# in *this* stage. Thank azure for that one.
# BRANCH is used for experimental build warnings for pr builds
# used in main_window.cpp. AVVER is used for GitHub releases.
BRANCH="${REPO_NAME}/${REPO_BRANCH}"
echo "##vso[task.setvariable variable=branch]$BRANCH"
echo "##vso[task.setvariable variable=build]$BUILD"
echo "##vso[task.setvariable variable=avver]$AVVER"
