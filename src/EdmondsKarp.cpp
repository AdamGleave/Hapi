/*
 * EdmondsKarp.cpp
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#include <queue>
#include <set>
#include <list>
#include <utility>
#include <cstring>

#include "EdmondsKarp.h"

namespace flowsolver {

EdmondsKarp::EdmondsKarp(ResidualNetwork &g) : g(g) {
	numNodes = g.getNumNodes();
	predecessors = std::vector<uint32_t>(numNodes);
}

void EdmondsKarp::run() {
	for (uint32_t id = 1; id < numNodes; id++) {
		int64_t supply = g.getSupply(id);
		if (supply > 0) {
			// source node
			source.insert(id);
		} else if (supply < 0) {
			// sink node
			sink.insert(id);
		}
	}

	std::queue<Arc *> path;
	while (!(path = bfs()).empty()) {
		// whilst there is an augmenting path
		uint64_t flow = augmenting_flow(path);

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
}

uint64_t EdmondsKarp::augmenting_flow(std::queue<Arc *> path) {
	uint64_t augmenting_flow = UINT64_MAX;

	do {
		Arc *arc = path.front();
		path.pop();

		uint64_t capacity = arc->getCapacity();
		augmenting_flow = std::min(augmenting_flow, capacity);

	} while (!path.empty());

	return augmenting_flow;
}

std::queue<Arc *> EdmondsKarp::predecessor_path(uint32_t end) {
	std::deque<Arc *> path;
	uint32_t cur, prev;
	cur = end;

	while (source.count(cur) == 0) {
		// cur not a source
		prev = predecessors[cur];
		Arc *arc = g.getAdjacencies(prev)[cur];
		path.push_front(arc);
	}

	return std::queue<Arc *>(path);
}

// if no path exists from source to sink, return empty vector
std::queue<Arc *> EdmondsKarp::bfs() {
	// Breadth First Search (BFS) to find all paths from a source to sink
	// in the residual network (all augmenting paths)
	std::list<uint32_t> to_visit_list(source.begin(), source.end());
	std::queue<uint32_t, std::list<uint32_t>> to_visit(to_visit_list);

	// note ID 0 is unused
	predecessors.assign(predecessors.size(), 0);

	while (!to_visit.empty()) {
		uint32_t cur = to_visit.front();
		to_visit.pop();
		std::unordered_map<uint32_t, Arc*> adjacencies = g.getAdjacencies(cur);
		std::unordered_map<uint32_t, Arc*>::iterator it;
		for (it = adjacencies.begin(); it != adjacencies.end(); ++it) {
			uint32_t next = it->first;
			if (predecessors[next] == 0) {
				// not already encountered next
				predecessors[next] = cur;

				if (sink.count(next) > 0) {
					// found path from source to sink
					return predecessor_path(next);
				}

				to_visit.push(next);
			}
		}
	}

	return std::queue<Arc *>();
}

EdmondsKarp::~EdmondsKarp() { }

} /* namespace flowsolver */
