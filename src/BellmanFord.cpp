/*
 * BellmanFord.cpp
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#include "BellmanFord.h"

namespace flowsolver {

BellmanFord::BellmanFord(ResidualNetwork &g) : g(g) {
	numNodes = g.getNumNodes();
	distance.assign(numNodes, INT64_MAX);
	predecessors.resize(numNodes);

	const std::set<uint32_t> &sources = g.getSources();
	std::set<uint32_t>::const_iterator it;
	for (it = sources.begin(); it != sources.end(); ++it) {
		// all sources have zero distance
		uint32_t id = *it;
		distance[id - 1] = 0;
	}
}

void BellmanFord::relax(const Arc &arc) {
	uint32_t src = arc.getSrcId(), dst = arc.getDstId();
	int64_t through_distance = distance[src] + arc.getCost();
	if (distance[dst] > through_distance) {
		distance[dst] = through_distance;
		predecessors[dst] = src;
	}
}

void BellmanFord::relaxRepeatedly() {
	for (uint32_t i = 1; i < numNodes; i++) {
		// repeat numNodes - 1 times
		ResidualNetwork::const_iterator it;
		for (it = g.begin(); it != g.end(); ++it) {
			const Arc &arc = *it;
			relax(arc);
		}
	}
}

std::set<std::queue<Arc *>> BellmanFord::negativeCycles() {
	std::set<uint32_t> negative_cycle_nodes;
	ResidualNetwork::const_iterator graph_it;
	for (graph_it = g.begin(); graph_it != g.end(); ++graph_it) {
		const Arc &arc = *graph_it;
		if (distance[arc.getDstId()] >
			distance[arc.getSrcId()] + arc.getCost()) {
			negative_cycle_nodes.insert(arc.getSrcId());
		}
	}

	std::set<uint32_t> seen_nodes;
	std::set<std::queue<Arc *>> negative_cycles;
	std::set<uint32_t>::iterator it = negative_cycle_nodes.begin();
	while (it != negative_cycle_nodes.end()) {
		std::deque<Arc *> cycle;
		uint32_t cycle_start = *it;
		uint32_t cur, prev;
		cur = cycle_start;
		do {
			if (seen_nodes.count(cur) > 0) {
				// cur is already in a cycle: abort
				cycle.clear();
				break;
			}
			seen_nodes.insert(cur);

			prev = predecessors[cur];
			Arc *arc = g.getAdjacencies(prev)[cur];
			cycle.push_back(arc);

			cur = prev;

		} while (cur != cycle_start);

		negative_cycles.insert(std::queue<Arc *>(cycle));
	}

	return negative_cycles;
}

std::set<std::queue<Arc *>> BellmanFord::run() {
	relaxRepeatedly();
	return negativeCycles();
}

BellmanFord::~BellmanFord() { }

} /* namespace flowsolver */
