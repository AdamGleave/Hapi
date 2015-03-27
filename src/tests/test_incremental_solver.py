#!/usr/bin/env python3

import os
import sys
import sh

import config.incremental as config
import common
  
def runReferenceCommand(testcase):
  snapshots = config.SNAPSHOT_CREATOR_PROGRAM
  command = config.SNAPSHOT_SOLVER_PROGRAM.bake(
      config.REFERENCE_PROGRAM_PATH, *config.REFERENCE_PROGRAM_ARGUMENTS)
  # output can be BIG, Python slow. This significantly speeds up computation.
  # (Yes, this is a hack.)
  filter = sh.grep.bake("^s")
  solution = filter(command(snapshots(testcase(_piped="direct"), _piped="direct"),
                            _piped="direct"))
  return common.extractSolution(solution)

def runCommand(testcase, program):
  filter = sh.grep.bake("^s")
  solution = filter(program(testcase(_piped="direct"), _piped="direct"))
  return common.extractSolution(solution)

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
                   config.SOLVER_TEST_PROGRAMS, runReferenceCommand, runCommand)
    else:
      # We're not concatenating everything here, but we still use the feature
      # that concatenateFiles rewinds the file for each test
      common.runTest(sh.cat.bake(main_graph_path),
                   config.SOLVER_TEST_PROGRAMS, runReferenceCommand, runCommand)