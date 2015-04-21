#!/bin/bash

if [[ $# -ne 2 ]]; then
  echo "usage: <number of snapshot> <input file>" 
  echo "outputs to stdout"
  exit 1
fi

SNAPSHOT_N=$1
FILE=$2

LINES=`grep -n "c EOI" $FILE | head -n $SNAPSHOT_N | tail -n 2 | cut -d: -f1`
LINES=($LINES)
START_LINE=`expr ${LINES[0]} + 1`
END_LINE=${LINES[1]}

sed -n ${START_LINE},${END_LINE}p $FILE
