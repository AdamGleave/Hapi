#!/bin/bash

SIMULATOR=$HOME/adam/firmament/build/sim/trace-extract/google_trace_simulator
PERCENTAGE=25

SEED_FILE=$1
OUTPUT_DIR=$2
START_SEED=$3
END_SEED=$4

mkdir -p $OUTPUT_DIR

# -n: adds line numbers
let DELTA=$END_SEED-$START_SEED+1

cat -n $SEED_FILE | head -n $END_SEED | tail -n $DELTA | parallel  
for (( s=$START_SEED; s<=$END_SEED; s++ ))
do
  $SIMULATOR -logtostderr -batch_step 10 -flow_scheduling_cost_model 8 \
             -machine_tmpl_file /home/srguser/adam/firmament/tests/testdata/michael.pbin \
             -solver custom -solver_timeout 60 \
             -flow_scheduling_binary ~/adam/project/build/cs2/cs2 \
             -incremental_flow=False -trace_path /mnt/data/adam/traces/small \
             -num_files_to_process 1 -max_scheduling_rounds 1 \
             -percentage $PERCENTAGE -graph_output_file ${OUTPUT_DIR}/${s}.min
             -simulated_quincy_random_seed $SEED
done