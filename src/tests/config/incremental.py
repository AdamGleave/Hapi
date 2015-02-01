from config.common import *

# Program to merge incremental deltas with full graph
DELTA_PATCH_PATH = os.path.join(EXECUTABLE_DIR, "dimacs_incremental")
DELTA_PATCH = sh.Command(DELTA_PATCH_PATH)

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["incremental"]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

INCREMENTAL_DIR = os.path.join("clusters", "synthetic", 
                               "firmament", "incremental")
TEST_CASES = {os.path.join(INCREMENTAL_DIR, "graph_4m_2crs_6j.in"): 
                graphGlob(os.path.join(INCREMENTAL_DIR, "graph_4m_2crs_6j_*")),
              os.path.join(INCREMENTAL_DIR, "graph_4m_2crs_10j.in"): 
                graphGlob(os.path.join(INCREMENTAL_DIR, "graph_4m_2crs_10j_*")),
             }