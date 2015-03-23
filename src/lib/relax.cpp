#include <deque>
#include <glog/logging.h>

#include "arc.h"
#include "relax.h"
#include "residual_network_util.h"

namespace flowsolver {

RELAX::RELAX(ResidualNetwork &g)
																						: g(g), num_nodes(g.getNumNodes()) {
	potentials.assign(num_nodes + 1, 0);
	// note flow is initially zero (default in ResidualNetwork),
	// and potentials initialized to constant zero
}

RELAX::~RELAX() { }

// SOMEDAY(adam): potentially returning a big object, will compiler inline this?
std::queue<Arc *> RELAX::predecessorPath
			(uint32_t source, uint32_t sink, const std::vector<uint32_t>& parents) {
	std::deque<Arc *> path;
	uint32_t cur, prev;

	cur = sink;
	while (cur != source) {
		prev = parents[cur];
		Arc *arc = g.getArc(prev, cur);
		path.push_front(arc);
		cur = prev;
	}

	return std::queue<Arc *>(path);
}

void RELAX::init() {
	// We have all zero potential. We need x to be a pseudoflow satisfying the
	// reduced cost optimality conditions, or equivalently an optimal solution
	// to the relaxation objective function. This is achieved by saturating
	// all arcs with negative (reduced) costs.
	for (Arc &arc : g) {
		if (arc.getCost() < 0) {
			g.pushFlow(arc.getSrcId(), arc.getDstId(), arc.getCapacity());
		}
	}
}

void RELAX::run() {
	// create a pseudoflow satisfying reduced-cost optimality conditions
  init();
  reoptimize();
}

// PRECONDITION: tree_nodes non-empty
RELAX::cut_arcs_iterator::cut_arcs_iterator(RELAX &algo) : algo(algo) {
	node_it = algo.tree_nodes.begin();
	updateArcsIt();
}

RELAX::cut_arcs_iterator::cut_arcs_iterator(RELAX &algo, bool) : algo(algo) {
	// end sentinel
	node_it = algo.tree_nodes.end();
}

Arc* RELAX::cut_arcs_iterator::operator*() const {
	return arcs_it->second;
}

void RELAX::cut_arcs_iterator::updateArcsIt() {
	cur_node = *node_it;
	const std::unordered_map<uint32_t, Arc*>& adjacencies
																						= algo.g.getAdjacencies(cur_node);
	arcs_it = adjacencies.begin();
	arcs_it_end = adjacencies.end();
}

void RELAX::cut_arcs_iterator::nextValid() {
	while (true) {
		if (arcs_it == arcs_it_end) {
			// end of arcs for current node, go to next node
			++node_it;
			if (node_it == algo.tree_nodes.end()) {
				// no more tree nodes left to iterate over; end
				break;
			}
			updateArcsIt();
		} else {
			// not yet at end of arcs
			++arcs_it;

			uint32_t dst_id = arcs_it->first;
			if (algo.tree_nodes.count(dst_id) == 0) {
				// dst_id is not in tree_nodes
				// so we have found an arc crossing the cut
				Arc *arc = arcs_it->second;
				int64_t reduced_cost = arc->getCost()
														 - algo.potentials[cur_node]
														 + algo.potentials[dst_id];
				CHECK_GE(reduced_cost, 0);
				if (reduced_cost == 0) {
					// we've found what we were looking for
					break;
				}
			}

			// otherwise, keep on looking
		}
	}
}

RELAX::cut_arcs_iterator RELAX::cut_arcs_iterator::operator++() {
	arcs_it++;
	nextValid();
	return *this;
}

RELAX::cut_arcs_iterator RELAX::cut_arcs_iterator::operator++(int) {
	cut_arcs_iterator old(*this);
	++(*this);
	return old;
}

bool RELAX::cut_arcs_iterator::operator==(const cut_arcs_iterator &it) {
	// N.B. Implicitly assume we're iterating w.r.t. the same algorithm
	if (node_it == it.node_it) {
		if (node_it == algo.tree_nodes.end()) {
			// End iterator not unique, so need this check.
			return true;
		} else {
			return arcs_it == it.arcs_it;
		}
	} else {
		return false;
	}
}

bool RELAX::cut_arcs_iterator::operator!=(const cut_arcs_iterator &it) {
	return !(*this == it);
}

RELAX::cut_arcs_iterator RELAX::beginCutArcs() {
	return cut_arcs_iterator(*this);
}

RELAX::cut_arcs_iterator RELAX::endCutArcs() {
	return cut_arcs_iterator(*this, true);
}

// SOMEDAY(adam): handle networks with no feasible solutions elegantly
void RELAX::reoptimize() {
	const std::set<uint32_t> &sources = g.getSources();
	while (!sources.empty()) {
		// SOMEDAY: can we use a heuristic to choose the source to attain a speedup?
		// e.g. source with most excess

		// select some initial source
		uint32_t source = *sources.begin();

		// we build a tree with the properties that:
		// 1. every node has non-negative excess
		// 2. every arc between tree nodes has zero reduced cost
		std::set<uint32_t> tree_nodes;
		// note source satisfies (1) by definition
		// (2) trivially satisfied since no arcs
		tree_nodes.insert(source);

		// tree_excess = sum of balances of nodes in tree
		uint64_t tree_excess = g.getBalance(source);

		// sum of residual capacity of *zero reduced-cost* arcs from
		// tree to non-tree nodes, i.e. crossing the tree cut.
		uint64_t tree_residual_cut = 0;

		while (true) {
			// select a zero-reduced cost arc crossing the cut
			// TODO
		}
	}
}

} /* namespace flowsolver */
