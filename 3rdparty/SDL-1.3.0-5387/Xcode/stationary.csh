#!/bin/csh

###
## This script installs the stationary
###

sudo -v -p "Please enter the administrator password: "

# project templates
sudo /Developer/Tools/CpMac -r "Project Stationary/SDL Application" "/Developer/ProjectBuilder Extras/Project Templates/Application/"

sudo /Developer/Tools/CpMac -r "Project Stationary/SDL Cocoa Application" "/Developer/ProjectBuilder Extras/Project Templates/Application/"

sudo /Developer/Tools/CpMac -r "Project Stationary/SDL Custom Cocoa Application" "/Developer/ProjectBuilder Extras/Project Templates/Application/"

sudo /Developer/Tools/CpMac -r "Project Stationary/SDL OpenGL Application" "/Developer/ProjectBuilder Extras/Project Templates/Application/"


# target templates
sudo mkdir -p "/Developer/ProjectBuilder Extras/Target Templates/SDL"

sudo /Developer/Tools/CpMac -r "Project Stationary/Application.trgttmpl" "/Developer/ProjectBuilder Extras/Target Templates/SDL"



