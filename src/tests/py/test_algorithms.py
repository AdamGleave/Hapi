#!/usr/bin/env python3

import os
import sys
import sh

import config.algorithms as config
import common

def runCommand(graph_path, command):
  graph = open(graph_path, "rb")
  command_res = command(_in=graph, _in_bufsize=config.BUFFER_SIZE)
  
  solutions = common.extractSolution(command_res)
  assert(len(solutions) == 1)
  return solutions[0]

def runReferenceCommand(graph_path):
  return runCommand(graph_path, config.REFERENCE_PROGRAM)

if __name__ == "__main__":
  for graph_filename in config.TEST_GRAPHS:
    print("***", graph_filename, "***")
    graph_path = os.path.join(config.DATASET_ROOT, graph_filename)
    common.runTest(graph_path, config.TEST_PROGRAMS, 
                   runReferenceCommand, runCommand)