/*
 * BellmanFord.h
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#ifndef BELLMANFORD_H_
#define BELLMANFORD_H_

#include <queue>
#include <set>
#include <vector>

#include "ResidualNetwork.h"

namespace flowsolver {

class BellmanFord {
	ResidualNetwork &g;
	uint32_t numNodes;
	std::vector<int64_t> distance;
	std::vector<uint32_t> predecessors;
	void relax(const Arc &);
	void relaxRepeatedly();
	std::set<std::queue<Arc *>> negativeCycles();
public:
	BellmanFord(ResidualNetwork &);
	// returns a set of disjoint negative cycles, represented as an edge list
	// if no negative cycles, set is empty
	std::set<std::queue<Arc *>> run();
	virtual ~BellmanFord();
};

} /* namespace flowsolver */

#endif /* BELLMANFORD_H_ */
