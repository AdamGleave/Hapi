#!/bin/bash

OPTIONS="-v --delete -r -lpt"
EXCLUDES='--include=*.csv --exclude=*'
OPTIONS="$OPTIONS $EXCLUDES"
SRC_DIR=/home/srguser/adam/project/benchmark/
DEST_DIR=/home/adam/project/doc/data/

rsync $OPTIONS srguser@caelum-402.cl.cam.ac.uk:$SRC_DIR $DEST_DIR
