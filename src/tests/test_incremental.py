#!/usr/bin/env python3

import os
import sys
import sh

SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.dirname(os.path.dirname(SCRIPT_ROOT))

GRAPH_DIR = os.path.join(BASE_DIR, "src", "graphs", "clusters", "synthetic",
                         "firmament", "incremental")
TEST_CASES = {"graph_4m_2crs_6j.in": 
                ["graph_4m_2crs_6j_add_arc_equiv_core.in",
                 "graph_4m_2crs_6j_add_arc_pref_core.in",
                 "graph_4m_2crs_6j_add_core.in",
                 "graph_4m_2crs_6j_add_task.in",
                 "graph_4m_2crs_6j_add_task_noprempt.in",
                 "graph_4m_2crs_6j_chg_pref_core.in",
                 "graph_4m_2crs_6j_chg_task_sched.in",
                 "graph_4m_2crs_6j_rem_arc_pref_core.in",
                 "graph_4m_2crs_6j_rem_core_without_task.in",
                 "graph_4m_2crs_6j_rem_core_with_task.in",
                 "graph_4m_2crs_6j_rem_machine.in",
                 "graph_4m_2crs_6j_rem_sched_task.in"],
              "graph_4m_2crs_10j.in":
                ["graph_4m_2crs_10j_add_arc_equiv_core.in",
                 "graph_4m_2crs_10j_add_arc_pref_core.in",
                 "graph_4m_2crs_10j_add_core.in",
                 "graph_4m_2crs_10j_add_task.in",
                 "graph_4m_2crs_10j_add_task_nopremt.in",
                 "graph_4m_2crs_10j_chg_pref_core.in",
                 "graph_4m_2crs_10j_chg_task_sched.in",
                 "graph_4m_2crs_10j_rem_arc_pref_core.in",
                 "graph_4m_2crs_10j_rem_core_with_task.in",
                 "graph_4m_2crs_10j_rem_machine.in",
                 "graph_4m_2crs_10j_rem_sched_task.in"],
              }

# Goldberg's CS2 solver
# Known-working reference implementation
REFERENCE_PROGRAM_PATH = os.path.join(BASE_DIR, "build", "cs2", "cs2")
REFERENCE_PROGRAM = sh.Command(REFERENCE_PROGRAM_PATH)

# Program to merge incremental deltas with full graph
DELTA_PATCH_PATH = os.path.join(BASE_DIR, "build", "bin", "dimacs_incremental")
DELTA_PATCH = sh.Command(DELTA_PATCH_PATH)

EXECUTABLE_DIR = os.path.join(BASE_DIR, "build", "bin")
TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["incremental",
                         ]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

def extractSolution(command_res):
    solution = None
    for line in command_res:
        fields = line.split()
        if fields[0] == "s":
            # solution
            solution = int(fields[1])
    return solution
  
def runReferenceCommand(original_path, delta_path):
  command_res = REFERENCE_PROGRAM(DELTA_PATCH(original_path, delta_path))
  return extractSolution(command_res)

def runCommand(original_path, delta_path, program):
  command_res = program(original_path, delta_path)
  return extractSolution(command_res)

def runTest(original_path, delta_path):
    reference_solution = runReferenceCommand(original_path, delta_path)
    if reference_solution == None:
        print("ERROR: Reference command failed on %s %s",
              original_path, delta_path)
    else:
        print("Reference solution: ", reference_solution)
    
    for name, program in TEST_PROGRAMS.items():
        solution = runCommand(original_path, delta_path, program)
        if solution == reference_solution:
            print("PASS - ", name)
        else:
            print("FAIL - ", name, file=sys.stderr)
            print("Expected {0}, actual {1}".
                  format(reference_solution, solution), file=sys.stderr)
            sys.exit(1)

if __name__ == "__main__":
  for (original_fname, incremental_fnames) in TEST_CASES.items():
    print("***", original_fname, "***")
    for incremental_fname in incremental_fnames:
      print("- ", incremental_fname)
      runTest(os.path.join(GRAPH_DIR, original_fname),
              os.path.join(GRAPH_DIR, incremental_fname))