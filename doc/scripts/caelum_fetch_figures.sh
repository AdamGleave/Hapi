#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

OPTIONS="-v --delete -r -lpt"
EXCLUDES="--include='**/*.pdf'"
OPTIONS="$OPTIONS $EXCLUDES"
SRC_DIR=/home/srguser/adam/project/doc/figures/
DEST_DIR=$DIR/../figures/

if [[ `hostname` != "ADAM-PETREL" ]]; then
  rsync $OPTIONS srguser@caelum-401.cl.cam.ac.uk:$SRC_DIR $DEST_DIR
fi
