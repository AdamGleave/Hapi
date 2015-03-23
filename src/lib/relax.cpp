#include <deque>
#include <glog/logging.h>
#include <unordered_map>

#include "arc.h"
#include "relax.h"
#include "residual_network_util.h"

namespace flowsolver {

RELAX::RELAX(ResidualNetwork &g) : g(g) {
	potentials.assign(g.getNumNodes() + 1, 0);

	// note flow is initially zero (default in ResidualNetwork).
	// potentials we initialize to zero explicitly.
}

RELAX::~RELAX() { }

void RELAX::run() {
	// create a pseudoflow satisfying reduced-cost optimality conditions
  init();
  reoptimize();
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

// PRECONDITION: tree_nodes non-empty
RELAX::cut_arcs_iterator::cut_arcs_iterator(RELAX &algo) : algo(algo) {
	node_it = algo.tree_nodes.begin();
	CHECK(node_it != algo.tree_nodes.end()) << "tree nodes empty";
	updateArcsIt();
	nextValid();
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

// if the current arc is valid, no-op.
// otherwise, iterates until it finds the next valid arc.
// may update both node_it and arcs_it, arcs_it_end
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
			Arc *arc = arcs_it->second;
			if (arc->getCapacity() > 0) {
				// zero capacity arcs are not properly in the residual network, ignore
				uint32_t dst_id = arcs_it->first;
				if (algo.tree_nodes.count(dst_id) == 0) {
					// dst_id is not in tree_nodes
					// so we have found an arc crossing the cut
					break;
				}
			}

			// otherwise, keep on looking
			++arcs_it;
		}
	}
}

RELAX::cut_arcs_iterator RELAX::cut_arcs_iterator::operator++() {
	++arcs_it;
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

int64_t RELAX::compute_reduced_cost(Arc *arc, bool allow_negative) {
	uint32_t src_id = arc->getSrcId(), dst_id = arc->getDstId();
	int64_t reduced_cost = arc->getCost()
			                 - potentials[src_id] + potentials[dst_id];
	if (!allow_negative) {
		CHECK_GE(reduced_cost, 0) << "negative reduced cost " << reduced_cost
															<< " on arc " << src_id << "->" << dst_id;
	}
	return reduced_cost;
}

void RELAX::adjust_potential() {
	int64_t minimum_residual_reduced_cost = INT64_MAX;
  for (auto it = beginCutArcs(), end = endCutArcs(); it != end; ++it) {
  	// iterate over all arcs crossing the cut
  	Arc *arc = *it;
  	uint32_t src_id = arc->getSrcId(), dst_id = arc->getDstId();
  	int64_t reduced_cost = arc->getCost()
  			                 - potentials[src_id] + potentials[dst_id];

		if (reduced_cost == 0) {
			// saturate all arcs crossing the cut with zero reduced cost
			g.pushFlow(src_id, dst_id, arc->getCapacity());
		} else if (reduced_cost > 0) {
			minimum_residual_reduced_cost = std::min(reduced_cost,
					                                     minimum_residual_reduced_cost);
		} else {
			// all negative reduced cost arcs must be saturated,
			// so not in residual network
			CHECK(false) << "arc " << src_id << "->" << dst_id
					         << " has negative reduced cost " << reduced_cost;
		}
  }

  CHECK_NE(minimum_residual_reduced_cost, INT64_MAX)
         << "did not find any arc crossing the cut with non-zero reduced cost.";
  for (auto it = tree_nodes.begin(), end = tree_nodes.end();
  		 it != end; ++it) {
  	uint32_t id = *it;
  	potentials[id] += minimum_residual_reduced_cost;
  }
}

void RELAX::adjust_flow(uint32_t src, uint32_t dst) {
	ResidualNetworkUtil::augmentPath(g, src, dst, predecessors);
}

// unused. retained since it could be valuable for debugging (can check
// computed value agrees with that computed online by update_residual_cut)
uint64_t RELAX::compute_residual_cut() {
	uint64_t tree_residual_cut = 0;
	for (auto it = beginCutArcs(), end = endCutArcs(); it != end; ++it) {
		Arc *arc = *it;
		if (compute_reduced_cost(arc) == 0) {
			tree_residual_cut += arc->getCapacity();
		}
	}
	return tree_residual_cut;
}

void RELAX::update_residual_cut(uint32_t new_node) {
	std::unordered_map<uint32_t, Arc*> adjacencies = g.getAdjacencies(new_node);
	for (auto it = adjacencies.begin(), end = adjacencies.end();
			 it != end; ++it) {
		Arc *arc = it->second;

		int64_t reduced_cost = compute_reduced_cost(arc, true);

		if (reduced_cost == 0) {
			uint64_t dst_id = it->first;
			if (tree_nodes.count(dst_id) == 0) {
				// arc is from new_node to non-tree node
				// so adding new_node to tree *increases* residual cut
				tree_residual_cut += arc->getCapacity();
			} else {
				// arc is from new_node to another tree node
				// so adding new_node to tree *decreases* residual cut,
				// since we can no longer include that arc in the sum
				Arc *rev_arc = g.getArc(dst_id, new_node);
				tree_residual_cut -= rev_arc->getCapacity();
			}
		}
	}
}

// SOMEDAY(adam): handle networks with no feasible solutions elegantly
void RELAX::reoptimize() {
	// do this here rather than in constructor, since number of nodes may change
	// between runs. Note DynamicMaintainOptimality will resize potentials for us
	predecessors.resize(g.getNumNodes() + 1);

	const std::set<uint32_t> &sources = g.getSources();
	while (!sources.empty()) {
		// SOMEDAY: can we use a heuristic to choose the source to attain a speedup?
		// e.g. source with most excess

		// select some initial source
		uint32_t source = *sources.begin();

		// we build a tree with the properties that:
		// 1. every node has non-negative excess
		// 2. every arc between tree nodes has zero reduced cost

		// note source satisfies (1) by definition
		// (2) trivially satisfied since no arcs
		tree_nodes.clear();
		tree_nodes.insert(source);

		// tree_excess = sum of balances of nodes in tree
		tree_excess = g.getBalance(source);

		// sum of residual capacity of *zero reduced-cost* arcs from
		// tree to non-tree nodes, i.e. crossing the tree cut.
		tree_residual_cut = 0;
		update_residual_cut(source);

		if (tree_excess > tree_residual_cut) {
			adjust_potential();
			// this may have changed tree_residual_cut, recompute
			tree_residual_cut = 0;
			update_residual_cut(source);
		}

		while (tree_excess <= tree_residual_cut) {
			// build the tree

			// find an arc crossing the cut, with zero reduced cost
			Arc *crossing_arc = NULL;
			for (auto it = beginCutArcs(), end = endCutArcs(); it != end; ++it) {
				Arc *arc = *it;
				if (compute_reduced_cost(arc) == 0) {
					// we've found what we were looking for
					crossing_arc = arc;
					break;
				}
			}

			CHECK_NOTNULL(crossing_arc);

			uint32_t dst_id = crossing_arc->getDstId();
			int64_t balance = g.getBalance(dst_id);
			predecessors[dst_id] = crossing_arc->getSrcId();
			if (balance >= 0) {
				// also an excess node, so can be added to tree
				tree_nodes.insert(dst_id);

				update_residual_cut(dst_id);
				tree_excess += balance;
			} else {
				// cannot build tree any further; adjust flows and end
				adjust_flow(source, dst_id);
				break;
			}
		}

		if (tree_excess > tree_residual_cut) {
			adjust_potential();
		}
	}
}

} /* namespace flowsolver */
