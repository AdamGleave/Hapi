#!/bin/bash

# Find absolute path this script is in
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
DIMACS_TO_DOT=$SCRIPTPATH/dimacs_to_dot.py

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <graph file> <output directory>"
  exit 1
fi

DIMACS_FILE=$1
OUT_DIR=$2

# Filename, without file extension .in
BASENAME=`basename -s .in $DIMACS_FILE`
DOT_FILE=$OUT_DIR/$BASENAME.dot
python $DIMACS_TO_DOT $DIMACS_FILE > $DOT_FILE
dot -Tpng $DOT_FILE > $OUT_DIR/$BASENAME.png
