#!/bin/sh -e

# PCSX2 - PS2 Emulator for PCs
# Copyright (C) 2002-2011  PCSX2 Dev Team
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
VERSION=0.9.9
COPYRIGHT="PCSX2 Dev Team"
BUG_MAIL="http://code.google.com/p/pcsx2/"

GENERAL_OPTION="--sort-by-file --no-wrap --package-name=PCSX2 \
	--package-version=$VERSION --msgid-bugs-address=$BUG_MAIL"


######################################################################
# Build Input file list
######################################################################
[ ! -e pcsx2 ] && echo "pcsx2 directory not present" && return 1;
[ ! -e common ] && echo "common directory not present" && return 1;
[ ! -e locales ] && echo "locales directory not present" && return 1;

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
MAIN_POT=locales/templates/pcsx2_Main.pot
MAIN_KEY1=_
MAIN_KEY2=pxL

DEV_POT=locales/templates/pcsx2_Devel.pot
DEV_KEY1=_d
DEV_KEY2=pxDt

ICO_POT=locales/templates/pcsx2_Iconized.pot
ICO_KEY1=pxE

TER_POT=locales/templates/pcsx2_Tertiary.pot
TER_KEY1=_t
TER_KEY2=pxLt
TER_KEY3=pxEt

echo "Generate $MAIN_POT"
xgettext --keyword=$MAIN_KEY1 --keyword=$MAIN_KEY2 --keyword=$DEV_KEY1 --keyword=$DEV_KEY2 \
	--keyword=$TER_KEY1 --keyword=$TER_KEY2 $GENERAL_OPTION --copyright-holder="$COPYRIGHT"  \
	$input_files --output=$MAIN_POT
sed --in-place $MAIN_POT --expression=s/charset=CHARSET/charset=UTF-8/

echo "Generate $ICO_POT"
xgettext --keyword=$ICO_KEY1 --keyword=$TER_KEY3 $GENERAL_OPTION --copyright-holder="$COPYRIGHT" \
	$input_files --output=$ICO_POT
sed --in-place $ICO_POT --expression=s/charset=CHARSET/charset=UTF-8/

######################################################################
# Add poedit metadata
# It eases poedit for translators
######################################################################
# Metadata example
# "X-Poedit-KeywordsList: pxE_dev;pxDt\n"
# "X-Poedit-SourceCharset: utf-8\n"
# "X-Poedit-Basepath: trunk\\\n"
# "X-Poedit-SearchPath-0: pcsx2\n"
# "X-Poedit-SearchPath-1: common\n"
# Normally "...Content-Transfer-Encoding..." is the end of the header. Use it as anchor to place poedit metadata after
COMMON_META="\"X-Poedit-SourceCharset: utf-8\\\n\"\n\"X-Poedit-Basepath: trunk\\\\\\\\\\\n\"\n\"X-Poedit-SearchPath-0: pcsx2\\\n\"\n\"X-Poedit-SearchPath-1: common\\\n\""
sed --in-place $MAIN_POT --expression=s/'\"Content-Transfer-Encoding: 8bit\\n\"'/"\"Content-Transfer-Encoding: 8bit\\\n\"\n\"X-Poedit-KeywordsList: ${MAIN_KEY1};${MAIN_KEY2};${DEV_KEY1};${DEV_KEY2};${TER_KEY1};${TER_KEY2}\\\n\"\n${COMMON_META}"/
sed --in-place $ICO_POT --expression=s/'\"Content-Transfer-Encoding: 8bit\\n\"'/"\"Content-Transfer-Encoding: 8bit\\\n\"\n\"X-Poedit-KeywordsList: ${ICO_KEY1};${TER_KEY3}\\\n\"\n${COMMON_META}"/

######################################################################
# Automatically align the .po to the new pot file
######################################################################
echo "Update pcsx2_Main.po files"
for po_file in `find ./locales -iname pcsx2_Main.po`
do
    msgmerge --update $po_file $MAIN_POT
done

echo "Update pcsx2_Iconized.po files"
for po_file in `find ./locales -iname pcsx2_Iconized.po`
do
    msgmerge --update $po_file $ICO_POT
done

######################################################################
# Automatically compile po into mo file
######################################################################
echo "Compile po files, result are directly installed on bin/Langs directory"
for po_file in `find ./locales -iname "pcsx2_*.po"`
do
    mo_file=`echo $po_file | sed -e 's/locales/bin\/Langs/' | sed -e 's/\.po/.mo/'`
    mkdir -p `dirname $mo_file`
    msgfmt -o $mo_file $po_file
done
