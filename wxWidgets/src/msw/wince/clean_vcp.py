'''
This script will delete dependences from *.vcp files.
After using this script, next time when you will try to save project,
you will have wait untill 'Visual Tools' will rebuild all dependencies
and this process might take HUGE amount of time

Author : Viktor Voroshylo
$Id: clean_vcp.py 24713 2003-12-04 08:59:16Z JS $
'''
__version__='$Revision: 24713 $'[11:-2]

import sys

if len(sys.argv) != 2 :
    print "Usage: %s project_file.vcp" % sys.argv[0]
    sys.exit(0)

vsp_filename = sys.argv[1]
exclude_line = 0
resultLines  = []

vsp_file       = open(vsp_filename, "r")
empty_if_start = -1

line = vsp_file.readline()
while line :
    skip_line = 0
    if exclude_line :
        if not line.endswith("\\\n") : exclude_line = 0
        skip_line = 1
    elif line.startswith("DEP_CPP_") or line.startswith("NODEP_CPP_") :
        exclude_line = 1
        skip_line = 1
    elif empty_if_start != -1 :
        if line == "!ENDIF \n" :
            resultLines    = resultLines[:empty_if_start]
            empty_if_start = -1
            skip_line      = 1
        elif line != "\n" and not line.startswith("!ELSEIF ") :
            empty_if_start = -1
    elif line.startswith("!IF ") :
        empty_if_start = len(resultLines)

    if not skip_line : 
        resultLines.append(line)
    
    line = vsp_file.readline()

open(vsp_filename, "w").write("".join(resultLines))
