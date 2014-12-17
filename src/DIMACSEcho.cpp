/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */

#include <iostream>

#include "FlowNetwork.h"
#include "DIMACS.h"

namespace flowsolver {

int main() {
	FlowNetwork g = DIMACS::readDIMACSMin(std::cin);
	DIMACS::writeDIMACSMin(g, std::cout);

	return 0;
}

}

