#!/usr/bin/env python3

import os
import sys
import sh

import config.incremental as config
import common
  
def runReferenceCommand(testcase):
  command_res = config.SNAPSHOT_SOLVER_PROGRAM(
     config.SNAPSHOT_CREATOR_PROGRAM(_in=testcase()), # pipe output into solver
     config.REFERENCE_PROGRAM_PATH, *config.REFERENCE_PROGRAM_ARGUMENTS)
  return common.extractSolution(command_res)

def runCommand(testcase, program):
  command_res = program(_in=testcase())
  return common.extractSolution(command_res)

def concatenateFiles(f_list):
  # closure
  def helper():
    for f in f_list:
      f.seek(0)
    for f in f_list:
      for line in f:
        yield line
  return helper

if __name__ == "__main__":
  for (main_graph_fname, delta_fnames) in config.TEST_CASES.items():
    print("***", main_graph_fname, "***")
    
    main_graph_path = os.path.join(config.DATASET_ROOT, main_graph_fname)
    with open(main_graph_path, 'r') as main_graph_file:
      if delta_fnames:
        # we have deltas to apply
        for delta_fname in delta_fnames:
          print("- ", delta_fname)
          
          delta_path = os.path.join(config.DATASET_ROOT, delta_fname)
          with open(delta_path, 'r') as delta_file:
            common.runTest(concatenateFiles([main_graph_file, delta_file]),
                           config.TEST_PROGRAMS, runReferenceCommand, runCommand)
      else:
        # no deltas, just the main graph
        testcase = (os.path.join(config.DATASET_ROOT, main_graph_fname),
                    None)
        # We're not concatenating everything here, but we still use the feature
        # that concatenateFiles rewinds the file for each test
        common.runTest(concatenateFiles([main_graph_file]),
                       config.TEST_PROGRAMS, runReferenceCommand, runCommand)