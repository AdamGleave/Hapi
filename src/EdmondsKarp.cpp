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

#include "ResidualNetworkUtil.h"
#include "EdmondsKarp.h"

// TODO: debug
#include <iostream>

namespace flowsolver {

EdmondsKarp::EdmondsKarp(ResidualNetwork &g) : g(g) {
	numNodes = g.getNumNodes();
	predecessors.resize(numNodes + 1);
}

void EdmondsKarp::run() {
	std::queue<Arc *> path;
	while (!(path = bfs()).empty()) {
		// whilst there is an augmenting path
		ResidualNetworkUtil::augmentPath(g, path);
	}
}

std::queue<Arc *> EdmondsKarp::predecessorPath(uint32_t end) {
	std::deque<Arc *> path;
	uint32_t cur, prev;
	cur = end;

	const std::set<uint32_t> &sources = g.getSources();
	while (sources.count(cur) == 0) {
		// cur not a source
		prev = predecessors[cur];
		Arc *arc = g.getArc(prev, cur);
		path.push_front(arc);
		cur = prev;
	}

	return std::queue<Arc *>(path);
}

// if no path exists from source to sink, return empty vector
std::queue<Arc *> EdmondsKarp::bfs() {
	// Breadth First Search (BFS) to find all paths from a source to sink
	// in the residual network (all augmenting paths)
	const std::set<uint32_t> &sources = g.getSources();
	const std::set<uint32_t> &sinks = g.getSinks();
	// TODO: debug
	std::set<uint32_t>::const_iterator it;
	std::cout << sources.size() << std::endl;
	for (it = sources.begin(); it != sources.end(); ++it) {
		std::cout << *it << std::endl;
	}
	std::cout << "baz" << std::endl;
	std::list<uint32_t> to_visit_list(sources.begin(), sources.end());
	std::cout << "foo" << std::endl;
	std::queue<uint32_t, std::list<uint32_t>> to_visit(to_visit_list);
	std::cout << "bar" << std::endl;

	// note ID 0 is unused
	predecessors.assign(predecessors.size() + 1, 0);

	while (!to_visit.empty()) {
		uint32_t cur = to_visit.front();
		std::cout << "Visited " << cur << std::endl;
		to_visit.pop();
		std::unordered_map<uint32_t, Arc*> adjacencies = g.getAdjacencies(cur);
		std::unordered_map<uint32_t, Arc*>::iterator it;
		for (it = adjacencies.begin(); it != adjacencies.end(); ++it) {
			Arc *arc = it->second;
			if (arc->getCapacity() == 0) {
				// 0-capacity arcs are not really in the residual network,
				// they're present in the data structure only to simplify
				// the representation. Ignore.
				continue;
			}

			uint32_t next = it->second->getDstId();
			std::cout << "Adjacent: " << next << std::endl;
			if (predecessors[next] == 0) {
				// TODO: this does not handle back edges to sources
				// A source may have already been visited, but still have
				// predecessor zero.

				// not already encountered next
				predecessors[next] = cur;

				if (sinks.count(next) > 0) {
					// found path from source to sink
					std::cout << "Found path terminating at " << next << std::endl;
					return predecessorPath(next);
				}

				std::cout << "To visit: " << next << std::endl;
				to_visit.push(next);
			}
		}
	}

	std::cout << "No augmenting paths found" << std::endl;
	return std::queue<Arc *>();
}

EdmondsKarp::~EdmondsKarp() { }

} /* namespace flowsolver */
