/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */
#include "dimacs.h"

#include <iostream>
#include <fstream>
#include <glog/logging.h>

#include "residual_network.h"
#include "flow_network.h"

using namespace flowsolver;

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	if (argc != 3) {
		std::cerr << "usage: " << argv[0]
							<< " <original input> <incremental input>" << std::endl;
		return -1;
	}
	std::string original_fname = argv[1], incremental_fname = argv[2];
	std::ifstream original(original_fname), incremental(incremental_fname);
	if (original.fail()) {
		std::cerr << "error opening file " << original_fname << std::endl;
		return -1;
	}
	if (incremental.fail()) {
		std::cerr << "error opening file " << incremental_fname << std::endl;
		return -1;
	}

	ResidualNetwork *g = DIMACSOriginalImporter<ResidualNetwork>(original).read();
	DIMACSIncrementalImporter<ResidualNetwork>(incremental, *g).read();
	DIMACSExporter<ResidualNetwork>(*g, std::cout).write();

	return 0;
}
