#!/bin/sh
# copyright (c) 2011 Gregory Hainaut
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
    -local         : download the svn repository into $HOME/.cache (not deleted by the script)
EOF

    exit 0
}

# Default value
SVN_CO_VERSION=0;
BRANCH="trunk"
LOCAL=0
while [ -n "$1" ]; do
case $1 in
    -help|-h) help;shift 1;;
    -rev|-r) SVN_CO_VERSION=$2; shift 2;;
    -branch|-b) BRANCH=$2; shift 1;;
    -local|-l) LOCAL=1;shift 1;;
    --) shift;break;;
    -*) echo "ERROR: $1 option does not exists. Use -h for help";exit 1;;
    *)  break;;
esac
done

if [ "$SVN_CO_VERSION" = "0" ] ; then
    help
fi

if [ "$BRANCH" = "trunk" ] ; then
    SVN_TRUNK="http://pcsx2.googlecode.com/svn/trunk"
else
    SVN_TRUNK="http://pcsx2.googlecode.com/svn/branches/$BRANCH"
fi

# Debian name of package and tarball
PKG_NAME="pcsx2.snapshot-${SVN_CO_VERSION}"
TAR_NAME="pcsx2.snapshot_${SVN_CO_VERSION}.orig.tar"

# Directory
TMP_DIR=/tmp
if [ "$LOCAL" = 1 ] ; then
    ROOT_DIR=${HOME}/.cache/svn_pcsx2__${BRANCH}
else
    ROOT_DIR=${TMP_DIR}/subversion_pcsx2_${SVN_CO_VERSION}
fi
NEW_DIR=${TMP_DIR}/$PKG_NAME


######################################################################
# Basic functions
######################################################################
get_svn_dir()
{
    for directory in $* ; do
        # Print the directory without /
        echo "  $directory" | sed -e 's/\//\ /g'
        if [ -e `basename ${directory}` ] ; then
            # Directory already exist so only update
            svn up --quiet ${SVN_TRUNK}/${directory} -r $SVN_CO_VERSION;
        else 
            svn co --quiet ${SVN_TRUNK}/${directory} -r $SVN_CO_VERSION;
        fi
    done
}

get_svn_file()
{
    for file in $* ; do
        # Versioning information is not supported for a single file
        # therefore you can't use svn co
        svn export --quiet ${SVN_TRUNK}/${file} -r $SVN_CO_VERSION;
    done
}

######################################################################
# Main script
######################################################################

## Download the svn repository (only the useful things)
echo "Downloading pcsx2 source revision ${SVN_CO_VERSION}"
mkdir -p $ROOT_DIR;
(cd $ROOT_DIR; 
    get_svn_file CMakeLists.txt;
    get_svn_dir common cmake locales pcsx2 tools;
    get_svn_dir debian-unstable-upstream;
echo "Done")

# separate bin to avoid downloading the .mo file
mkdir -p $ROOT_DIR/bin;
(cd $ROOT_DIR/bin;
    get_svn_file bin/GameIndex.dbf;
    get_svn_dir  bin/docs;)

echo "Downloading Linux compatible plugins revision ${SVN_CO_VERSION}"
# Note: Other plugins exist but they are not 100% copyright free.
mkdir -p $ROOT_DIR/plugins
(cd $ROOT_DIR/plugins; 
    get_svn_file plugins/CMakeLists.txt;
    get_svn_dir plugins/CDVDnull;
    # Potential copyrigh issue. Optional anyway
    # get_svn_dir plugins/CDVDnull plugins/CDVDiso;
    get_svn_dir plugins/PadNull plugins/onepad;
    get_svn_dir plugins/SPU2null plugins/spu2-x;
    get_svn_dir plugins/GSnull plugins/zzogl-pg plugins/GSdx;
    get_svn_dir plugins/dev9null plugins/FWnull plugins/USBnull;
echo "Note: some plugins are more or less deprecated CDVDisoEFP, CDVDlinuz, Zerogs, Zeropad ...";
echo "Done")

## Download the internal sdl 1.3 for gsdx
echo "Downloading 3rdpary SDL 1.3 (need by gsdx) revision ${SVN_CO_VERSION}"
mkdir -p $ROOT_DIR/3rdparty
(cd $ROOT_DIR/3rdparty/;
    get_svn_file 3rdparty/CMakeLists.txt;
    get_svn_dir 3rdparty/SDL-1.3.0-5387;)
echo "Done"
    

## Installation
echo "Copy the subversion repository to a temporary directory"
# Copy the dir
rm -fr $NEW_DIR
cp -r $ROOT_DIR $NEW_DIR

echo "Remove .svn directories"
find $NEW_DIR -name ".svn" -type d -exec rm -fr {} \; 2> /dev/null
echo "Remove old build system (scripts and autotools)"
find $NEW_DIR -name "build.sh" -exec rm -f {} \;
find $NEW_DIR -name "install-sh" -exec rm -f {} \;
find $NEW_DIR -name "depcomp" -exec rm -f {} \;
find $NEW_DIR -name "missing" -exec rm -f {} \;
find $NEW_DIR -name "aclocal.m4" -exec rm -f {} \;
find $NEW_DIR -name "configure.ac" -exec rm -f {} \;
find $NEW_DIR -name "Makefile.am" -exec rm -f {} \;
echo "Remove windows file (useless & copyright issue)"
find $NEW_DIR -iname "windows" -type d -exec rm -fr {} \; 2> /dev/null
find $NEW_DIR -name "Win32" -type d -exec rm -fr {} \; 2> /dev/null
rm -fr "${NEW_DIR}/plugins/zzogl-pg/opengl/Win32"
rm -fr "${NEW_DIR}/tools/GSDumpGUI"
rm -fr "${NEW_DIR}/common/vsprops"
echo "Remove useless files (copyright issues)"
rm -fr "${NEW_DIR}/pcsx2/3rdparty" # useless link which annoy me
rm -fr "${NEW_DIR}/plugins/zzogl-pg/opengl/ZeroGSShaders"
rm -fr "${NEW_DIR}/common/src/Utilities/x86/MemcpyFast.cpp"
rm -fr "${NEW_DIR}/plugins/GSdx/baseclasses"
rm -fr "${NEW_DIR}/plugins/GSdx/vtune"

## BUILD
echo "Build the tar.gz file"
tar -C $TMP_DIR -czf ${TAR_NAME}.gz $PKG_NAME 

## Clean
rm -fr $NEW_DIR
if [ "$LOCAL" = 0 ] ; then
    rm -fr $ROOT_DIR
fi
