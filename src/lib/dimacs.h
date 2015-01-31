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

class DIMACSImporter {
protected:
	explicit DIMACSImporter(std::istream &is) : is(is) {};
	virtual void processLine(char type, const char *remainder) = 0;
	void parse() {
		std::string line;

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
			// WARNING: Do NOT 'simplify' this to oss.str().c_str()
			// If you do, oss.str() will be a temporary, and will be deallocated
			// immediately afterwards. This will lead to subtle memory access bugs.
			std::string oss_str = oss.str();
			const char *remainder = oss_str.c_str();

			processLine(type, remainder);
		}
	}

	std::istream &is;
	unsigned int line_num = 0;
};

template<class T>
class DIMACSOriginalImporter : public DIMACSImporter {
	BOOST_CONCEPT_ASSERT((Graph<T>));
public:
	explicit DIMACSOriginalImporter(std::istream &is) : DIMACSImporter(is) {};

	T *read() {
		parse();

		return g;
	}
private:
	void processLine(char type, const char *remainder) final {
		int num_matches = -1;
		switch (type) {
		case 'c':
			// comment line -- ignore
			break;
		case 'p':
			// problem line

			CHECK(!g) << "duplicate problem line at " << line_num;
			CHECK(!seen_node) << "problem line after node line";
			CHECK(!seen_arc) << "problem line after arc line";

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

			CHECK(g) << "node line before problem line";
			CHECK(!seen_arc) << "node line after arc line";

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

	T *g = nullptr;
	std::vector<std::unordered_set<uint32_t>> arcs_seen;
	bool seen_node = false, seen_arc = false;
};

template<class T>
class DIMACSFlowImporter : public DIMACSImporter {
	BOOST_CONCEPT_ASSERT((Graph<T>));
public:
	explicit DIMACSFlowImporter(std::istream &is, T &g) : DIMACSImporter(is),
																										    g(g) {};

	int64_t read() {
		DIMACSImporter::parse();

		return solution;
	}
private:
	void processLine(char type, const char *remainder) final {
		int num_matches = -1;
		switch (type) {
		case 'c':
			// comment line -- ignore;
			break;
		case 's':
		{
			// solution line
			CHECK(!solution_seen) << "duplicate solution at L" << line_num;
			num_matches = sscanf(remainder, "%ld", &solution);
			CHECK_EQ(num_matches, 1);
			solution_seen = true;
			break;
		}
		case 'f':
		{
			// flow assignment
			CHECK(solution_seen) << "flow before solution at L" << line_num;
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

	T &g;
	int64_t solution = 0;
	bool solution_seen = false;
};

template<class T>
class DIMACSExporter {
	BOOST_CONCEPT_ASSERT((Graph<T>));
public:
	DIMACSExporter(const T &g, std::ostream &os) : g(g), os(os) {};

	void write() {
		uint32_t num_nodes = g.getNumNodes();
		uint32_t num_arcs = g.getNumArcs();

		// problem line
		os << boost::format("p min %u %u\n") % num_nodes % num_arcs;

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

	void writeFlow() {
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
private:
	const T &g;
	std::ostream &os;
};

template<class T>
class DIMACSIncrementalImporter : public DIMACSImporter {
	BOOST_CONCEPT_ASSERT((DynamicGraphCallbacks<T>));
public:
	DIMACSIncrementalImporter(std::istream &is, T &g) : DIMACSImporter(is),
			                                                g(g) {};

	void read() {
		DIMACSImporter::parse();
	}
private:
	void processLine(char type, const char *remainder) {
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
			CHECK_EQ(num_matches, 1) << "malformed remove node, at line "
															 << line_num;
			g.removeNode(node_id);
			break;
			}
		case 'n':
			{
			// change supply of a node
			uint32_t id;
			int64_t supply;
			num_matches = sscanf(remainder, "%u %ld", &id, &supply);
			CHECK_EQ(num_matches, 2);

			g.setSupply(id, supply);
			}
			break;
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
			CHECK(arc != nullptr) << "trying to change non-existent arc "
					                  << src << "->" << dst;
			if (upper_bound == 0) {
				g.removeArc(src, dst);
			} else {
				uint64_t current_upper_bound = arc->getCapacity();
				if (current_upper_bound != upper_bound) {
					g.changeArcCapacity(src, dst, upper_bound);
				}
				int64_t current_cost = arc->getCost();
				if (current_cost != cost) {
					g.changeArcCost(src, dst, cost);
				}
			}
			break;
			}
		case 'd':
			{
			// add new node
			CHECK_EQ(arcs_remaining, 0);
			int64_t supply;
			uint64_t potential;
			uint32_t num_arcs;
			num_matches = sscanf(remainder, "%ld %lu %u",
													 &supply, &potential, &num_arcs);
			CHECK_EQ(num_matches, 3);
			// potential not currently used anywhere
			// maintained for backwards compatibility, and in case we wish to hint
			// at potential in the future
			CHECK_EQ(potential, 0);

			new_node_id = g.addNode();
			g.setSupply(new_node_id, supply);

			arcs_remaining = num_arcs;
			break;
			}
		case 'a':
			{
			// add new arc
			uint32_t src, dst;
			uint64_t lower_bound, upper_bound;
			int64_t cost;
			num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
							 &src, &dst, &lower_bound, &upper_bound, &cost);
			CHECK_EQ(num_matches, 5);

			CHECK_EQ(lower_bound, 0);
			if (arcs_remaining > 0) {
				// we're adding arcs for a newly added node
				if (src == 0) {
					src = new_node_id;
				}
				if (dst == 0) {
					dst = new_node_id;
				}
			}

			g.addArc(src, dst, upper_bound, cost);
			arcs_remaining--;
			break;
			}
		}
	}

	T &g;
	unsigned int arcs_remaining = 0;
	uint32_t new_node_id = 0;
};

} /* namespace flowsolver */

#endif /* LIB_DIMACS_H_ */
