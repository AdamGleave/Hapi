/* Applies Cycle Cancelling algorithm to DIMACS input, outputting a
 * DIMACS representation of the optimal solution.
 */

#include <iostream>

#include "CycleCancelling.h"
#include "ResidualNetwork.h"
#include "DIMACS.h"

using namespace flowsolver;

int main() {
	ResidualNetwork *g = DIMACS<ResidualNetwork>::readDIMACSMin(std::cin);
	CycleCancelling cc(*g);
	cc.run();
	DIMACS<ResidualNetwork>::writeDIMACSMinFlow(*g, std::cout);

	return 0;
}
