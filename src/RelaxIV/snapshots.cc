#include <iostream>
#include <fstream>
#include <string>

#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "RelaxIV_incremental.h"

#include "dimacs.h"

using namespace flowsolver_bertsekas;

void compute_reserved_memory(MCFClass::Index &num_nodes,
		                         MCFClass::Index &num_arcs) {
	// TODO(adam): less crude method of reserving memory
	num_nodes *= 2;
	num_arcs *= 2;
}

int main(int, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// initialize relevant classes
	RelaxIV *mcf = new RelaxIV();
	DIMACS dimacs(std::cin, mcf);

	// load full graph
	MCFClass::Index     tn, tm; // number of nodes & arcs
	MCFClass::FRow      tU; // arc upper capacities
	MCFClass::CRow      tC; // arc costs
	MCFClass::FRow      tDfct; // node deficits
	MCFClass::Index_Set tStartn, tEndn; // arc start & end nodes

	dimacs.ReadInitial(&tn, &tm, &tU, &tC, &tDfct, &tStartn, &tEndn);

	MCFClass::Index num_nodes_reserved = tn, num_arcs_reserved = tm;
	compute_reserved_memory(num_nodes_reserved, num_arcs_reserved);
	mcf->LoadNet(num_nodes_reserved, num_arcs_reserved, tn, tm,
			           tU, tC, tDfct, tStartn, tEndn);

	delete[] tU;
	delete[] tDfct;
	delete[] tStartn;
	delete[] tEndn;
	delete[] tC;

	// output state of graph, read delta, repeat
	// produces snapshots of the (full) state of the graph
	do {
		mcf->WriteMCF(std::cout, MCFClass::kDimacs);
		std::cout << "c EOI" << endl;
		std::cout.flush();
	} while (dimacs.ReadDelta());

	return 0;
}
