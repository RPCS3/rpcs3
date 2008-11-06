#!/bin/bash
# main settings
runexe='./pcsx2.exe'

results=results`date +'%Y%m%d%H%M'`

echo "Creating results directory ${results}/..."
mkdir $results

exec 0<test.cfg
while read line
do
  gameid=`echo $line|cut -d ' ' -f 1`
  options=`echo $line|cut -d ' ' -f 2-`
  echo "Starting test for '${gameid}' with options '${options}'..."
  # build pcsx2 commandline
  "${runexe} -log ${results}/${gameid}.log -image ${results}/${gameid} ${options} dumps/${gameid}.dump"

  for token in `cat ${results}/${gameid}.log`
  do
    lasttoken=$token
  done
  if [ "${lasttoken}" == "done" ]; then
    echo Game finished correctly.
  else
    echo Game didn't finish correctly.
  fi
done
