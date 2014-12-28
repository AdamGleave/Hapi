#include <cassert>
#include <algorithm>

#include <boost/format.hpp>
#include <glog/logging.h>

#include "CostScaling.h"

namespace flowsolver {

int64_t compute_balance(const FlowNetwork &g,
						std::vector<int64_t> initial_supply, uint32_t id) {
	const std::forward_list<Arc *> &adjacencies = g.getAdjacencies(id);
	std::forward_list<Arc *>::const_iterator it;
	int64_t flow_sum = initial_supply[id];
	for (it = adjacencies.begin(); it != adjacencies.end(); ++it) {
		Arc *arc = *it;
		if (arc->getSrcId() == id) {
			// forwards arc
			flow_sum -= arc->getFlow();
		} else if (arc->getDstId() == id) {
			// reverse arc
			flow_sum += arc->getFlow();
		} else {
			assert(false);
		}
	}
	return flow_sum;
}

void check_invariants(const FlowNetwork &g, std::vector<int64_t> initial_supply,
					  bool circulation_expected) {
	bool active_seen = false;
	int64_t balance_sum = 0;
	for (size_t i = 1; i <= g.getNumNodes(); i++) {
		int64_t balance = g.getBalance(i);
		if (balance > 0) {
			active_seen = true;
		}
		int64_t computed_balance = compute_balance(g, initial_supply, i);
		LOG_IF(WARNING, balance != computed_balance)
				<< boost::format("computed balance %ld != actual balance %ld")
				% computed_balance % balance;
		balance_sum += balance;
	}
	LOG_IF(WARNING, balance_sum != 0) << "balance sum " << balance_sum;
	LOG_IF(WARNING, active_seen && circulation_expected)
			<< "active vertex when circulation expected";

	FlowNetwork::const_iterator it;
	for (it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;
		uint64_t capacity = arc.getCapacity();
		uint64_t initial_capacity = arc.getInitialCapacity();
		LOG_IF(WARNING, capacity > initial_capacity)
			<< boost::format("%u->%u: %lu > %lu")
			% arc.getSrcId() % arc.getDstId() % capacity % initial_capacity;
	}
}

CostScaling::CostScaling(FlowNetwork &g) : g(g), epsilon(0),
										   SCALING_FACTOR(2 * g.getNumNodes()) {
	uint32_t num_nodes = g.getNumNodes();

	potentials.resize(num_nodes + 1);
	current_edges.reserve(num_nodes + 1);
}

int64_t CostScaling::reducedCost(Arc &arc, uint32_t src_id) {
	uint32_t dst_id;
	int64_t cost;
	if (arc.getSrcId() == src_id) {
		// forwards arc
		dst_id = arc.getDstId();
		cost = arc.getCost();

	} else if (arc.getDstId() == src_id) {
		// reverse arc
		dst_id = arc.getSrcId();
		cost = -arc.getCost();
	} else {
		assert(false);
		// NOREACH
		return 0;
	}
	return (cost * SCALING_FACTOR) - potentials[src_id] + potentials[dst_id];
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
			cost *= SCALING_FACTOR;
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
bool CostScaling::discharge(uint32_t id) {
	do {
		bool relabel_performed = pushOrUpdate(id);
		if (relabel_performed) {
			return true;
		}
	} while (g.getBalance(id) > 0);

	return false;
}

void CostScaling::refine() {
	uint32_t num_nodes = g.getNumNodes();

	/*** initialization ***/
	// bring all edges in kilter
	for (FlowNetwork::iterator it = g.begin(); it != g.end(); ++it) {
		Arc &arc = *it;
		// TODO: This will always be a forwards arc, can optimize this
		int64_t reduced_cost = reducedCost(arc, arc.getSrcId());
		if (reduced_cost < 0) {
			g.pushFlow(arc, arc.getSrcId(), arc.getCapacity());
		} else if (reduced_cost > 0) {
			g.pushFlow(arc, arc.getDstId(), arc.getFlow());
		}
	}

	current_edges.clear();
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

		prev = vertices.before_begin();
		it = vertices.begin();
		while (it != vertices.end()) {
			uint32_t id = *it;
			bool relabel_performed = false;

			if (g.getBalance(id) > 0) {
				active_seen = true;
				relabel_performed = discharge(id);
			}

			if (relabel_performed) {
				vertices.push_front(id);
				it = vertices.erase_after(prev);
				// note prev remains unchanged: the iterator returned by
				// erase_after(prev) points to the same index in the list as
				// *it had originally (since it erases *it)
			} else {
				prev = it;
				++it;
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
