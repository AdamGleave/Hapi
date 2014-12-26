/* Reads a min-cost flow network in DIMACS format on stdin, and parses it.
 * Exports the resulting graph representation in DIMACS format on stdout.
 * (Output should be identical to input, modulo missing comments.)
 */

#include <iostream>

#include "CycleCancelling.h"
#include "DIMACS.h"
#include "ResidualNetwork.h"

using namespace flowsolver;

int main() {
	ResidualNetwork g = DIMACS<ResidualNetwork>::readDIMACSMin(std::cin);
	CycleCancelling cc(g);
	cc.run();
	DIMACS<ResidualNetwork>::writeDIMACSMinFlow(g, std::cout);

	return 0;
}
