from config_common import *

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "find_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["augmenting_path",
                          "cost_scaling",
                          #"cycle_cancelling"
                         ]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

GRAPH_DIR = os.path.join(BASE_DIR, "src", "graphs", "clusters")
TEST_GRAPHS = ["synthetic/handmade/small_graph.in",
               "synthetic/handmade/small_graph_neg_costs.in",
               "synthetic/firmament/graph_4m_2crs_8j.in",
               "synthetic/firmament/graph_4m_2crs_10j.in",
               "synthetic/firmament/graph_100m_8j_100t_10p.in",
               "synthetic/firmament/graph_1000m_32j_100t_10p.in",
               "natural/google_trace/google_all.in"
               ]