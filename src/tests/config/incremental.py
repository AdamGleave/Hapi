from config.common import *

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["augmenting_path", "relax"]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }
TEST_PROGRAMS["relax_frangioni"] = sh.Command(os.path.join(PROJECT_ROOT,
                                        BUILD_PREFIX, "RelaxIV", "incremental"))

HANDMADE_INCREMENTAL_DIR = os.path.join("clusters", "synthetic", 
                                        "firmament", "incremental")
GOOGLE_INCREMENTAL_DIR = os.path.join("clusters", "natural", "google_trace")
TEST_CASES = {
  ### handmade data. tiny, but cover a lot of cases.
  # 6 jobs
  os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j.in"): 
    graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j_*")),
  # 10 jobs
  os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j.in"): 
    graphGlob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j_*")),
  ### deltas from Google traces
  # this one is quick
  os.path.join(GOOGLE_INCREMENTAL_DIR, "tiny_trace.imin"): None,
  # this one is quite large (~20M) and will take a while
  os.path.join(GOOGLE_INCREMENTAL_DIR, "small_trace.imin"): None,
}
