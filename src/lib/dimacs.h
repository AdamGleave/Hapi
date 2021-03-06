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
#include <boost/algorithm/string.hpp>

#include "arc.h"
#include "graph.h"

#include "incremental_solver.h"

namespace flowsolver {

class DIMACSImporter {
protected:
	explicit DIMACSImporter(std::istream &is) : is(is) {};
	// return true if more data to read, false if end-of-graph token read
	// (only needed for incremental DIMACS; others can always return true)
	virtual bool processLine(char type, const char *remainder) = 0;
	// returns false if EOF read, true if terminated before EOF
	bool parse() {
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

			bool more_data = processLine(type, remainder);
			if (!more_data) {
				return true;
			}
		}

		// EOF read
		return false;
	}

	std::istream &is;
	unsigned int line_num = 0;
};

template<class T>
class DIMACSOriginalImporter : public DIMACSImporter {
	BOOST_CONCEPT_ASSERT((Graph<T>));
public:
	explicit DIMACSOriginalImporter(std::istream &is,
			                            bool allow_antiparallel_arcs=false,
																	bool allow_duplicates=false)
	    : DIMACSImporter(is), allow_antiparallel_arcs(allow_antiparallel_arcs),
				allow_duplicates(allow_duplicates) {};

	T *read() {
		parse();

		return g;
	}
protected:
	bool processLine(char type, const char *remainder) {
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

			g = new T(num_nodes, num_arcs);
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

			if (!allow_duplicates && arcs_seen[src].count(dst) > 0) {
				LOG(WARNING) << "Duplicate definition of arc "
							 << src << "->" << dst
							 << " at line " << line_num << " -- ignoring.";
			} else if (!allow_antiparallel_arcs && arcs_seen[dst].count(src) > 0) {
				LOG(ERROR) << "Anti-paralell arc "
						       << src << "->" << dst
									 << " at line " << line_num << " -- ignoring.";
			} else {
				if (upper_bound != 0) {
					// ignore zero-capacity arcs
					g->addArc(src, dst, upper_bound, cost);
					arcs_seen[src].insert(dst);
					if (!allow_antiparallel_arcs) {
						arcs_seen[dst].insert(src);
					}
				}
			}

			break;
		default:
			LOG(FATAL) << "Unrecognized type " << type
						 << " at line " << line_num;
		}

		return true;
	}

	T *g = nullptr;
	std::vector<std::unordered_set<uint32_t>> arcs_seen;
	bool seen_node = false, seen_arc = false;
	bool allow_antiparallel_arcs, allow_duplicates;
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
protected:
	virtual bool processLine(char type, const char *remainder) {
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

		return true;
	}

	T &g;
	int64_t solution = 0;
	bool solution_seen = false;
};

template<class T>
class DIMACSFlowPotentialImporter : public DIMACSFlowImporter<T> {
	BOOST_CONCEPT_ASSERT((DynamicGraph<T>));
public:
  DIMACSFlowPotentialImporter(std::istream &is, T &g, IncrementalSolver &solver)
             : DIMACSFlowImporter<T>(is, g), potentials(solver.getPotentials()) {};

protected:
  virtual bool processLine(char type, const char *remainder) override {
  	int sign_seen = 0;
    switch (type) {
    case 'p':
    {
    	uint32_t node_id;
    	int64_t potential;
    	int num_matches = sscanf(remainder, "%u %ld", &node_id, &potential);
    	CHECK_EQ(num_matches, 2);

    	// some solvers have RC(i,j) = C(i,j) - P[i] + P[j]
    	// others convention RC(i,j) = C(i,j) + P[i] - P[j]
    	// This results in different signs for potentials P. Try and detect which
    	// convention is in use. (We use the former one for our solver.)
    	// N.B. We assume all potentials have the same sign. This needn't be the
    	// case, but is for all solvers I've seen.
    	if (sign_seen == 0) {
    		if (potential > 0) {
    			sign_seen = 1;
    		} else if (potential < 0) {
    			sign_seen = -1;
    		}
    	}
			potential = sign_seen * potential;
			CHECK_GE(potential, 0);

    	potentials[node_id] = potential;

    	return true;
    	break;
    }
    default:
    	return DIMACSFlowImporter<T>::processLine(type, remainder);
    	break;
    }
  }

