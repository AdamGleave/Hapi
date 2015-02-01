#!/usr/bin/env python3

import os
import sys
import sh

import config_incremental as config
import common
  
def runReferenceCommand(original_path, delta_path):
  command_res = config.REFERENCE_PROGRAM(
                  config.DELTA_PATCH(original_path, delta_path)
                )
  return common.extractSolution(command_res)

def runCommand(original_path, delta_path, program):
  command_res = program(original_path, delta_path)
  return common.extractSolution(command_res)

def runTest(original_path, delta_path):
    reference_solution = runReferenceCommand(original_path, delta_path)
    if reference_solution == None:
        print("ERROR: Reference command failed on %s %s",
              original_path, delta_path)
    else:
        print("Reference solution: ", reference_solution)
    
    for name, program in config.TEST_PROGRAMS.items():
        solution = runCommand(original_path, delta_path, program)
        if solution == reference_solution:
            print("PASS - ", name)
        else:
            print("FAIL - ", name, file=sys.stderr)
            print("Expected {0}, actual {1}".
                  format(reference_solution, solution), file=sys.stderr)
            sys.exit(1)

if __name__ == "__main__":
  for (original_fname, incremental_fnames) in config.TEST_CASES.items():
    print("***", original_fname, "***")
    for incremental_fname in incremental_fnames:
      print("- ", incremental_fname)
      runTest(os.path.join(config.GRAPH_DIR, original_fname),
              os.path.join(config.GRAPH_DIR, incremental_fname))