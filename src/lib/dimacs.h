/*
 * DIMACS.h
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#ifndef LIB_DIMACS_H_
#define LIB_DIMACS_H_

#include <cstring>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <sstream>
#include <string>

#include <glog/logging.h>
#include <boost/format.hpp>
#include <boost/concept/assert.hpp>

#include "arc.h"
#include "graph.h"

namespace flowsolver {

template<class T>
class DIMACS {
	BOOST_CONCEPT_ASSERT((Graph<T>));
	DIMACS() = delete;

public:
	static T *read(std::istream &is) {
		unsigned int line_num = 0;
		std::string line;

		T *g = 0;
		std::vector<std::unordered_set<uint32_t>> arcs_seen;
		bool seen_node = false, seen_arc = false;
		while (getline(is, line)) {
			line_num++;

			std::istringstream iss (line);

			std::string first;
			iss >> first;
			CHECK(first.length() == 1);
			char type = first[0];

			std::ostringstream oss;
			oss << iss.rdbuf();
			const char *remainder = oss.str().c_str();

			int num_matches = -1;
			switch (type) {
			case 'c':
				// comment line -- ignore
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
				CHECK(num_matches == 3);
				CHECK(strcmp(problem, "min") == 0);

				g = new T(num_nodes);
				arcs_seen.resize(num_nodes + 1);
				break;
			case 'n':
				// node descriptor line

				// all lines of this type must appear before arc descriptor
				// lines, and after the problem line
				CHECK(g && !seen_arc);

				seen_node = true;
				uint32_t id;
				int64_t supply;
				num_matches = sscanf(remainder, "%u %ld", &id, &supply);
				CHECK(num_matches == 2);

				LOG_IF(ERROR, g->getBalance(id) != 0)
					<< "Duplicate definition of node " << id
					<< " at line " << line_num;

				g->setSupply(id, supply);
				break;
			case 'a':
				// arc descriptor line

				// must appear after problem line (and node descriptor lines)
				CHECK(g);

				seen_arc = true;
				uint32_t src, dst;
				uint64_t lower_bound, upper_bound;
				int64_t cost;
				num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
							   &src, &dst, &lower_bound, &upper_bound, &cost);
				CHECK(num_matches == 5);

				CHECK(lower_bound == 0);

				if (arcs_seen[src].count(dst) > 0 ||
					arcs_seen[dst].count(src) > 0) {
					LOG(WARNING) << "Duplicate definition of arc "
								 << src << "->" << dst
								 << " at line " << line_num;
				} else {
					if (upper_bound != 0) {
						// ignore zero-capacity arcs
						g->addArc(src, dst, upper_bound, cost);
						arcs_seen[src].insert(dst);
						arcs_seen[dst].insert(src);
					}
				}

				break;
			default:
				LOG(FATAL) << "Unrecognized type " << type
				 	 	   << " at line " << line_num;
			}
		}

		return g;
	}

	/*
	 * Returns cost of the solution on success, -1 on failure.
	 * Side-effect: g updated to contain the flows in is.
	 */
	static int64_t readFlow(std::istream &is, T &g) {
		unsigned int line_num = 0;
		std::string line;

		int64_t solution = -1;
		while (getline(is, line)) {
			line_num++;

			std::istringstream iss (line);
			std::string first;
			iss >> first;
			CHECK_EQ(first.length(), 1)
				<< "type not single character at L" << line_num;
			char type = first[0];

			std::ostringstream oss;
			oss << iss.rdbuf();
			const char *remainder = oss.str().c_str();

			int num_matches = -1;
			switch (type) {
			case 'c':
				// comment line -- ignore;
				break;
			case 's':
			{
				// solution line
				CHECK_EQ(solution, -1) << "duplicate solution at L" << line_num;
				uint64_t min_cost;
				num_matches = sscanf(remainder, "%lu", &min_cost);
				solution = (int64_t)min_cost;
				CHECK_EQ(num_matches, 1);
				break;
			}
			case 'f':
			{
				// flow assignment
				CHECK_NE(solution, -1) << "flow before solution at L" << line;
				uint32_t src_id, dst_id;
				int64_t flow;
				num_matches = sscanf(remainder, "%u %u %ld",
									 &src_id, &dst_id, &flow);
				CHECK_EQ(num_matches, 3);
				Arc *arc = g.getArc(src_id, dst_id);
				CHECK(arc != 0) << "arc " << src_id << "->" << dst_id
								   << "has flow but not in original network";
				g.pushFlow(*arc, src_id, flow);
				break;
			}
			default:
				LOG(FATAL) << "Unrecognized type " << type
				 	 	   << " at line " << line_num;
				break;
			}
		}

		return solution;
	}

	static void write(const T &g, std::ostream &os) {
		uint32_t num_nodes = g.getNumNodes();
		uint32_t num_arcs = g.getNumArcs();

		// problem line
		printf("p min %u %u\n", num_nodes, num_arcs);

		// node descriptor lines
		for (uint32_t id = 1; id <= num_nodes; ++id) {
			int64_t supply = g.getBalance(id);
			if (supply != 0) {
				os << boost::format("n %u %ld\n") % id % supply;
			}
		}

		// arc descriptor lines
		typename T::const_iterator it;
		for (it = g.begin(); it != g.end(); ++it) {
			const Arc &arc = *it;
			uint64_t capacity = arc.getCapacity();
			if (capacity != 0) {
				os << boost::format("a %u %u 0 %lu %lu\n")
					 % arc.getSrcId() % arc.getDstId()
					 % capacity % arc.getCost();
			}
		}
	}

	static void writeFlow(const T &g, std::ostream &os) {
		typename T::const_iterator it;

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
};

