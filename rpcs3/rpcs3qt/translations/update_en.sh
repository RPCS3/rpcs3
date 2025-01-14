#!/bin/bash

SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")

OPTS="-tr-function-alias QT_TRANSLATE_NOOP+=TRANSLATE,QT_TRANSLATE_NOOP+=TRANSLATE_SV,QT_TRANSLATE_NOOP+=TRANSLATE_STR,QT_TRANSLATE_NOOP+=TRANSLATE_FS,QT_TRANSLATE_N_NOOP3+=TRANSLATE_FMT,QT_TRANSLATE_NOOP+=TRANSLATE_NOOP,translate+=TRANSLATE_PLURAL_STR,translate+=TRANSLATE_PLURAL_FS -pluralonly -no-obsolete"
SRCDIRS=$(realpath "$SCRIPTDIR/..")/\ $(realpath "$SCRIPTDIR/../../../rpcs3")/
OUTDIR=$(realpath "$SCRIPTDIR")

lupdate $SRCDIRS $OPTS -no-obsolete -source-language en_US -ts "$OUTDIR/rpcs3-qt_en-US.ts"

