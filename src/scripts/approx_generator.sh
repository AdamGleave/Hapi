#!/bin/bash

HELPER=$HOME/adam/project/src/scripts/approx_generator_helper.sh
PARALLEL_OPTS=-j -4

SEED_FILE=$1
OUTPUT_DIR=$2
START_SEED=$3
END_SEED=$4

mkdir -p $OUTPUT_DIR

let DELTA=$END_SEED-$START_SEED+1

# -n: adds line numbers
cat -n $SEED_FILE | head -n $END_SEED | tail -n $DELTA \
                  | parallel $PARALLEL_OPTS $HELPER $OUTPUT_DIR {} 