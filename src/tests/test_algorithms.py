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

def runReferenceCommand(graph_path):
  return runCommand(graph_path, config.REFERENCE_PROGRAM)

if __name__ == "__main__":
  for graph_filename in config.TEST_GRAPHS:
      print("***", graph_filename, "***")
      graph_path = os.path.join(config.GRAPH_DIR, graph_filename)
      common.runTest(graph_path, config.TEST_PROGRAMS, 
                     runReferenceCommand, runCommand)