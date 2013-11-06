#!/bin/sh
#############################################################################
# Name:        splice
# Purpose:     Splice a marked section of regcustom.h into regex.h
# Author:      Mike Wetherell
# Copyright:   (c) 2004 Mike Wetherell
# Licence:     wxWindows licence
#############################################################################

#
# Works by greping for the marks then passing their line numbers to sed. Which
# is slighly the long way round, but allows some error checking.
#

SRC=regcustom.h
DST=regex.h
MARK1='^/\* --- begin --- \*/$'
MARK2='^/\* --- end --- \*/$'
PROG=`basename $0`
TMP=$DST.tmp

# findline(pattern, file)
# Prints the line number of the 1st line matching the pattern in the file
#
findline() {
    if ! LINE=`grep -n -- "$1" "$2"`; then
        echo "$PROG: marker '$1' not found in '$2'" >&2
        return 1
    fi
    echo $LINE | sed -n '1s/[^0-9].*//p'    # take just the line number
}

# findmarkers([out] line1, [out] line2, pattern1, pattern2, file)
# Returns (via the variables named in the 1st two parameters) the line
# numbers of the lines matching the patterns in file. Checks pattern1 comes
# before pattern2.
#
findmarkers() {
    if ! LINE1=`findline "$3" "$5"` || ! LINE2=`findline "$4" "$5"`; then
        return 1
    fi
    if [ $LINE1 -ge $LINE2 ]; then
        echo "$PROG: marker '$3' not before '$4' in '$5'" >&2
        return 1
    fi
    eval $1=$LINE1
    eval $2=$LINE2
}

# find markers
#
if findmarkers SRCLINE1 SRCLINE2 "$MARK1" "$MARK2" $SRC &&
   findmarkers DSTLINE1 DSTLINE2 "$MARK1" "$MARK2" $DST
then
    # do splice
    #
    if (sed $DSTLINE1,\$d $DST &&
        sed -n $SRCLINE1,${SRCLINE2}p $SRC &&
        sed 1,${DSTLINE2}d $DST) > $TMP
    then
        mv $TMP $DST
        exit 0
    else
        rm $TMP
    fi
fi

exit 1
