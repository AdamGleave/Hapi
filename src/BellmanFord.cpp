/*
 * BellmanFord.cpp
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

//TODO: debug
#include <iostream>

#include "BellmanFord.h"

namespace flowsolver {

BellmanFord::BellmanFord(ResidualNetwork &g) : g(g) {
	numNodes = g.getNumNodes();
	distance.resize(numNodes + 1);
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
			if (arc.getCapacity() > 0) {
				// ignore 0-capacity arcs: these are not really part of the
				// residual network
				relax(arc);
			}
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
	std::set<uint32_t>::iterator it;
	for (it = negative_cycle_nodes.begin();
		 it != negative_cycle_nodes.end(); ++it) {
		std::deque<Arc *> cycle;
		uint32_t cycle_start = *it;

		if (seen_nodes.count(cycle_start) > 0) {
			// We've already dealt with a cycle containing cycle_start;
			// we just started from a different end! Ignore to avoid duplicates.
			continue;
		}

		std::set<uint32_t> nodes_traversed;
		uint32_t cur, prev;
		cur = cycle_start;

		// TODO: debug
		std::cout << "Start: " << cycle_start << std::endl;
		do {
			// TODO: debug
			std::cout << cur << std::endl;
			if (nodes_traversed.count(cur) > 0) {
				// TODO: better way of handling this?
				// we started from a place that's not in a cycle: abort
				cycle.clear();
				break;
			}
			nodes_traversed.insert(cur);

			prev = predecessors[cur];
			Arc *arc = g.getArc(prev, cur);
			cycle.push_back(arc);

			cur = prev;

		} while (cur != cycle_start);

		if (!cycle.empty()) {
			negative_cycles.insert(std::queue<Arc *>(cycle));
			seen_nodes.insert(nodes_traversed.begin(), nodes_traversed.end());
		}
	}

	return negative_cycles;
}

std::set<std::queue<Arc *>> BellmanFord::run() {
	// TODO: if set to INT64_MAX, will overflow when calculating through_distance. Better fix?
	distance.assign(numNodes + 1, INT64_MAX/2);
	predecessors.assign(numNodes + 1, 0);

	const std::set<uint32_t> &sources = g.getSources();
	std::set<uint32_t>::const_iterator it;
	for (it = sources.begin(); it != sources.end(); ++it) {
		// all sources have zero distance
		uint32_t id = *it;
		distance[id - 1] = 0;
	}

	relaxRepeatedly();
	return negativeCycles();
}

BellmanFord::~BellmanFord() { }

} /* namespace flowsolver */
