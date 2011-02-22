#!/bin/csh

###
## This script removes the Developer SDL package
###

setenv HOME_DIR ~

sudo -v -p "Enter administrator password to remove SDL: "

sudo rm -rf "$HOME_DIR/Library/Frameworks/SDL.framework"

# will only remove the Frameworks dir if empty (since we put it there)
sudo rmdir "$HOME_DIR/Library/Frameworks"

sudo rm -r "$HOME_DIR/Readme SDL Developer.txt"
sudo rm -r "/Developer/Documentation/SDL"
sudo rm -r "/Developer/Documentation/ManPages/man3/SDL"*
sudo rm -r "/Developer/ProjectBuilder Extras/Project Templates/Application/SDL Application"
sudo rm -r "/Developer/ProjectBuilder Extras/Project Templates/Application/SDL Cocoa Application"
sudo rm -r "/Developer/ProjectBuilder Extras/Project Templates/Application/SDL Custom Cocoa Application"
sudo rm -r "/Developer/ProjectBuilder Extras/Project Templates/Application/SDL OpenGL Application"
sudo rm -r "/Developer/ProjectBuilder Extras/Target Templates/SDL"
sudo rm -r "/Library/Receipts/SDL-devel.pkg"

# rebuild apropos database
sudo /usr/libexec/makewhatis

unsetenv HOME_DIR



