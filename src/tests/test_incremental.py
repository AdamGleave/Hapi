#!/usr/bin/env python3

import os
import sys
import sh

import config_incremental as config
import common
  
def runReferenceCommand(testcase):
  original_path, delta_path = testcase
  command_res = config.REFERENCE_PROGRAM(
                  config.DELTA_PATCH(original_path, delta_path)
                )
  return common.extractSolution(command_res)

def runCommand(testcase, program):
  original_path, delta_path = testcase
  command_res = program(original_path, delta_path)
  return common.extractSolution(command_res)

if __name__ == "__main__":
  for (original_fname, incremental_fnames) in config.TEST_CASES.items():
    print("***", original_fname, "***")
    for incremental_fname in incremental_fnames:
      print("- ", incremental_fname)
      testcase = (os.path.join(config.GRAPH_DIR, original_fname),
                  os.path.join(config.GRAPH_DIR, incremental_fname))
      common.runTest(testcase, config.TEST_PROGRAMS, 
                     runReferenceCommand, runCommand) 