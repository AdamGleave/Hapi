/*
 * DIMACS.cpp
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/format.hpp>

#include "DIMACS.h"
#include "Arc.h"

namespace flowsolver {

ResidualNetwork &DIMACS::readDIMACSMin(std::istream &is) {
	std::string line;

	ResidualNetwork *g = 0;
	bool seen_node = false, seen_arc = false;
	while (getline(is, line)) {
		// TODO: how does this respond to blank lines in the file?

		std::istringstream iss (line);

		std::string first;
		iss >> first;
		assert(first.length() == 1);
		char type = first[0];

		std::ostringstream oss;
		oss << iss.rdbuf();
		const char *remainder = oss.str().c_str();

		int num_matches = -1;
		switch (type) {
		case 'c':
			// comment line -- ignore;
			break;
		case 'p':
			// problem line

			// problem line must appear exactly once in file
			assert(!g);
			// and before any node or arc descriptor lines
			assert(!(seen_node || seen_arc));

			char problem[4];
			uint32_t num_nodes, num_arcs;
			num_matches = sscanf(remainder, "%3s %u %u",
								 problem, &num_nodes, &num_arcs);
			assert(num_matches == 3);
			assert(strcmp(problem, "min") == 0);

			g = new ResidualNetwork(num_nodes);
			break;
		case 'n':
			// node descriptor line

			// all lines of this type must appear before arc descriptor lines,
			// and after the problem line
			assert(g && !seen_arc);

			seen_node = true;
			uint32_t id;
			int64_t supply;
			num_matches = sscanf(remainder, "%u %ld", &id, &supply);
			assert(num_matches == 2);

			if (g->getSupply(id) != 0) {
				std::cerr << "WARNING: DIMACS redefines supply of node "
						  << id << std::endl;
			}

			g->setSupply(id, supply);
			break;
		case 'a':
			// arc descriptor line

			// must appear after problem line (and any node descriptor lines)
			assert(g);

			seen_arc = true;
			uint32_t src, dst;
			uint64_t lower_bound, upper_bound;
			int64_t cost;
			num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
						   &src, &dst, &lower_bound, &upper_bound, &cost);
			assert(num_matches == 5);

			assert(lower_bound == 0);

			if (g->getArc(src,dst) != 0) {
				std::cerr << "WARNING: DIMACS attempts to redefine arc "
						  << src << "->" << dst << std::endl;
			} else {
				g->addEdge(src, dst, upper_bound, cost);
			}

			break;
		default:
			assert(false);
		}
	}

	return *g;
}

void DIMACS::writeDIMACSMin(const ResidualNetwork &g, std::ostream &os) {
	uint32_t num_nodes = g.getNumNodes();
	uint32_t num_arcs = g.getNumArcs();

	// problem line
	printf("p min %u %u\n", num_nodes, num_arcs);

	// node descriptor lines
	for (uint32_t id = 1; id <= num_nodes; ++id) {
		int64_t supply = g.getSupply(id);
		if (supply != 0) {
			os << boost::format("n %u %ld\n") % id % supply;
		}
	}

	// arc descriptor lines
	for (ResidualNetwork::const_iterator it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;
		os << boost::format("a %u %u 0 %lu %lu\n")
		 % arc.getSrcId() % arc.getDstId() % arc.getCapacity() % arc.getCost();
	}
}

void DIMACS::writeDIMACSMinFlow(const ResidualNetwork &g, std::ostream &os) {
	ResidualNetwork::const_iterator it;

	uint64_t total_cost = 0;
	for (it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;

		int64_t flow = arc.getFlow();
		if (flow > 0) {
			// ignore negative flows
			total_cost += arc.getCost() * flow;
		}
	}
	os << boost::format("s %lu\n") % total_cost;

	for (it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;
		int64_t flow = arc.getFlow();
		if (flow > 0) {
			os << boost::format("f %u %u %lu\n")
					  % arc.getSrcId() % arc.getDstId() % arc.getFlow();
		}
	}
}

} /* namespace flowsolver */
