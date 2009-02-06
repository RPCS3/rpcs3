#!/bin/sh
# Convenience script for obtaining plugins,
# Use at your own risk. Assumes that it is being run in the plugins directory.

#Running this script should do the following:
#1) Check to see if the 6 needed plugin folders exist in the current directory.
#2) If they aren't. create a temp directory, download the plugins to it from
#    the official pcsx2 svn, move them into the plugin directory, and then delete
#    the temp directory.

#3) Check for copies of certain plugins in the current directory, and copy them to
#     the right place if they exist, assuming that they are modified copies of those plugins.

cd plugins
sh fetch.sh
cd ..