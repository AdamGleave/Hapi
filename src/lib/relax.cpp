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

RELAX::iterator RELAX::beginZeroCostCut() {
  return iterator(&tree_cut_arcs_zero_cost, &tree_nodes);
}

RELAX::iterator RELAX::endZeroCostCut() {
  return iterator(&tree_nodes, true);
}

RELAX::const_iterator RELAX::beginZeroCostCut() const {
  return const_iterator(&tree_cut_arcs_zero_cost, &tree_nodes);
}

RELAX::const_iterator RELAX::endZeroCostCut() const {
  return const_iterator(&tree_nodes, true);
}

RELAX::iterator RELAX::beginPositiveCostCut() {
	return iterator(&tree_cut_arcs_positive_cost, &tree_nodes);
}

RELAX::iterator RELAX::endPositiveCostCut() {
	return iterator(&tree_nodes, true);
}

RELAX::const_iterator RELAX::beginPositiveCostCut() const {
  return const_iterator(&tree_cut_arcs_positive_cost, &tree_nodes);
}

RELAX::const_iterator RELAX::endPositiveCostCut() const {
  return const_iterator(&tree_nodes, true);
}

int64_t RELAX::compute_reduced_cost(const Arc &arc, bool allow_negative) {
	uint32_t src_id = arc.getSrcId(), dst_id = arc.getDstId();
	int64_t reduced_cost = arc.getCost()
			                 - potentials[src_id] + potentials[dst_id];
	if (!allow_negative) {
		CHECK_GE(reduced_cost, 0) << "negative reduced cost " << reduced_cost
															<< " on arc " << src_id << "->" << dst_id;
	}
	return reduced_cost;
}

void RELAX::adjust_potential() {
  for (auto it = beginZeroCostCut(), end = endZeroCostCut(); it != end; ++it) {
  	// iterate over all zero reduced cost arcs crossing the cut, saturating them
  	const Arc &arc = *it;
  	g.pushFlow(arc.getSrcId(), arc.getDstId(), arc.getCapacity());
  }

  int64_t minimum_residual_reduced_cost = INT64_MAX;
  for (auto it = beginPositiveCostCut(), end = endPositiveCostCut();
  		 it != end; ++it) {
    // iterative over all positive reduced cost arcs crossing the cut
  	int64_t reduced_cost = compute_reduced_cost(*it);
    minimum_residual_reduced_cost = std::min(reduced_cost,
    		                                     minimum_residual_reduced_cost);
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
	for (auto it = beginZeroCostCut(), end = endZeroCostCut(); it != end; ++it) {
		const Arc &arc = *it;
		tree_residual_cut += arc.getCapacity();
	}
	return tree_residual_cut;
}

void RELAX::update_cut(uint32_t new_node) {
	std::unordered_map<uint32_t, Arc*> adjacencies = g.getAdjacencies(new_node);
	for (auto it = adjacencies.begin(), end = adjacencies.end();
			 it != end; ++it) {
		Arc *arc = it->second;

		int64_t reduced_cost = compute_reduced_cost(*arc, true);
		uint64_t dst_id = it->first;

		if (tree_nodes.count(dst_id) == 0) {
			// arc is from new_node to non-tree node; so add to cut
			int64_t capacity = arc->getCapacity();
			if (capacity > 0) {
				if (reduced_cost == 0) {
					tree_residual_cut += arc->getCapacity();
					tree_cut_arcs_zero_cost[new_node][dst_id] = arc;
				} else {
					tree_cut_arcs_positive_cost[new_node][dst_id] = arc;
				}
			} else {
				// ignore -- not in residual network
			}
		} else {
			// arc is from new_node to another tree node; remove from cut
			Arc *rev_arc = g.getArc(dst_id, new_node);
			int64_t capacity = rev_arc->getCapacity();
			if (capacity > 0) {
				if (reduced_cost == 0) {
					tree_residual_cut -= rev_arc->getCapacity();
					tree_cut_arcs_zero_cost[dst_id].erase(new_node);
				} else {
					tree_cut_arcs_positive_cost[dst_id].erase(new_node);
				}
			} else {
				// ignore -- not in residual network
			}
		}
	}
}

void RELAX::reset_cut() {
	tree_residual_cut = 0;
	for (auto node : tree_nodes) {
	  tree_cut_arcs_zero_cost[node].clear();
	  tree_cut_arcs_positive_cost[node].clear();
	}
}

void RELAX::reoptimize() {
	// do this here rather than in constructor, since number of nodes may change
	// between runs. Note DynamicMaintainOptimality will resize potentials for us
	predecessors.resize(g.getNumNodes() + 1);
	tree_cut_arcs_zero_cost.resize(g.getNumNodes() + 1);
  tree_cut_arcs_positive_cost.resize(g.getNumNodes() + 1);

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
		reset_cut();
		tree_nodes.clear();
		tree_nodes.insert(source);

		// tree_excess = sum of balances of nodes in tree
		tree_excess = g.getBalance(source);

		// sum of residual capacity of *zero reduced-cost* arcs from
		// tree to non-tree nodes, i.e. crossing the tree cut.
		update_cut(source);

		if (tree_excess > tree_residual_cut) {
			adjust_potential();
			// this may have changed tree_residual_cut, recompute
			reset_cut();
			update_cut(source);
		}

		while (tree_excess <= tree_residual_cut) {
			// build the tree

			RELAX::const_iterator it = beginZeroCostCut(),
					                 end = endZeroCostCut();
			for (;it != end; ++it) {
				const Arc &arc = *it;
				if (arc.getCapacity() > 0) {
					break;
				}
			}

			CHECK(it != end) << "no zero reduced cost arc crossing cut";

			const Arc &crossing_arc = *it;

			uint32_t dst_id = crossing_arc.getDstId();
			int64_t balance = g.getBalance(dst_id);
			predecessors[dst_id] = crossing_arc.getSrcId();
			if (balance >= 0) {
				// also an excess node, so can be added to tree
				tree_nodes.insert(dst_id);

				update_cut(dst_id);
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
