#!/bin/sh
# finish up the installation
# this script should be executed using the sudo command
# this file is copied to SDL-devel.post_install and SDL-devel.post_upgrade
# inside the .pkg bundle
echo "Running post-install script"
umask 022

USER=`basename ~`
echo "User is \"$USER\""

ROOT=/Developer/Documentation/SDL
echo "Fixing framework permissions"
find $ROOT -type d -exec chmod a+rx {} \;
find $ROOT -type f -exec chmod a+r {} \;

## We're not installing frameworks here anymore. The single
## framework should be installed to /Library/Frameworks which 
## is handled by the standard package (not developer package).
## Using the home directory here is problematic for multi-user systems too.
# echo "Moving SDL.framework to ~/Library/Frameworks"
# move SDL to its proper home, so the target stationary works
#sudo -u $USER mkdir -p ~/Library/Frameworks
#sudo -u $USER /Developer/Tools/CpMac -r $ROOT/SDL.framework ~/Library/Frameworks

## I'm not sure where this gets created and what's put in there.
rm -rf $ROOT/SDL.framework

## I think precompiled headers have changed through the revisions of Apple's gcc.
## I don't know how useful this is anymore w.r.t. Apple's newest system for precompiled headers.
## I'm removing this for now.
# echo "Precompiling Header"
# precompile header for speedier compiles
#sudo -u $USER /usr/bin/cc -precomp ~/Library/Frameworks/SDL.framework/Headers/SDL.h -o ~/Library/Frameworks/SDL.framework/Headers/SDL.p

# find the directory to store stationary in
if [ -e "/Library/Application Support/Apple/Developer Tools" ] ; then
    echo "Installing project stationary for XCode"
    PBXDIR="/Library/Application Support/Apple/Developer Tools"
else
    echo "Installing project stationary for Project Builder"
    PBXDIR="/Developer/ProjectBuilder Extras"
fi

# move stationary to its proper home
mkdir -p "$PBXDIR/Project Templates/Application"
mkdir -p "$PBXDIR/Target Templates/SDL"

cp -r "$ROOT/Project Stationary/SDL Application"              "$PBXDIR/Project Templates/Application/"
cp -r "$ROOT/Project Stationary/SDL Cocoa Application"        "$PBXDIR/Project Templates/Application/"
cp -r "$ROOT/Project Stationary/SDL Custom Cocoa Application" "$PBXDIR/Project Templates/Application/"
cp -r "$ROOT/Project Stationary/SDL OpenGL Application"       "$PBXDIR/Project Templates/Application/"
cp "$ROOT/Project Stationary/Application.trgttmpl"            "$PBXDIR/Target Templates/SDL/"

rm -rf "$ROOT/Project Stationary"

# Actually, man doesn't check this directory by default, so this isn't
# very helpful anymore.
#echo "Installing Man Pages"
## remove old man pages
#rm -rf "/Developer/Documentation/ManPages/man3/SDL"*
#
## install man pages
#mkdir -p "/Developer/Documentation/ManPages/man3"
#cp "$ROOT/docs/man3/SDL"* "/Developer/Documentation/ManPages/man3/"
#rm -rf "$ROOT/docs/man3"
#
#echo "Rebuilding Apropos Database"
## rebuild apropos database
#/usr/libexec/makewhatis

# copy README file to your home directory
sudo -u $USER cp "$ROOT/Readme SDL Developer.txt" ~/

# open up the README file
sudo -u $USER open ~/"Readme SDL Developer.txt"
