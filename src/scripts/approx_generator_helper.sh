PERCENTAGE=25

INDEX=$1
SEED=$2

$SIMULATOR -logtostderr -batch_step 10 -flow_scheduling_cost_model 8 \
 -machine_tmpl_file /home/srguser/adam/firmament/tests/testdata/michael.pbin \
 -solver custom -solver_timeout 60 \
 -flow_scheduling_binary ~/adam/project/build/cs2/cs2 \
 -incremental_flow=False -trace_path /mnt/data/adam/traces/small \
 -num_files_to_process 1 -max_scheduling_rounds 1 \
 -percentage $PERCENTAGE -graph_output_file ${OUTPUT_DIR}/${INDEX}.min
 -simulated_quincy_random_seed $SEED