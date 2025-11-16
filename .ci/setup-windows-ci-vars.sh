#!/bin/sh -ex

CPU_ARCH="${1:-win64}"
COMPILER="${2:-msvc}"

# These are CI specific, so we wrap them for portability
REPO_NAME="$BUILD_REPOSITORY_NAME"
REPO_BRANCH="$BUILD_SOURCEBRANCHNAME"
PR_NUMBER="$BUILD_PR_NUMBER"

# Gather explicit version number and number of commits
COMM_TAG=$(awk '/version{.*}/ { printf("%d.%d.%d", $5, $6, $7) }' ./rpcs3/rpcs3_version.cpp)
COMM_COUNT=$(git rev-list --count HEAD)
COMM_HASH=$(git rev-parse --short=8 HEAD)

# Format the above into filenames
if [ -n "$PR_NUMBER" ]; then
    AVVER="${COMM_TAG}-${COMM_HASH}"
    BUILD_RAW="rpcs3-v${AVVER}_${CPU_ARCH}_${COMPILER}"
    BUILD="${BUILD_RAW}.7z"
else
    AVVER="${COMM_TAG}-${COMM_COUNT}"
    BUILD_RAW="rpcs3-v${AVVER}-${COMM_HASH}_${CPU_ARCH}_${COMPILER}"
    BUILD="${BUILD_RAW}.7z"
fi

# BRANCH is used for experimental build warnings for pr builds, used in main_window.cpp.
# BUILD is the name of the release artifact
# BUILD_RAW is just filename
# AVVER is used for GitHub releases, it is the version number.
BRANCH="${REPO_NAME}/${REPO_BRANCH}"

# SC2129
{
    echo "BRANCH=$BRANCH"
    echo "BUILD=$BUILD"
    echo "BUILD_RAW=$BUILD_RAW"
    echo "AVVER=$AVVER"
} >> .ci/ci-vars.env
