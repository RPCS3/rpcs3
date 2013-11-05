#!/bin/sh
#
# Basic sanity check for tiffdump
#
. ${srcdir:-.}/common.sh
f_test_reader "${TIFFDUMP}" "${IMG_MINISWHITE_1C_1B}"