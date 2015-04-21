#!/bin/bash

FILE=$1

# extract problem
grep "^p" $FILE > $FILE.problem

# sort by node ID
grep "^n" $FILE | sort -n -k 2 > $FILE.nodes

# sort by source, dst ID
grep "^a" $FILE | sort -n -k 2,3 > $FILE.arcs

# recombine
cat $FILE.problem $FILE.nodes $FILE.arcs > $FILE

# clean up
rm $FILE.{problem,nodes,arcs}
