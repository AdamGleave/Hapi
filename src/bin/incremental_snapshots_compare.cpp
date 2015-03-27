/* Parses a standard DIMACS input file, solves the min-cost flow problem.
 * Parses an extended DIMACS input file specifying change(s) to the graph,
 * and solves it using an incremental approach. Outputs DIMACS representation
 * of the solution. */

#include <iostream>
#include <fstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <glog/logging.h>

#include "residual_network.h"
#include "dimacs.h"

using namespace flowsolver;

void exportGraph(DIMACSExporter<ResidualNetwork> &exporter) {
	exporter.write();
	std::cout << "c EOI" << std::endl;
	std::cout.flush();
}

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	if (argc != 3) {
		std::cerr << "usage: " << argv[0]
				      << "[incremental input] [snapshot input]" << std::endl
							<< "verifies graphs in snapshot file agree with those produced "
							<< "by processing incremental input." << std::endl;
		return 1;
	}

	std::string incremental_input_path(argv[1]);
	std::string snapshot_input_path(argv[2]);

	std::ifstream incremental_input(incremental_input_path);
	std::ifstream snapshot_input(snapshot_input_path);

	if (!incremental_input.is_open()) {
		LOG(FATAL) << "could not open " << incremental_input_path << " for reading.";
	}
	if (!snapshot_input.is_open()) {
		LOG(FATAL) << "could not open " << snapshot_input_path << " for reading.";
	}

	ResidualNetwork *incremental_graph = NULL, *snapshot_graph = NULL;
	unsigned int num_iterations = 0;

	typedef DIMACSIncrementalFullImporter<ResidualNetwork> FullImporter;
	typedef DIMACSIncrementalDeltaImporter<ResidualNetwork> DeltaImporter;

	// the first iteration in both is a full graph
	incremental_graph = FullImporter(incremental_input).read();
	snapshot_graph = FullImporter(snapshot_input).read();

	CHECK_NOTNULL(incremental_graph);
	CHECK_NOTNULL(snapshot_graph);

	// but in snapshots each iteration is full, whereas in incremental subsequent
	// are deltas
	DeltaImporter incremental_delta(incremental_input, *incremental_graph);

	bool incremental_read = true, snapshot_read = true;
	while (snapshot_read && incremental_read) {
		if (*incremental_graph != *snapshot_graph) {
			std::cerr << "Divergence between graphs at iteration " << num_iterations
					      << std::endl;
			std::cout << "c ITERATION " << num_iterations << std::endl;

			std::cout << "c INCREMENTAL (expected)" << std::endl;
			DIMACSExporter<ResidualNetwork>(*incremental_graph, std::cout).write();
			std::cout << "c SNAPSHOT (actual)" << std::endl;
			DIMACSExporter<ResidualNetwork>(*snapshot_graph, std::cout).write();

			return 42;
		}

		incremental_read = incremental_delta.read();

		delete snapshot_graph;
		snapshot_graph = FullImporter(snapshot_input).read();
		snapshot_read = (snapshot_graph != NULL);

		CHECK_EQ(incremental_read, snapshot_read) << "different number of iterations";
	}

	return 0;
}
