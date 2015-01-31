#include "dynamic_maintain_optimality.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include "augmenting_path.h"
#include "dimacs.h"
#include "residual_network.h"

using namespace flowsolver;

void check_reduced_cost_optimality(const ResidualNetwork &g,
		                               const std::vector<uint64_t> potentials) {
	for (const Arc &arc : g) {
		int64_t reduced_cost = arc.getCost() - potentials[arc.getSrcId()]
																				 + potentials[arc.getDstId()];
		int64_t flow = arc.getFlow();

		if (reduced_cost > 0) {
			// flow must be zero if reduced cost is positive
			// but we may be looking at a *reverse* arc; in this case, the way
			// we maintain the data structure will lead to a *negative* flow if there
			// is positive flow on the (negative reduced cost) forward arc.
			EXPECT_LE(flow, 0) << arc.getSrcId() << "->" << arc.getDstId();
		} else if (reduced_cost < 0) {
			EXPECT_EQ(arc.getInitialCapacity(), flow)
					<< arc.getSrcId() << "->" << arc.getDstId()
					<< " cost " << arc.getCost() << " - " << potentials[arc.getSrcId()]
					<< " + " << potentials[arc.getDstId()] << " = " << reduced_cost;
		}
	}
}

struct TestCase {
	std::string prefix;
	std::string graph_fname;
	std::list<std::string> incremental_fnames;
};

class DynamicMaintainOptimalityTest :public ::testing::TestWithParam<TestCase> {
protected:
	void SetUp() {
		// load the graph
		std::string graph_fname = GetParam().prefix + GetParam().graph_fname;
		std::ifstream graph_file(graph_fname);
		CHECK(!graph_file.fail()) << "could not open " << graph_fname;

		g = DIMACSOriginalImporter<ResidualNetwork>(graph_file).read();
		CHECK(g != nullptr) << "error parsing " << graph_fname;

		// solve the flow network: this guarantees we have a feasible, optimal
		// solution in g
		AugmentingPath ap(*g);
		ap.run();

		potentials = ap.getPotentials();

		check_reduced_cost_optimality(*g, potentials);
	}

	ResidualNetwork *g;
	std::vector<uint64_t> potentials;
};

TEST_P(DynamicMaintainOptimalityTest, OptimalityInvariant) {
	for (std::string incremental_fname : GetParam().incremental_fnames) {
		std::cout << "Testing " << incremental_fname << std::endl;
		// load the incremental delta
		std::string incremental_path = GetParam().prefix + incremental_fname;
		std::ifstream incremental_file(incremental_path);
		CHECK(!incremental_file.fail()) << "could not open " << incremental_path;

		// make a copy of the state we're about to modify
		ResidualNetwork copy_g(*g);
		std::vector<uint64_t> copy_potentials(potentials);
		DynamicMaintainOptimality dynamic(copy_g, copy_potentials);
		typedef DIMACSIncrementalImporter<DynamicMaintainOptimality> DIMACSImporter;
		DIMACSImporter(incremental_file, dynamic).read();

		// check optimality still holds
		check_reduced_cost_optimality(copy_g, copy_potentials);
	}
}

const std::string GRAPH_PATH = CMAKE_SRC_DIR "/graphs/";
const std::string SMALL_INCREMENTAL_PATH =
											 GRAPH_PATH + "clusters/synthetic/firmament/incremental/";


TestCase tc = { GRAPH_PATH + "clusters/synthetic/firmament/incremental/",
							  "graph_4m_2crs_6j.in",
								{
										 "graph_4m_2crs_6j_add_arc_equiv_core.in",
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
										 "graph_4m_2crs_6j_rem_sched_task.in"
								}
              };

INSTANTIATE_TEST_CASE_P(SmallIncrementalTest, DynamicMaintainOptimalityTest,
												::testing::Values(tc));
