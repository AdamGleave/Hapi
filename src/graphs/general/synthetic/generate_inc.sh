# Shell script to be included. Must have GENERATOR defined

# If not installed, can replace with xargs (will run sequentially)
PARALLEL=parallel
# Print progress indicator. 
# Run 2 jobs fewer than the number of cores (machine is unusable if running on all cores).
PARALLEL_ARGS="--eta --jobs -2"

mkdir $OUTPUT
cd $OUTPUT
# Write .param files describing each graph to be generated
for param_script in $SCRIPTPATH/params/*.sh; do
  $param_script
done

# {.} drops last suffix 
ls *.param | $PARALLEL $PARALLEL_ARGS "$GENERATOR < {} > {.}" 

mkdir ${OUTPUT}_approx 
cd ${OUTPUT}_approx
$SCRIPTPATH/params_approx.sh

# {.} drops last suffix 
ls *.param | $PARALLEL $PARALLEL_ARGS "$GENERATOR < {} > {.}" 
