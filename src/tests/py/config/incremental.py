from config.common import *

### Incremental graph

SNAPSHOTS_COMPARE = sh.Command(os.path.join(PROJECT_ROOT, BUILD_PREFIX, "bin",
                                            "incremental_snapshots_compare"))
GRAPH_TEST_PROGRAMS = {"relaxiv": sh.Command(os.path.join(PROJECT_ROOT,
                         BUILD_PREFIX, "RelaxIV", "RelaxIV_snapshots"))
                      }
FIFO_FNAME = "snapshots.fifo"

### Incremental solver
SOLVER_TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
#TEST_PROGRAM_ARGUMENTS = ["augmenting_path", "relax"]
SOLVER_TEST_PROGRAM_ARGUMENTS = []

SOLVER_TEST_PROGRAMS = { name : SOLVER_TEST_PROGRAM.bake(name)
                         for name in SOLVER_TEST_PROGRAM_ARGUMENTS }
SOLVER_TEST_PROGRAMS["relax_frangioni"] = sh.Command(os.path.join(PROJECT_ROOT,
                                        BUILD_PREFIX, "RelaxIV", "incremental"))

### Dataset

HANDMADE_INCREMENTAL_DIR = os.path.join("clusters", "synthetic", 
                                        "firmament", "incremental")
GOOGLE_INCREMENTAL_DIR = os.path.join("clusters", "natural", "google_trace")
TEST_CASES = {
  ### handmade data. tiny, but cover a lot of cases.
  # 6 jobs
  os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j.in"): 
    graph_glob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_6j_*")),
  # 10 jobs
  os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j.in"): 
    graph_glob(os.path.join(HANDMADE_INCREMENTAL_DIR, "graph_4m_2crs_10j_*")),
  ### deltas from Google traces
  # this one is quick
  os.path.join(GOOGLE_INCREMENTAL_DIR, "tiny_trace.imin"): None,
  # this one is quite large (~20M) and will take a while
  os.path.join(GOOGLE_INCREMENTAL_DIR, "small_trace.imin"): None,
}
