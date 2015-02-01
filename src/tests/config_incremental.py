from config_common import *

# Program to merge incremental deltas with full graph
DELTA_PATCH_PATH = os.path.join(EXECUTABLE_DIR, "dimacs_incremental")
DELTA_PATCH = sh.Command(DELTA_PATCH_PATH)

TEST_PROGRAM = sh.Command(os.path.join(EXECUTABLE_DIR, "incremental_min_cost"))
TEST_PROGRAM_ARGUMENTS = ["incremental"]

TEST_PROGRAMS = { name : TEST_PROGRAM.bake(name)
                  for name in TEST_PROGRAM_ARGUMENTS }

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