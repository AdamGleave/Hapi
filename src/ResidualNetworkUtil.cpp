/*
 * ResidualNetworkUtil.cpp
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#include <queue>

#include "ResidualNetworkUtil.h"

namespace flowsolver {

uint64_t ResidualNetworkUtil::augmentingFlow(std::queue<Arc *> path) {
	uint64_t augmenting_flow = UINT64_MAX;

	do {
		Arc *arc = path.front();
		path.pop();

		uint64_t capacity = arc->getCapacity();
		augmenting_flow = std::min(augmenting_flow, capacity);

	} while (!path.empty());

	return augmenting_flow;
}

void ResidualNetworkUtil::augmentPath
	 	 	 	 	 	  (ResidualNetwork &g, std::queue<Arc *> path) {
	uint64_t flow = augmentingFlow(path);

	while (!path.empty()) {
		Arc *arc = path.front();
		path.pop();

		uint32_t src = arc->getSrcId();
		uint32_t dst = arc->getDstId();
		// N.B. Must do this through ResidualNetwork rather than Arc
		// directly, so that reverse arc is also updated
		g.pushFlow(src, dst, flow);
	}
}

} /* namespace flowsolver */
