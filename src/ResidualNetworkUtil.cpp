/*
 * ResidualNetworkUtil.cpp
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#include <queue>

#include "ResidualNetworkUtil.h"

namespace flowsolver {

uint64_t ResidualNetworkUtil::augmentingFlow
							  (ResidualNetwork &g, std::queue<Arc *> path) {
	uint32_t src_id = path.front()->getSrcId();
	// TODO: delete?
	/*uint64_t excess = g.getSupply(src_id);
	std::unordered_map<uint32_t, Arc *> adjacencies = g.getAdjacencies(src_id);
	std::unordered_map<uint32_t, Arc *>::iterator it;
	for (it = adjacencies.begin(); it != adjacencies.end(); ++it) {
		Arc *arc = it->second;
		excess -= arc->getFlow();
	}*/
	uint64_t augmenting_flow = std::max(g.getSupply(src_id), 0l);

	while (!path.empty()) {
		Arc *arc = path.front();
		path.pop();

		uint64_t capacity = arc->getCapacity();
		augmenting_flow = std::min(augmenting_flow, capacity);
	}

	return augmenting_flow;
}

void ResidualNetworkUtil::augmentPath
	 	 	 	 	 	  (ResidualNetwork &g, std::queue<Arc *> path) {
	uint64_t flow = augmentingFlow(g, path);

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
