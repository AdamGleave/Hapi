from config.common import *

# Program to merge incremental deltas with full graph
SNAPSHOT_CREATOR_PROGRAM_PATH = os.path.join(EXECUTABLE_DIR,
                                             "incremental_snapshots")
SNAPSHOT_CREATOR_PROGRAM = sh.Command(SNAPSHOT_CREATOR_PROGRAM_PATH)

# Program to run full solver repeatedly for each snapshot
SNAPSHOT_SOLVER_PROGRAM_PATH  = os.path.join(EXECUTABLE_SRC_DIR,
                                             "snapshot_solver")
SNAPSHOT_SOLVER_PROGRAM = sh.Command(SNAPSHOT_SOLVER_PROGRAM_PATH)

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = []

TEST_PROGRAMS = { "augmenting_path" : TEST_PROGRAM }

HANDMADE_INCREMENTAL_DIR = os.path.join("clusters", "synthetic", 
                                        "firmament", "incremental")
GOOGLE_INCREMENTAL_DIR = os.path.join("clusters", "natural", "google_trace")
TEST_CASES = {
#               os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j.in"): 
#                graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j_*")),
#               os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j.in"): 
#                 graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j_*")),
#               os.path.join(GOOGLE_INCREMENTAL_DIR, "tiny_trace.imin"): None,
              os.path.join(GOOGLE_INCREMENTAL_DIR, "small_trace.imin"): None,
             }