from config.common import *

REFERENCE_PROGRAM = sh.Command(REFERENCE_PROGRAM_PATH) \
                    .bake(*REFERENCE_PROGRAM_ARGUMENTS)
                    
TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "find_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["augmenting_path",
                          "cost_scaling",
                          #"cycle_cancelling"
                         ]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

TEST_GRAPHS = prefix_list("clusters",
                prefix_list("synthetic/handmade",
                             ["small_graph.in", "small_graph_neg_costs.in"])
              + prefix_list("synthetic/firmament", 
                             ["graph_4m_2crs_8j.in", "graph_4m_2crs_10j.in",
                              "graph_100m_8j_100t_10p.in", 
                              "graph_1000m_32j_100t_10p.in"])
              + ["natural/google_trace/google_all.in"])