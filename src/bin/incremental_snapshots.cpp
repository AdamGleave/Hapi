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
	// CLI parsing
	bool last_only = false;
	for (int i = 1; i < argc; i++) {
		char *str = argv[i];
		if (strcmp(str, "quiet") == 0) {
			FLAGS_minloglevel = google::ERROR;
		} else if (strcmp(str, "last_only") == 0) {
			last_only = true;
		} else {
			fprintf(stderr, "usage: %s [quiet] [last_only]\n", argv[0]);
			return -1;
		}
	}
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// load full file
	ResidualNetwork *g = DIMACSIncrementalFullImporter<ResidualNetwork>
                                                              (std::cin).read();

	// re-export immediately: this is our initial snapshot
	DIMACSExporter<ResidualNetwork> exporter(*g, std::cout);
	if (!last_only) {
		exportGraph(exporter);
	}

	// now process stream of incremental deltas, outputting snapshots
	DIMACSIncrementalDeltaImporter<ResidualNetwork>
	                                           incremental_importer(std::cin, *g);

	while (incremental_importer.read()) {
		if (!last_only) {
			exportGraph(exporter);
		}
	}
	if (last_only) {
		exportGraph(exporter);
	}

	return 0;
}
