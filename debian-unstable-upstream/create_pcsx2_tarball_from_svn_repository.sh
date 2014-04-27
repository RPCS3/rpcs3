#!/bin/sh
# copyright (c) 2011-2014 Gregory Hainaut
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.



######################################################################
# Global Parameters
######################################################################
help()
{
    cat <<EOF
    Help:
    -rev <rev>     : revision number
    -branch <name> : branch name, take trunk otherwise
EOF

    exit 0
}

# Default value
GIT_SHA1=0;
BRANCH="master"
while [ -n "$1" ]; do
case $1 in
    -help|-h   ) help;shift 1;;
    -rev|-r    ) GIT_SHA1=$2; shift 2;;
    -branch|-b ) BRANCH=$2; shift 2;;
    --) shift;break;;
    -*) echo "ERROR: $1 option does not exists. Use -h for help";exit 1;;
    *)  break;;
esac
done

# Directory
TMP_DIR=/tmp/pcsx2_git
mkdir -p $TMP_DIR

REMOTE_REPO="https://github.com/PCSX2/pcsx2.git"
LOCAL_REPO="$TMP_DIR/pcsx2"


######################################################################
# Basic functions
######################################################################
date=
version=
release=
get_pcsx2_version()
{
    local major=`grep -o "VersionHi.*" $LOCAL_REPO/pcsx2/SysForwardDefs.h | grep -o "[0-9]*"`
    local mid=`grep -o "VersionMid.*" $LOCAL_REPO/pcsx2/SysForwardDefs.h | grep -o "[0-9]*"`
    local minor=`grep -o "VersionLo.*" $LOCAL_REPO/pcsx2/SysForwardDefs.h | grep -o "[0-9]*"`
    release=`grep -o "isReleaseVersion.*" $LOCAL_REPO/pcsx2/SysForwardDefs.h | grep -o "[0-9]*"`
    version="$major.$mid.$minor"
}

get_git_version()
{
    date=`git -C $LOCAL_REPO show -s --format=%ci HEAD | sed -e 's/[\:\-]//g' -e 's/ /./' -e 's/ .*//'`
}

download_orig()
{
    (cd $TMP_DIR && git clone --branch $1 $REMOTE_REPO pcsx2)
    if [ "$SVN_CO_VERSION" = "1" ] ; then
        (cd $TMP_DIR/pcsx2 && git checkout $GIT_SHA1)
    fi
}

remove_3rdparty()
{
    echo "Remove 3rdparty code"
    rm -fr $LOCAL_REPO/3rdparty
    rm -fr $LOCAL_REPO/fps2bios
    rm -fr $LOCAL_REPO/tools
}

remove_not_yet_free_plugin()
{
    echo "Remove non free plugins"
    # remove also deprecated plugins
    for plugin in CDVDiso CDVDisoEFP CDVDlinuz CDVDolio CDVDpeops dev9ghzdrk PeopsSPU2 SSSPSXPAD USBqemu xpad zerogs zerospu2 zzogl-pg-cg
    do
        rm -fr $LOCAL_REPO/plugins/$plugin
    done
}

remove_remaining_non_free_file()
{
    echo "Remove remaining non free file. TODO UPSTREAM"
    rm -fr $LOCAL_REPO/plugins/GSdx/baseclasses
    rm -f  $LOCAL_REPO/plugins/zzogl-pg/opengl/Win32/aviUtil.h
    rm -f  $LOCAL_REPO/plugins/spu2-x/src/Windows/Hyperlinks.h
    rm -f  $LOCAL_REPO/plugins/spu2-x/src/Windows/Hyperlinks.cpp
    rm -f  $LOCAL_REPO/common/src/Utilities/x86/MemcpyFast.cpp
    rm -f  $LOCAL_REPO/common/include/comptr.h
}
remove_dot_git()
{
    # To save 66% of the package size
    rm -fr  $LOCAL_REPO/.git
}

######################################################################
# Main script
######################################################################
download_orig $BRANCH
remove_3rdparty
remove_not_yet_free_plugin
remove_remaining_non_free_file

get_git_version
get_pcsx2_version

# must be done after getting the git version
remove_dot_git

# Debian name of package and tarball
if [ $release -eq 1 ]
then
    PKG_NAME="pcsx2-${version}"
    TAR_NAME="pcsx2_${version}.orig.tar"
else
    PKG_NAME="pcsx2.snapshot-${version}~git${date}"
    TAR_NAME="pcsx2.snapshot_${version}~git${date}.orig.tar"
fi


NEW_DIR=${TMP_DIR}/$PKG_NAME
rm -fr $NEW_DIR
mv $LOCAL_REPO $NEW_DIR

echo "Build the tar.gz file"
tar -C $TMP_DIR -cJf ${TAR_NAME}.xz $PKG_NAME

## Clean
rm -fr $TMP_DIR

exit 0
