#!/bin/bash

# Find absolute path this script is in
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
DIMACS_TO_DOT=$SCRIPTPATH/dimacs_to_dot.py

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <graph file> [flow file] <output directory>"
  exit 1
fi

DIMACS_FILE=$1

# Filename, without file extension .in
BASENAME=`basename -s .in $DIMACS_FILE`

if [[ $# -eq 3 ]]; then
  FLOW_FILE=$2
  OUT_DIR=$3
else
  OUT_DIR=$2
fi
DOT_FILE=$OUT_DIR/$BASENAME.dot
python $DIMACS_TO_DOT $DIMACS_FILE $FLOW_FILE > $DOT_FILE

dot -Tpng $DOT_FILE > $OUT_DIR/$BASENAME.png
