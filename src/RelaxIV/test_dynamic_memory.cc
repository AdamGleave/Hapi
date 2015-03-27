#include "RelaxIV_incremental.h"
#include "dimacs.h"

#include <fstream>

#include <gtest/gtest.h>

class DynamicMemoryTest : public ::testing::Test {
protected:
	std::fstream graph_file;

	RelaxIV *mcf;
	flowsolver_bertsekas::DIMACS *dimacs;

	const std::string GRAPH_PATH = CMAKE_SRC_DIR
			      "/graphs/clusters/synthetic/firmament/incremental/add_node_arcs.in";
	const MCFClass::Index initial_nodes = 24, initial_arcs = 56;
	const MCFClass::Index final_nodes = 25, final_arcs = 61;
	const MCFClass::Index final_nodes_alloc = 48, final_arcs_alloc = 112;

	void SetUp() {
		graph_file.open(GRAPH_PATH, std::fstream::in);
		ASSERT_FALSE(graph_file.fail()) << "unable to open " << GRAPH_PATH;

		// initialize relevant classes
		mcf = new RelaxIV();
		dimacs = new flowsolver_bertsekas::DIMACS(graph_file, mcf);

		// load initial network
		MCFClass::Index     tn, tm; // number of nodes & arcs
		MCFClass::FRow      tU; // arc upper capacities
		MCFClass::CRow      tC; // arc costs
		MCFClass::FRow      tDfct; // node deficits
		MCFClass::Index_Set tStartn, tEndn; // arc start & end nodes

		dimacs->ReadInitial(&tn, &tm, &tU, &tC, &tDfct, &tStartn, &tEndn);

		// load network
		mcf->LoadNet(tn, tm, tn, tm, tU, tC, tDfct, tStartn, tEndn);
		delete[] tU;
		delete[] tDfct;
		delete[] tStartn;
		delete[] tEndn;
		delete[] tC;
	}
};

TEST_F(DynamicMemoryTest, Delta) {
	ASSERT_EQ(mcf->MCFn(), initial_nodes);
	ASSERT_EQ(mcf->MCFnmax(), initial_nodes);
	ASSERT_EQ(mcf->MCFm(), initial_arcs);
	ASSERT_EQ(mcf->MCFmmax(), initial_arcs);

	ASSERT_TRUE(dimacs->ReadDelta()) << "error reading first delta.";

	ASSERT_EQ(mcf->MCFn(), final_nodes);
	ASSERT_EQ(mcf->MCFnmax(), final_nodes_alloc);
	ASSERT_EQ(mcf->MCFm(), final_arcs);
	ASSERT_EQ(mcf->MCFmmax(), final_arcs_alloc);
}
