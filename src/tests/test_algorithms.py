#!/usr/bin/env python3

import os
import sys
import sh

import config_algorithms as config
import common

def runCommand(graph_path, command):
    graph = open(graph_path, "rb")
    command_res = command(_in=graph, _in_bufsize=config.BUFFER_SIZE)
    return common.extractSolution(command_res)

def runTest(graph_path):
    reference_solution = runCommand(graph_path, config.REFERENCE_PROGRAM)
    if reference_solution == None:
        print("ERROR: Reference command failed on %s", graph_path)
    else:
        print("Reference solution: ", reference_solution)
    
    for name, program in config.TEST_PROGRAMS.items():
        solution = runCommand(graph_path, program)
        if solution == reference_solution:
            print("PASS - ", name)
        else:
            print("FAIL - ", name, file=sys.stderr)
            print("Expected {0}, actual {1}".
                  format(reference_solution, solution), file=sys.stderr)
            sys.exit(1)

if __name__ == "__main__":
  for graph_filename in config.TEST_GRAPHS:
      print("***", graph_filename, "***")
      runTest(os.path.join(config.GRAPH_DIR, graph_filename))