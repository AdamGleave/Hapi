#!/bin/bash

SIMULATOR=$HOME/adam/firmament/build/sim/trace-extract/google_trace_simulator

OUTPUT_DIR=$1
PERCENTAGE=$2
ARGS=$3
read -ra PARSED_ARGS <<< "$ARGS"
INDEX=${PARSED_ARGS[0]}
SEED=${PARSED_ARGS[1]]}

$SIMULATOR -logtostderr -batch_step 10 -flow_scheduling_cost_model 8 \
 -machine_tmpl_file /home/srguser/adam/firmament/tests/testdata/michael.pbin \
 -solver custom -solver_timeout 60 \
 -flow_scheduling_binary ~/adam/project/build/cs2/cs2 \
 -incremental_flow=False -trace_path /mnt/data/adam/traces/small \
 -num_files_to_process 1 -max_scheduling_rounds 1 \
 -percentage $PERCENTAGE -graph_output_file ${OUTPUT_DIR}/${INDEX}.min \
 -simulated_quincy_random_seed $SEED \
