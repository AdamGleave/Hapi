from config.common import *

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = []

TEST_PROGRAMS = { "augmenting_path" : TEST_PROGRAM }

HANDMADE_INCREMENTAL_DIR = os.path.join("clusters", "synthetic", 
                                        "firmament", "incremental")
GOOGLE_INCREMENTAL_DIR = os.path.join("clusters", "natural", "google_trace")
TEST_CASES = {
               os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j.in"): 
                graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j_*")),
               os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j.in"): 
                 graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j_*")),
               os.path.join(GOOGLE_INCREMENTAL_DIR, "tiny_trace.imin"): None,
              os.path.join(GOOGLE_INCREMENTAL_DIR, "small_trace.imin"): None,
             }