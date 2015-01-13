#!/bin/bash

NETGEN=generator/netgen
# If not installed, can replace with xargs (will run sequentially)
PARALLEL=parallel
# Print progress indicator. 
# Run 2 jobs fewer than the number of cores (machine is unusable if running on all cores).
PARALLEL_ARGS="--eta --jobs -2"

# Write .param files describing each graph to be generated
for param_script in params/*.sh; do
  $param_script
done

# {.} drops last suffix 
ls *.param | $PARALLEL $PARALLEL_ARGS "$NETGEN < {} > {.}" 