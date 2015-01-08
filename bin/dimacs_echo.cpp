/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */

#include <iostream>
#include <glog/logging.h>

#include "dimacs.h"
#include "residual_network.h"

using namespace flowsolver;

int main(int argc, char *argv[]) {
	google::InitGoogleLogging(argv[0]);

	ResidualNetwork *g = DIMACS<ResidualNetwork>::readDIMACSMin(std::cin);
	DIMACS<ResidualNetwork>::writeDIMACSMin(*g, std::cout);

	return 0;
}
