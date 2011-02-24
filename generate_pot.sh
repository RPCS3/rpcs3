#!/bin/sh -e

# PCSX2 - PS2 Emulator for PCs
# Copyright (C) 2002-2010  PCSX2 Dev Team
#
# PCSX2 is free software: you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License as published by the Free Software Found-
# ation, either version 3 of the License, or (at your option) any later version.
#
# PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with PCSX2.
# If not, see <http://www.gnu.org/licenses/>.

# This script call xgettext utility to generate some nice pot (translation template) files

######################################################################
# Script configuration
######################################################################
VERSION=0.9.7
COPYRIGHT=PCSX2_Dev_Team #FIXME if someone is able to properly escape the space...
BUG_MAIL="http://code.google.com/p/pcsx2/"

GENERAL_OPTION="--sort-by-file --no-wrap \
    --package-name=PCSX2 --package-version=$VERSION \
    --copyright-holder=$COPYRIGHT --msgid-bugs-address=$BUG_MAIL"


######################################################################
# Build Input file list
######################################################################
[ ! -e pcsx2 ] && echo "pcsx2 directory not present" && return 1;
[ ! -e common ] && echo "common directory not present" && return 1;

# Build a list of all input files
input_files=""
for directory in pcsx2 common
do
    for type in cpp h
    do
        new_input_files=`find $directory -iname "*.${type}" -print0 | xargs -0`
        input_files="$input_files $new_input_files"
    done
done

######################################################################
# Generate the pot
######################################################################
MAIN_FILE=locales/templates/pcsx2_Main.pot
MAIN_KEY=_

DEV_FILE=locales/templates/pcsx2_Devel.pot
DEV_KEY1=pxE_dev
DEV_KEY2=pxDt

ICO_FILE=locales/templates/pcsx2_Iconized.pot
ICO_KEY=pxE

TER_FILE=locales/templates/pcsx2_Tertiary.pot
TER_KEY=pxEt

echo "Generate $MAIN_FILE"
xgettext --keyword=$MAIN_KEY $GENERAL_OPTION $input_files --output=$MAIN_FILE
sed --in-place $MAIN_FILE --expression=s/charset=CHARSET/charset=UTF-8/

echo "Generate $DEV_FILE"
xgettext --keyword=$DEV_KEY1 --keyword=$DEV_KEY2  $GENERAL_OPTION $input_files --output=$DEV_FILE
sed --in-place $DEV_FILE --expression=s/charset=CHARSET/charset=UTF-8/

echo "Generate $ICO_FILE"
xgettext --keyword=$ICO_KEY  $GENERAL_OPTION $input_files --output=$ICO_FILE
sed --in-place $ICO_FILE --expression=s/charset=CHARSET/charset=UTF-8/

echo "Generate $TER_FILE"
xgettext --keyword=$TER_KEY  $GENERAL_OPTION $input_files --output=$TER_FILE
sed --in-place $TER_FILE --expression=s/charset=CHARSET/charset=UTF-8/
