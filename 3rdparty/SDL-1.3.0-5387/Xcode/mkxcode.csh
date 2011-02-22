#!/bin/csh

###
## This script creates "Xcode.tar.gz" in the parent directory
###

# remove build products
rm -rf SDL/build
rm -rf SDLTest/build

# remove Finder info files
find . -name ".DS_Store" -exec rm "{}" ";"

# remove user project prefs
find . -name "*.pbxuser*" -exec rm "{}" ";"
find . -name "*.mode*" -exec rm "{}" ";"
find . -name "*.perspective*" -exec rm "{}" ";"

# create the archive
(cd .. && gnutar -zcvf Xcode.tar.gz Xcode)