template<class T>
class IncrementalDIMACS {
	BOOST_CONCEPT_ASSERT((Graph<T>));
	IncrementalDIMACS() = delete;
public:
	static void readIncremental(std::istream &is, T &g) {
		unsigned int line_num = 0;
		std::string line;

		unsigned int arcs_remaining = 0;
		uint32_t new_node_id = 0;
		// TODO (adam): reduce code duplication?
		// TODO (adam): do we want this to directly update graph,
		// or e.g. build list of changes?
		while ((getline(is, line))) {
			line_num++;

			std::istringstream iss (line);

			std::string first;
			iss >> first;
			CHECK(first.length() == 1);
			char type = first[0];

			std::ostringstream oss;
			oss << iss.rdbuf();
			const char *remainder = oss.str().c_str();

			int num_matches = -1;
			switch (type) {
			case 'c':
				// comment line -- ignore
				break;
			case 'r':
				{
				// remove node
				CHECK_EQ(arcs_remaining, 0);
				uint32_t node_id;
				num_matches = sscanf(remainder, "%u", &node_id);
				CHECK_EQ(num_matches, 1) << "malformed remove node at line " << line_num;
				g.removeNode(node_id);
				break;
				}
			case 'x':
				{
				// change of an existing arc;
				// could be either cost or capacity, or both
				CHECK_EQ(arcs_remaining, 0);
				uint32_t src, dst;
				uint64_t lower_bound, upper_bound;
				int64_t cost;
				num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
								 &src, &dst, &lower_bound, &upper_bound, &cost);
				CHECK_EQ(num_matches, 5);

				CHECK_EQ(lower_bound, 0);
				Arc *arc = g.getArc(src, dst);
				CHECK_NOTNULL(arc);
				CHECK_EQ(arc->getSrcId(), src) << "arc specified in reverse direction?";

				arc->setCapacity(upper_bound);
				arc->setCost(cost);
				break;
				}
			case 'd':
				{
				// add new node
				CHECK_EQ(arcs_remaining, 0);
				int64_t supply;
				// TODO (adam): why do we need potential of a node?
				uint64_t potential;
				uint32_t num_arcs;
				num_matches = sscanf(remainder, "%ld %lu %u",
						                 &supply, &potential, &num_arcs);
				CHECK_EQ(num_matches, 3);
				CHECK_EQ(potential, 0);

				new_node_id = g.addNode();
				g.setSupply(new_node_id, supply);

				arcs_remaining = num_arcs;
				break;
				}
			case 'a':
				{
				// add new arc
				CHECK_GT(arcs_remaining, 0) << "too many arcs for new node, at line "
																	  << line_num;
				uint32_t src, dst;
				uint64_t lower_bound, upper_bound;
				int64_t cost;
				num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
								 &src, &dst, &lower_bound, &upper_bound, &cost);
				CHECK_EQ(num_matches, 5);

				CHECK_EQ(lower_bound, 0);
				if (src == 0) {
					src = new_node_id;
				}
				if (dst == 0) {
					dst = new_node_id;
				}

				// TODO(adam): detect duplicate arcs?
				g.addArc(src, dst, upper_bound, cost);
				arcs_remaining--;
				break;
				}
			}
		}
	}
};

} /* namespace flowsolver */

#endif /* LIB_DIMACS_H_ */
