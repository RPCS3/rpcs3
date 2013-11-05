#!/bin/sh
set -x
libtoolize --force --copy
aclocal -I ./m4
autoheader
automake --foreign --add-missing --copy
autoconf

