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

void printStatistics(DIMACSIncrementalDeltaStatistics &stats) {
	std::cout << stats.node_additions << "," << stats.node_deletions << ","
					  << stats.arc_additions << "," << stats.arc_deletions << ","
					  << stats.arc_changes << ","	<< stats.arc_cost_changes << ","
						<< stats.arc_cap_changes << ","
					  << stats.noop_additions << "," << stats.noop_changes << ","
						<< stats.noop_deletions
					  << std::endl;
}

int main(int argc, char *argv[]) {
	// initialise logging
	FLAGS_minloglevel = google::ERROR;
	if (argc == 2 && strcmp(argv[1], "verbose") == 0) {
			FLAGS_minloglevel = google::INFO;
	} else if (argc > 1) {
		fprintf(stderr, "usage: %s [quiet]\n", argv[0]);
		return -1;
	}
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// load full file
	ResidualNetwork *g = DIMACSIncrementalFullImporter<ResidualNetwork>
                                                              (std::cin).read();


	// now process stream of incremental deltas, outputting statistics
	std::cout << "iterations,n_additions,n_deletions,"
			      << "a_additions,a_deletions,a_changes,"
						<< "a_cost_changes,a_cap_changes,"
						<< "noop_additions,noop_changes,noop_deletions"
						<< std::endl;

	DIMACSIncrementalDeltaImporter<ResidualNetwork>
	                                           incremental_importer(std::cin, *g);
	unsigned int num_iterations = 0;
	while (incremental_importer.read()) {
		std::cout << num_iterations << ",";
		printStatistics(incremental_importer.getStatistics());
		incremental_importer.resetStatistics();

		num_iterations++;
	}

	return 0;
}
