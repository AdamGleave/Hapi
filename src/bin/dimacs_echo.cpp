/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */
#include "dimacs.h"

#include <iostream>
#include <glog/logging.h>

#include "residual_network.h"
#include "flow_network.h"

using namespace flowsolver;

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	std::string graph;
	if (argc == 1) {
		graph = "FlowNetwork";
	} else if (argc == 2) {
		graph = argv[1];
	} else {
		std::cerr << "usage: " << argv[0] << " [FlowNetwork|ResidualNetwork]";
		return -1;
	}

	if (graph == "FlowNetwork") {
		FlowNetwork *g = DIMACSOriginalImporter<FlowNetwork>(std::cin).read();
		DIMACSExporter<FlowNetwork>(*g, std::cout).write();
	} else if (graph == "ResidualNetwork") {
		ResidualNetwork *g = DIMACSOriginalImporter<ResidualNetwork>(std::cin).read();
		DIMACSExporter<ResidualNetwork>(*g, std::cout).write();
	} else {
		assert(false);
	}

	return 0;
}
