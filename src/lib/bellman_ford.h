/*
 * BellmanFord.h
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#ifndef LIB_BELLMAN_FORD_H_
#define LIB_BELLMAN_FORD_H_

#include <queue>
#include <set>
#include <vector>

#include "residual_network.h"

namespace flowsolver {

class BellmanFord {
public:
	explicit BellmanFord(ResidualNetwork &);
	// returns a set of disjoint negative cycles, represented as an edge list
	// if no negative cycles, set is empty
	std::set<std::queue<Arc *>> run();
	virtual ~BellmanFord();
private:
	void relax(const Arc &);
	void relaxRepeatedly();
	std::set<std::queue<Arc *>> negativeCycles();

	ResidualNetwork &g;
	uint32_t numNodes;
	std::vector<int64_t> distance;
	std::vector<uint32_t> predecessors;
};

} /* namespace flowsolver */

#endif /* LIB_BELLMAN_FORD_H_ */
