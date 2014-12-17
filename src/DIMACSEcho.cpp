/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */

#include <iostream>

#include "DIMACS.h"
#include "ResidualNetwork.h"

int main() {
	flowsolver::ResidualNetwork g = flowsolver::DIMACS::readDIMACSMin(std::cin);
	flowsolver::DIMACS::writeDIMACSMin(g, std::cout);

	return 0;
}
