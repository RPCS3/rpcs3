#!/bin/sh

######################################################################
# Global Parameter
######################################################################
# Svn parameter
if [ -n "$1" ] ; then
    SVN_CO_VERSION=$1;
else
    echo "Please provide the subversion version as fisrt parameter"
    exit 1;
fi
SVN_TRUNK="http://pcsx2.googlecode.com/svn/trunk"

# Debian name of package and tarball
PKG_NAME="pcsx2.snapshot-${SVN_CO_VERSION}"
TAR_NAME="pcsx2.snapshot_${SVN_CO_VERSION}.orig.tar"

# Directory
TMP_DIR=/tmp
ROOT_DIR=${TMP_DIR}/subversion_pcsx2_${SVN_CO_VERSION}
NEW_DIR=${TMP_DIR}/$PKG_NAME


######################################################################
# Basic function
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
        if [ ! -e `basename ${file}` ] ; then
            # Versionning information are not support for a single file 
            # so you can not use svn co
            svn export --quiet ${SVN_TRUNK}/${file} -r $SVN_CO_VERSION;
        fi
    done
}

######################################################################
# Main script
######################################################################

## Download the svn repository (only the usefull things)
echo "Download pcsx2 source rev ${SVN_CO_VERSION}"
mkdir -p $ROOT_DIR;
(cd $ROOT_DIR; 
    get_svn_file CMakeLists.txt;
    get_svn_dir bin common cmake pcsx2 tools;
    get_svn_dir debian-unstable-upstream;
echo "Done")

echo "Donwload Linux compatible plugins ${SVN_CO_VERSION}"
mkdir -p $ROOT_DIR/plugins
(cd $ROOT_DIR/plugins; 
    get_svn_file plugins/CMakeLists.txt;
    # DVD
    get_svn_dir plugins/CDVDnull plugins/CDVDiso;
    # PAD
    get_svn_dir plugins/PadNull plugins/onepad;
    # AUDIO
    get_svn_dir plugins/SPU2null plugins/spu2-x plugins/zerospu2;
    # Graphics
    get_svn_dir plugins/GSnull plugins/zzogl-pg;
    # Misc
    get_svn_dir plugins/dev9null plugins/FWnull plugins/USBnull;
echo "Note: some plugins are more or less deprecated CDVDisoEFP, CDVDlinuz, Zerogs, Zeropad ...";
echo "Done")

## Installation
echo "Copy the subversion repository in a tmp directory"
# Copy the dir
rm -fr $NEW_DIR
cp -r $ROOT_DIR $NEW_DIR

echo "Remove .svn file"
find $NEW_DIR -name ".svn" -type d -exec rm -fr {} \; 2> /dev/null
echo "Remove old build system (script and autotools)"
find $NEW_DIR -name "build.sh" -exec rm -f {} \;
find $NEW_DIR -name "install-sh" -exec rm -f {} \;
find $NEW_DIR -name "depcomp" -exec rm -f {} \;
find $NEW_DIR -name "missing" -exec rm -f {} \;
find $NEW_DIR -name "aclocal.m4" -exec rm -f {} \;
find $NEW_DIR -name "configure.ac" -exec rm -f {} \;
find $NEW_DIR -name "Makefile.am" -exec rm -f {} \;
echo "Remove 3rd party directory"
find $NEW_DIR -name "3rdparty" -exec rm -fr {} \; 2> /dev/null
# echo "Remove plugins/zzogl-pg/opengl/ZeroGSShaders (some zlib source in the middle)"
# rm -fr $NEW_DIR/plugins/zzogl-pg/opengl/ZeroGSShaders
echo "Remove windows file (useless & copyright issue)"
find $NEW_DIR -iname "windows" -exec rm -fr {} \; 2> /dev/null
rm -fr "${NEW_DIR}/plugins/zzogl-pg/opengl/Win32"


## BUILD
echo "Build the tar gz file"
tar -C ${TMP_DIR} -czf ${TAR_NAME}.gz $PKG_NAME 

## Clean
rm -fr $NEW_DIR
