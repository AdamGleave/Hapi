#!/bin/bash

SEED_FILE=$1
OUTPUT_DIR=$2
START_SEED=$3
END_SEED=$4

mkdir -p $OUTPUT_DIR

for (( s=$START_SEED; s<=$END_SEED; s++ ))
do
  touch $OUTPUT_DIR/${s}.min
done