  std::vector<uint64_t> &potentials;
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
protected:
	const T &g;
	std::ostream &os;
};

template<class T>
class DIMACSPotentialExporter : public DIMACSExporter<T> {
public:
	DIMACSPotentialExporter(const T &g, IncrementalSolver &solver, std::ostream &os)
              : DIMACSExporter<T>(g, os), potentials(solver.getPotentials()) {};

	void writePotentials() {
		for (size_t i = 1; i <= this->g.getNumNodes(); i++) {
			std::cout << "p " << i << " " << potentials[i] << std::endl;
		}
	}

protected:
	std::vector<uint64_t> &potentials;
};

template<class T>
class DIMACSIncrementalFullImporter : public DIMACSOriginalImporter<T> {
public:
	explicit DIMACSIncrementalFullImporter(std::istream &is)
	                                           : DIMACSOriginalImporter<T>(is) {};

protected:
	// add support for c EOI as end-of-graph indicator
	bool processLine(char type, const char *remainder) {
		if (type == 'c') {
			std::string comment(remainder);
			// remove whitespace
			boost::trim(comment);

			if (comment == "EOI") {
				return false;
			}
		}

		// if not c EOI, handle normally
		return DIMACSOriginalImporter<T>::processLine(type, remainder);
	}
};

struct DIMACSIncrementalDeltaStatistics {
	unsigned int node_additions, node_deletions;
	unsigned int arc_additions, arc_deletions,
	             arc_cost_changes, arc_cap_changes, arc_changes;
	unsigned int noop_additions, noop_changes, noop_deletions;

	unsigned int getTotalArcMutations() {
		return arc_additions + arc_deletions + arc_changes;
	}

	unsigned int getTotalArcNoOps() {
		return noop_additions + noop_changes + noop_deletions;
	}

	unsigned int getTotalArcEvents() {
		return getTotalArcMutations() + getTotalArcNoOps();
	}

	unsigned int getTotalNodes() {
		return node_additions + node_deletions;
	}
protected:
	void reset() {
		node_additions = node_deletions = 0;
		arc_additions = arc_changes = arc_deletions = 0;
		arc_cap_changes = arc_cost_changes = 0;
		noop_additions = noop_changes = noop_deletions = 0;
	}

	template<class T>
	friend class DIMACSIncrementalDeltaImporter;
};

template<class T>
class DIMACSIncrementalDeltaImporter : public DIMACSImporter {
	BOOST_CONCEPT_ASSERT((DynamicGraphCallbacks<T>));
public:
	typedef std::function<void(void)> callback;
	DIMACSIncrementalDeltaImporter(std::istream &is, T &g,
			                           callback new_delta=callback())
	        : DIMACSImporter(is), g(g), new_delta_callback(new_delta) {};

	bool read() {
		new_delta = false;
		return DIMACSImporter::parse();
	}

	DIMACSIncrementalDeltaStatistics &getStatistics() {
		return statistics;
	}

	void resetStatistics() {
		statistics.reset();
	}
private:
	const static uint32_t SINK_NODE = 1;

	void adjustSinkCapacity(int64_t delta) {
		int64_t sink_supply = g.getSupply(SINK_NODE);
		CHECK_LE(g.getSupply(SINK_NODE), 0);
		g.setSupply(SINK_NODE, sink_supply + delta);
	}

