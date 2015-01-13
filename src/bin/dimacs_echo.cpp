/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */

#include <iostream>
#include <glog/logging.h>

#include "dimacs.h"
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
		std::cerr << "usage: " << argv[0] << "[FlowNetwork|ResidualNetwork]";
		return -1;
	}

	if (graph == "FlowNetwork") {
		FlowNetwork *g = DIMACS<FlowNetwork>::readDIMACSMin(std::cin);
		DIMACS<FlowNetwork>::writeDIMACSMin(*g, std::cout);
	} else if (graph == "ResidualNetwork") {
		ResidualNetwork *g = DIMACS<ResidualNetwork>::readDIMACSMin(std::cin);
		DIMACS<ResidualNetwork>::writeDIMACSMin(*g, std::cout);
	} else {
		assert(false);
	}

	return 0;
}
