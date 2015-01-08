#include "cost_scaling.h"

#include <cassert>
#include <algorithm>
#include <limits>

#include <boost/format.hpp>
#include <glog/logging.h>


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

CostScaling::CostScaling(FlowNetwork &g) : g(g), epsilon(0), num_iterations(0),
							total_cost(0), SCALING_FACTOR(2 * g.getNumNodes()) {
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
		int64_t reduced_cost = (arc.getCost() * SCALING_FACTOR)
					- potentials[arc.getSrcId()] + potentials[arc.getDstId()];
		if (reduced_cost < 0) {
			uint64_t capacity = arc.getCapacity();
			if (capacity > 0) {
				g.pushFlow(arc, arc.getSrcId(), arc.getCapacity());
			}
		} else if (reduced_cost > 0) {
			int64_t flow = arc.getFlow();
			if (flow > 0) {
				g.pushFlow(arc, arc.getDstId(), arc.getFlow());
			}
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

bool CostScaling::run(std::function<bool()> continue_running) {
	// initialize epsilon to C, max cost
	FlowNetwork::const_iterator max_cost;
	max_cost = std::max_element(g.begin(), g.end(), costCompare);
	epsilon = max_cost->getCost() * SCALING_FACTOR;

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

	while (epsilon > 1 && continue_running()) {
		std::cout << "epsilon: " << epsilon << std::endl;
		/*
		 * The below computation will decrease epsilon by (slightly) more than
		 * a factor of two when epsilon is odd, since integer division truncates
		 * (rounds towards zero).
		 *
		 * The asymptotic complexity bound continues to hold, however, so this
		 * is harmless. Using the Goldberg & Tarjan (1987) paper, by lemma 6.2
		 * the maximum price update per vertex is n*(old_epsilon + new_epsilon).
		 * Applying lemma 5.2 (price increases by at least epsilon per relabel),
		 * we arrive at the number of relabels being at most
		 * n^2*(old_epsilon + new_epsilon)/new_epsilon.
		 *
		 * new_epsilon is at most old_epsilon/2 and at least (old_epsilon - 1)/2.
		 * So the number of relabels is at most
		 * n^2*3/2*old_epsilon/[(old_epsilon-1)/2] = n^2*3*old_epsilon/(old_epsilon-1).
		 *
		 * For most rounds this is practically identical to 3n^2, the bound
		 * given in the paper. For the lattermost round when old_epsilon=2, this
		 * increases to 6n^2, however this still does not change the asymptotic
		 * result.
		 */
		epsilon = std::max(1ul, epsilon / 2);
		refine();
		num_iterations++;
	}
	return true;
}

bool CostScaling::runOptimal() {
	return run([]() -> bool { return true; });
}

bool CostScaling::runEpsilonOptimal(double threshold) {
	uint64_t epsilon_threshold = threshold * SCALING_FACTOR;
	return run([epsilon_threshold, this]()
			    -> bool { return epsilon > epsilon_threshold; });
}

bool CostScaling::runFixedIterations(uint64_t max_iterations) {
	return run([max_iterations, this]()
				-> bool { return num_iterations < max_iterations; });
}

uint64_t totalCost(FlowNetwork &g) {
	uint64_t total_cost = 0;
	FlowNetwork::const_iterator it;
	for (it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;

		int64_t flow = arc.getFlow();
		total_cost += arc.getCost() * flow;
	}

	return total_cost;
}

bool CostScaling::costThreshold(double minimum_factor) {
	uint64_t new_total_cost = totalCost(g);
	// usually positive, but can be negative
	int64_t cost_reduction = total_cost - new_total_cost;
	std::cerr << "cost reduced by " << cost_reduction << std::endl;
	double proportion_reduced = (double)cost_reduction / (double)total_cost;

	total_cost = new_total_cost;

	// FIXME: hack, sign of proportion_reduced is important.
	// Empirically, find cost bounces around first few rounds, then settles
	// down. If we used price refinement heuristic, I think this wouldn't happen.
	// Alternatively, could keep track of total cost of several previous rounds
	// and only terminate if not reduced in last k rounds (for some constant k.)
	return fabs(proportion_reduced) > minimum_factor;
}

bool CostScaling::runCostThreshold(double minimum_factor) {
	total_cost =  INT64_MAX;
	return run([minimum_factor, this]()
			   -> bool { return costThreshold(minimum_factor); });
}

CostScaling::~CostScaling() {
}

} /* namespace flowsolver */