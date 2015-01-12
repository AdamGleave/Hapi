#!/usr/bin/env python3

import os
import sys
import sh

SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.dirname(os.path.dirname(SCRIPT_ROOT))

GRAPH_DIR = os.path.join(BASE_DIR, "src", "tests", "graphs")
TEST_GRAPHS = ["small_graph_neg_costs.in",
               "small_graph.in",
               "graph_4m_2crs_8j.in",
               "graph_4m_2crs_10j.in",
               "stndrd2.in",
               "graph_100m_8j_100t_10p.in",
               "graph_1000m_32j_100t_10p.in",
               "google_all.in"
               ]

# Goldberg's CS2 solver
# Known-working reference implementation
REFERENCE_PROGRAM_PATH = os.path.join(BASE_DIR, "src", "cs2", "cs2")
REFERENCE_PROGRAM = sh.Command(REFERENCE_PROGRAM_PATH)

EXECUTABLE_DIR = os.path.join(BASE_DIR, "build", "bin")
TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "find_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["cost_scaling",
                          #"cycle_cancelling"
                         ]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

def runCommand(graph_path, command):
    graph = open(graph_path, "rb")
    solution = None
    for line in command(_in=graph):
        fields = line.split()
        if fields[0] == "s":
            # solution
            solution = int(fields[1])
    return solution

def runTest(graph_path):
    reference_solution = runCommand(graph_path, REFERENCE_PROGRAM)
    if reference_solution == None:
        print("ERROR: Reference command failed on %s", graph_path)
    else:
        print("Reference solution: ", reference_solution)
    
    for name, program in TEST_PROGRAMS.items():
        solution = runCommand(graph_path, program)
        if solution == reference_solution:
            print("PASS - ", name)
        else:
            print("FAIL - ", name, file=sys.stderr)
            print("Expected {0}, actual {1}".
                  format(reference_solution, solution), file=sys.stderr)

for graph_filename in TEST_GRAPHS:
    print("***", graph_filename, "***")
    runTest(os.path.join(GRAPH_DIR, graph_filename))
