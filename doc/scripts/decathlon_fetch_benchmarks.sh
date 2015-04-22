#!/bin/bash

source $HOME/bin/decathlontunnel
export RSYNC_RSH="ssh -p $PROXY_PORT"

OPTIONS="-v -z -r -lpt"
EXCLUDES='--include=*.csv --exclude=*'
OPTIONS="$OPTIONS $EXCLUDES"
SRC_DIR=/home/arg58/project/benchmark/
DEST_DIR=/home/adam/project/doc/data/

rsync $OPTIONS arg58@127.0.0.1:$SRC_DIR $DEST_DIR