	bool processLine(char type, const char *remainder) {
		if (!new_delta) {
			new_delta = true;
			if (new_delta_callback) {
				new_delta_callback();
			}
		}
		int num_matches = -1;
		switch (type) {
		case 'c':
			{
			// is it end of graph indicator?
			std::string comment(remainder);
			// remove whitespace
			boost::trim(comment);

			if (comment == "EOI") {
				return false;
			}
			// otherwise can ignore comments
			break;
			}
		case 'r':
			{
			// remove node
			uint32_t node_id;
			num_matches = sscanf(remainder, "%u", &node_id);
			CHECK_EQ(num_matches, 1) << "malformed remove node, at line "
															 << line_num;
			adjustSinkCapacity(g.getSupply(node_id));
			g.removeNode(node_id);

			statistics.node_deletions++;
			break;
			}
		case 'n':
			{
			// add a node
			uint32_t id;
			int64_t supply;
			// node type also present as third column, but we ignore this
			num_matches = sscanf(remainder, "%u %ld", &id, &supply);
			CHECK_EQ(num_matches, 2);

			g.addNode(id);
			g.setSupply(id, supply);

			// TODO: This is a bit of a hack - should I keep this?
			// Firmament doesn't export changes in sink demand. To keep the problem
			// balanced, increase demand at the sink whenever we add a new node.
			CHECK_GE(supply, 0) << "only one node allowed to be a sink.";
			// increase demand at sink by parameter supply
			adjustSinkCapacity(-supply);

			statistics.node_additions++;
			}
			break;
		case 'x':
			{
			// change of an existing arc;
			// could be either cost or capacity, or both
			uint32_t src, dst;
			uint64_t lower_bound, upper_bound;
			int64_t cost;
			num_matches = sscanf(remainder, "%u %u %lu %lu %ld",
							 &src, &dst, &lower_bound, &upper_bound, &cost);
			CHECK_EQ(num_matches, 5);

			CHECK_EQ(lower_bound, 0);
			Arc *arc = g.getArc(src, dst);
			if (arc == nullptr) {
				// SOMEDAY: Decide whether Firmament should change or not.
				// Firmament considers arcs with a zero upper bound to still be arcs.
				// We do not. So we have to special-case this.
				if (upper_bound == 0) {
					// Firmament sometimes generates arc changes when nothing has changed
					// This is the case "the arc had zero capacity before, and
					// "still doesn't". That is, it wasn't an arc before, and still isn't.
					LOG(WARNING) << "ignoring delete of non-existent arc "
							         << src << "->" << dst;

					statistics.noop_deletions++;
				} else {
					LOG(WARNING) << "converting change of non-existent arc "
											 << src << "->" << dst << " to an add";
					CHECK_EQ(lower_bound, 0);
					g.addArc(src, dst, upper_bound, cost);

					statistics.arc_additions++;
				}
				return true;
			} else {
				if (upper_bound == 0) {
					g.removeArc(src, dst);
					statistics.arc_deletions++;
				} else {
					bool done_something = false;
					uint64_t current_upper_bound = arc->getCapacity();
					if (current_upper_bound != upper_bound) {
						g.changeArcCapacity(src, dst, upper_bound);
						done_something = true;
						statistics.arc_cap_changes++;
					}
					int64_t current_cost = arc->getCost();
					if (current_cost != cost) {
						g.changeArcCost(src, dst, cost);
						done_something = true;
						statistics.arc_cost_changes++;
					}
					if (done_something) {
						statistics.arc_changes++;
					} else {
						LOG(WARNING) << "no-op arc change event " << src << "->" << dst;
						statistics.noop_changes++;
					}
				}
			}
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

			if (upper_bound == 0) {
				LOG(WARNING) << "ignoring add of arc " << src << "->"
						         << dst << " with zero upper-bound.";
				statistics.noop_additions++;
			} else {
				g.addArc(src, dst, upper_bound, cost);
				statistics.arc_additions++;
			}
			break;
			}
		}

		return true;
	}

	T &g;
	DIMACSIncrementalDeltaStatistics statistics;
	bool new_delta;
	callback new_delta_callback;
};

} /* namespace flowsolver */

#endif /* LIB_DIMACS_H_ */
