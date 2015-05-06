#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "usage: <number of snapshot> <input file>" 
  echo "outputs to stdout"
  exit 1
fi

SNAPSHOT_N=$1
FILE=$2

if [[ $SNAPSHOT_N -eq 1 ]]; then
  START_LINE=1
  END_LINE=`grep -n "c EOI" $FILE | head -n 1 | cut -d: -f1`
else
  LINES=`grep -n "c EOI" $FILE | head -n $SNAPSHOT_N | tail -n 2 | cut -d: -f1`
  LINES=($LINES)
  START_LINE=`expr ${LINES[0]} + 1`
  END_LINE=${LINES[1]}
fi

sed -n ${START_LINE},${END_LINE}p $FILE
