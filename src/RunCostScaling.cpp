/* Applies Cost Scaling algorithm to DIMACS input, outputting a
 * DIMACS representation of the optimal solution.
 */

#include <iostream>

#include <glog/logging.h>

#include "CostScaling.h"
#include "FlowNetwork.h"
#include "DIMACS.h"

using namespace flowsolver;

int main() {
	FlowNetwork *g = DIMACS<FlowNetwork>::readDIMACSMin(std::cin);
	CostScaling cc(*g);
	bool success = cc.run();
	LOG_IF(ERROR, !success) << "No feasible solution.";
	DIMACS<FlowNetwork>::writeDIMACSMinFlow(*g, std::cout);

	return 0;
}
