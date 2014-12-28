#include <cassert>
#include <algorithm>

#include "CostScaling.h"

namespace flowsolver {

CostScaling::CostScaling(FlowNetwork &g) : g(g) {
	uint32_t num_nodes = g.getNumNodes();

	potentials.resize(num_nodes + 1);
	current_edges.reserve(num_nodes + 1);
	epsilon = 0;
}

int64_t CostScaling::reducedCost(Arc &arc, uint32_t src_id) {
	if (arc.getSrcId() == src_id) {
		// forwards arc
		uint32_t dst_id = arc.getDstId();
		return arc.getCost() - potentials[src_id] + potentials[dst_id];
	} else if (arc.getDstId() == src_id) {
		// reverse arc
		uint32_t dst_id = arc.getSrcId();
		return -arc.getCost() - potentials[src_id] + potentials[dst_id];
	} else {
		assert(false);
		// NOREACH
		return 0;
	}
}

void CostScaling::relabel(uint32_t id) {
	std::forward_list<Arc *> &adjacencies = g.getAdjacencies(id);
	std::forward_list<Arc *>::iterator it;
	int64_t new_potential = INT64_MAX;
	for (it = adjacencies.begin(); it != adjacencies.end(); ++it) {
		Arc *arc = *it;

		int64_t cost;
		uint32_t dst_id;
		uint64_t capacity;
		if (arc->getSrcId() == id) {
			// forwards arc
			dst_id = arc->getDstId();
			cost = arc->getCost();
			capacity = arc->getCapacity();
		} else if (arc->getDstId() == id) {
			// reverse arc
			dst_id = arc->getSrcId();
			cost = -arc->getCost();
			// TODO: This logic is duplicated from inside FlowNetwork
			capacity = arc->getInitialCapacity() - arc->getCapacity();
		} else {
			assert(false);
		}

		if (capacity > 0) {
			// arc is in residual network
			int64_t potential = potentials[dst_id] + cost + epsilon;
			new_potential = std::min(new_potential, potential);
		}
	}
	potentials[id] = new_potential;
}

// precondition: id is active
// returns true if relabel occurs
bool CostScaling::pushOrUpdate(uint32_t id) {
	Arc &current_edge = **current_edges[id];

	// if push is applicable, then apply it
	// precondition for push is: id active (always satisfied);
	// positive residual capacity; negative reduced cost
	int64_t residual_capacity = g.getResidualCapacity(current_edge, id);
	if (residual_capacity > 0) {
		int64_t reduced_cost = reducedCost(current_edge, id);
		if (reduced_cost < 0) {
			// apply push
			uint64_t flow = std::min(residual_capacity, g.getBalance(id));
			g.pushFlow(current_edge, id, flow);
			return false;
		}
	}

	// push not applicable
	std::forward_list<Arc *> &adjacencies = g.getAdjacencies(id);
	++current_edges[id];
	if (current_edges[id] != adjacencies.end()) {
		// not the last edge in list
		return false;
	} else {
		// last edge in the list
		// wrap-around to first edge in list, and relabel the vertex
		current_edges[id] = adjacencies.begin();
		relabel(id);
		return true;
	}
}


// precondition: *it is active
bool CostScaling::discharge(uint32_t id,
		 	 	 	 	 	std::forward_list<uint32_t>::iterator before) {
	bool relabel_performed = false;
	do {
		relabel_performed = pushOrUpdate(id);
		if (relabel_performed) {
			vertices.erase_after(before);
			vertices.push_front(id);
			break;
		}
	} while (g.getBalance(id) > 0);

	return relabel_performed;
}

void CostScaling::refine() {
	uint32_t num_nodes = g.getNumNodes();

	/*** initialization ***/
	// bring all edges in kilter
	for (FlowNetwork::iterator it = g.begin(); it != g.end(); ++it) {
		Arc &arc = *it;
		// TODO: This will always be a forwards arc, can optimize this
		if (reducedCost(arc, arc.getSrcId()) < 0) {
			g.pushFlow(arc, arc.getSrcId(), arc.getCapacity());
		}
	}

	// initialize current edge to first edge in list
	for (size_t i = 0; i < num_nodes + 1; i++) {
		std::forward_list<Arc *> &adjacencies = g.getAdjacencies(i);
		current_edges.push_back(adjacencies.begin());
	}

	// vertices must maintain the invariant that it is in topological order
	// relative to the admissible network. However, initially the admissible
	// network has no edges, and so any initial permutation is permissible.
	// We choose a sequential one for simplicity.
	vertices.clear();
	for (size_t i = 0; i < num_nodes; i++) {
		vertices.push_front(num_nodes - i);
	}

	/*** main loop */
	std::forward_list<uint32_t>::iterator it, prev;
	bool active_seen;
	do {
		active_seen = false;
		for (prev = vertices.before_begin(), it = vertices.begin();
			 it != vertices.end();
			 prev = it, ++it) {
			uint32_t id = *it;
			if (g.getBalance(id) > 0) {
				active_seen = true;
				bool relabel_performed = discharge(id, prev);
				if (relabel_performed) {
					// must perform a fresh pass from beginning of
					// the vertices list
					break;
				}
			}
		}
	} while (active_seen);
}


bool costCompare(Arc &i, Arc &j) {
	return i.getCost() < j.getCost();
}

bool CostScaling::run() {
	// initialize epsilon to C, max cost
	FlowNetwork::const_iterator max_cost;
	max_cost = std::max_element(g.begin(), g.end(), costCompare);
	epsilon = max_cost->getCost();

	// TODO: presentation in paper makes a call to a max flow algorithm here
	// This is unnecessary, as refine subroutine will work OK with any
	// pseudoflow, but has the advantage of verifying feasibility of the problem.
	// I've opted to instead check the amount to which the potential increases
	// to determine feasibility. Worth seeing if there's any performance
	// benefit to starting with a max-flow algorithm, however.

	refine();
	// potential increase is at most 3*num_nodes*epsilon iff. there is a
	// feasible flow
	uint32_t num_nodes = g.getNumNodes();
	for (size_t i = 1; i <= num_nodes; i++) {
		// note potential initially zero
		if (potentials[i] > 0 &&
		    (uint64_t)potentials[i] > 3*num_nodes*epsilon) {
			return false;
		}
	}

	while (epsilon > 1) {
		uint64_t new_epsilon = epsilon / 2;
		// TODO: Does this actually matter?
		if (new_epsilon * 2 < epsilon) {
			new_epsilon++;
		}
		epsilon = new_epsilon;
		refine();
	}
	return true;
}

CostScaling::~CostScaling() {
}

} /* namespace flowsolver */
