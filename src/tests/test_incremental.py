#!/usr/bin/env python3

import os
import sys
import sh

import config.incremental as config
import common
  
def runReferenceCommand(testcase):
  input_data = testcase.bake(_piped=True)
  snapshots = config.SNAPSHOT_CREATOR_PROGRAM.bake(input_data, _piped=True)
  command = config.SNAPSHOT_SOLVER_PROGRAM.bake(snapshots,
     config.REFERENCE_PROGRAM_PATH, *config.REFERENCE_PROGRAM_ARGUMENTS)
  # output can be BIG, Python slow. This significantly speeds up computation.
  # (Yes, this is a hack.)
  #solution = sh.grep(command(_piped=True), "^s")
  solution = command(_piped=True)
  return common.extractSolution(command_res)

def runCommand(testcase, program):
  command = program.bake(testcase(_piped=True))
  solution = sh.grep(command(_piped=True), "s ")
  return common.extractSolution(command_res)

if __name__ == "__main__":
  for (main_graph_fname, delta_fnames) in config.TEST_CASES.items():
    print("***", main_graph_fname, "***")
    
    main_graph_path = os.path.join(config.DATASET_ROOT, main_graph_fname)
    if delta_fnames:
      # we have deltas to apply
      for delta_fname in delta_fnames:
        print("- ", delta_fname)
        
        delta_path = os.path.join(config.DATASET_ROOT, delta_fname)
        common.runTest(sh.cat.bake(main_graph_path, delta_path),
                       config.TEST_PROGRAMS, runReferenceCommand, runCommand)
    else:
      # We're not concatenating everything here, but we still use the feature
      # that concatenateFiles rewinds the file for each test
      common.runTest(sh.cat.bake(main_graph_path),
                     config.TEST_PROGRAMS, runReferenceCommand, runCommand)