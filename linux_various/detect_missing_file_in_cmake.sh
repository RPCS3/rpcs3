#!/bin/sh

# Script default parameter
MAX_DEPTH=1
DIR=$PWD
CMAKE_FILE=CMakeLists.txt
SKIP_H=FALSE

## Help message
help()
{
    cat <<EOF
This script detects '.cpp/.h' files that are in directory (and sub) but not in the current cmake.
options:
-dir   <dirname>   : give the directory to check. Default '.'
-help              : print this help message and exit
-max_depth <depth> : how many depth to check missing file. Default 1
-skip              : skip the check of '.h' file. Disable by default
EOF

    exit 0
}

## Handle option
while [ -n "$1" ]; do
case $1 in
    -help|-h) help;shift 1;; # appel de la fonction help
    -skip|-s) SKIP_H=TRUE;shift 1;;
    -dir|-d) DIR=$2;shift 2;;
    -max_depth|-m) MAX_DEPTH=$2;shift 2;;
    --) shift;break;;
    -*) echo "ERROR: $1 option does not exists. Use -h for help";exit 1;;
    *)  break;;
esac
done


## Main sript
for file in `find $DIR -maxdepth $MAX_DEPTH -name "*.cpp"` ; do
    PATTERN=`basename $file`
    grep $PATTERN $DIR/$CMAKE_FILE > /dev/null || echo $file is missing in $CMAKE_FILE
done

if [ "${SKIP_H}" = "FALSE" ] ; then
    for file in `find $DIR -maxdepth $MAX_DEPTH -name "*.h"` ; do
        PATTERN=`basename $file`
        grep $PATTERN $DIR/$CMAKE_FILE > /dev/null || echo $file is missing in $CMAKE_FILE
    done
fi
exit
