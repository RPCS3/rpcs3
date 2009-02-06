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

curdir=`pwd`
PLUGINDIRS="gs cdvd dev9 fw pad spu2 usb"

if [ -d "gs" ] & [ -d "cdvd" ] & [ -d "dev9" ] & [ -d "fw" ] & [ -d "pad" ] &  [ -d "spu2" ] & [ -d "usb" ]
then
echo "Plugin directories already present."
else
echo "Checking plugins out from official Pcsx2 svn..."
mkdir "temp"

if [ -d "temp" ]
then
cd "temp"
svn checkout http://pcsx2.googlecode.com/svn/trunk/plugins ./ -r 405

echo "Copying..."
for i in $PLUGINDIRS; do
		mv $i ../
        done
cd ..
rm -rf ./temp/
fi
fi

# Copy the common directory in each individual subfolder so that 
# you can compile the plugins both individually *and* from the 
# normal subdirectories.
for i in $PLUGINDIRS; do
		rm -rf $i/common
		cp -r common/ $i/
        done
	
if [ -d zerogs ]
then
echo "Importing local copy of zerogs..."
rm -rf ./gs/zerogs
cp -r zerogs ./gs/
fi

if [ -d zerospu2 ]
then
echo "Importing local copy of zerospu2..."
rm -rf ./spu2/zerospu2
cp -r zerospu2 ./spu2/
fi

if [ -d zeropad ]
then
echo "Importing local copy of zeropad..."
rm -rf ./pad/zeropad
cp -r zeropad ./pad/
fi

if [ -d CDVDiso ]
then
echo "Importing local copy of CDVDiso.."
rm -rf ./cdvd/CDVDiso
cp -r CDVDiso ./cdvd/
fi

if [ -d CDVDlinuz ]
then
echo "Importing local copy of CDVDlinuz.."
rm -rf ./cdvd/CDVDlinuz
cp -r CDVDlinuz ./cdvd/
fi

if [ -d CDVDisoEFP ]
then
echo "Importing local copy of CDVDisoEFP.."
rm -rf ./cdvd/CDVDisoEFP
cp -r CDVDisoEFP ./cdvd/
fi

if [ -d PeopsSPU2 ]
then
echo "Importing local copy of PeopsSPU2.."
rm -rf ./spu2/PeopsSPU2
cp -r PeopsSPU2 ./spu2/
fi

