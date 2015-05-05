#!/bin/bash

OPTIONS="-v --delete -r -lpt"
EXCLUDES='--include=*.pdf --exclude=*'
OPTIONS="$OPTIONS $EXCLUDES"
SRC_DIR=/home/srguser/adam/project/doc/figures/
DEST_DIR=/home/adam/project/doc/figures/

rsync $OPTIONS srguser@caelum-401.cl.cam.ac.uk:$SRC_DIR $DEST_DIR
