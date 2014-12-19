/*
 * CycleCancelling.cpp
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#include "ResidualNetworkUtil.h"
#include "CycleCancelling.h"

#include "DIMACS.h"

namespace flowsolver {

void CycleCancelling::run() {
	// calculate maximum flow (in general this is not a minimum cost flow)
	EdmondsKarp ek(g);
	ek.run();

	// whilst negative cycles in the residual network exist, cancel them
	BellmanFord bf(g);
	std::set<std::queue<Arc *>> negative_cycles;
	std::set<std::queue<Arc *>>::const_iterator cycle_it;
	while (!(negative_cycles = bf.run()).empty()) {
		for (cycle_it = negative_cycles.begin();
			 cycle_it != negative_cycles.end(); ++cycle_it) {
			ResidualNetworkUtil::cancelCycle(g, *cycle_it);
		}
	}
}

} /* namespace flowsolver */
