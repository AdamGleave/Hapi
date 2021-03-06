#include "cost_scaling.h"

#include <cassert>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>

#include <boost/format.hpp>
#include <glog/logging.h>

// this code is useful for debug, but not used otherwise
/*
namespace {

using namespace flowsolver;

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

} // namespace (unnamed)
*/

namespace flowsolver {

CostScaling::CostScaling(FlowNetwork &g, uint32_t scaling_factor)
	: g(g), epsilon(0), num_iterations(0),
	  SCALING_FACTOR(scaling_factor),
	  COST_SCALING_FACTOR(scaling_factor * g.getNumNodes()) {
	assert(scaling_factor > 1);
	uint32_t num_nodes = g.getNumNodes();

	potentials.resize(num_nodes + 1);
	current_edges.reserve(num_nodes + 1);
}

CostScaling::CostScaling(FlowNetwork &g) : CostScaling(g, 2) {}

CostScaling::~CostScaling() {}

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
	return (cost * COST_SCALING_FACTOR) - potentials[src_id] + potentials[dst_id];
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
		} else {
		  // reverse arc
		  assert(arc->getDstId() == id);
			dst_id = arc->getSrcId();
			cost = -arc->getCost();
			capacity = arc->getInitialCapacity() - arc->getCapacity();
		}

		if (capacity > 0) {
			// arc is in residual network
			cost *= COST_SCALING_FACTOR;
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
			int64_t flow = std::min(residual_capacity, g.getBalance(id));
			int64_t new_balance = g.pushFlow(current_edge, id, flow);
			if (new_balance > 0 && new_balance <= flow) {
				// destination vertex is active, and excess is less than flow;
				// so this push just *made* it active.
				active_vertices.push(current_edge.getOppositeId(id));
			}
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
void CostScaling::discharge(uint32_t id) {
	do {
		bool relabel_performed = pushOrUpdate(id);
		if (relabel_performed) {
			// relabel doesn't change potential, so id is still active
			active_vertices.push(id);
			return;
		}
	} while (g.getBalance(id) > 0);
}

void CostScaling::refine() {
	uint32_t num_nodes = g.getNumNodes();

	/*** initialization ***/
	// bring all edges in kilter
	for (FlowNetwork::iterator it = g.begin(); it != g.end(); ++it) {
		Arc &arc = *it;
		int64_t reduced_cost = (arc.getCost() * COST_SCALING_FACTOR)
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
	for (uint32_t id = 0; id <= num_nodes; id++) {
		std::forward_list<Arc *> &adjacencies = g.getAdjacencies(id);
		current_edges.push_back(adjacencies.begin());
	}

	// build list of active vertices
	CHECK(active_vertices.empty())
		<< "refine should only have terminated when no active vertices.";
	for (uint32_t id = 1; id <= num_nodes; id++) {
		if (g.getBalance(id) > 0) {
			active_vertices.push(id);
		}
	}

	/*** main loop */
	while (!active_vertices.empty()) {
		uint32_t active = active_vertices.front();
		active_vertices.pop();
		discharge(active);
	}
}

bool costCompare(Arc &i, Arc &j) {
	return i.getCost() < j.getCost();
}

bool CostScaling::run(std::function<bool()> continue_running) {
	// initialize epsilon to C, max cost
	FlowNetwork::const_iterator max_cost;
	max_cost = std::max_element(g.begin(), g.end(), costCompare);
	epsilon = max_cost->getCost() * COST_SCALING_FACTOR;
	// It's possible for the maximum cost to be zero. But refine will loop forever
	// with epsilon = 0; epsilon = 1 still guarantees optimality.
	epsilon = std::max(epsilon, (uint64_t)1);

	// TODO: presentation in paper makes a call to a max flow algorithm here
	// This is unnecessary, as refine subroutine will work OK with any
	// pseudoflow, but has the advantage of verifying feasibility of the problem.
	// I've opted to instead check the amount to which the potential increases
	// to determine feasibility. Worth seeing if there's any performance
	// benefit to starting with a max-flow algorithm, however.

	refine();
	// potential increase is less than 3*num_nodes*epsilon iff. there is a
	// feasible flow
	uint32_t num_nodes = g.getNumNodes();
	for (size_t i = 1; i <= num_nodes; i++) {
		// note potential initially zero
		if (potentials[i] > 0 &&
		    (uint64_t)potentials[i] > 3*num_nodes*epsilon) {
			return false;
		}
	}

	// N.B. Important this is always executed: side-effects are needed for e.g.
	// runStatistics.
	bool carry_on = continue_running();
	while (epsilon > 1 && carry_on) {
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
		epsilon = std::max(1ul, epsilon / SCALING_FACTOR);
		refine();
		num_iterations++;
		carry_on = continue_running();
	}
	return true;
}

bool CostScaling::runOptimal() {
	return run([]() -> bool { return true; });
}

bool CostScaling::runEpsilonOptimal(double threshold) {
	uint64_t epsilon_threshold = threshold * COST_SCALING_FACTOR;
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

class CostDelta {
	int64_t total_cost;
public:
	CostDelta() {
		total_cost = UINT64_MAX;
	}

	double update(FlowNetwork &g) {
		uint64_t new_total_cost = totalCost(g);
		// usually positive, but can be negative
		int64_t cost_reduction = total_cost - new_total_cost;
		double proportion_reduced = (double)cost_reduction / (double)total_cost;

		total_cost = new_total_cost;

		return proportion_reduced;
	}
};

bool CostScaling::runCostThreshold(double minimum_factor) {
	CostDelta *cd = new CostDelta();
	bool success = run([minimum_factor, cd, this]()-> bool {
		double proportion_reduced = cd->update(g);
		// FIXME: hack, sign of proportion_reduced is important.
		// Empirically, find cost bounces around first few rounds, then settles
		// down. If we used price refinement heuristic, I think this wouldn't happen.
		// Alternatively, could keep track of total cost of several previous rounds
		// and only terminate if not reduced in last k rounds (for some constant k.)
		return fabs(proportion_reduced) > minimum_factor;
	});
	delete cd;
	return success;
}

class TaskAssignmentDelta {
	TaskAssignment ta;
	std::unordered_map<uint32_t, uint32_t> *task_map;
	bool updated;

	uint32_t computeDelta(std::unordered_map<uint32_t, uint32_t> *new_map) {
		uint32_t differences = 0;

		CHECK_EQ(new_map->size(), task_map->size())
				<< "number of tasks should not change";

		for (std::pair<uint32_t, uint32_t> p : *task_map) {
			uint32_t task_id = p.first;
			CHECK_GT(new_map->count(task_id), 0)
				<< "new task assignment missing tasks";

			if ((*new_map)[task_id] != p.second) {
				differences++;
			}
		}

		return differences;
	}
public:
	TaskAssignmentDelta(FlowNetwork &g) : ta(g), updated(false) {
		task_map = new std::unordered_map<uint32_t, uint32_t>();
	}

	uint32_t update(FlowNetwork &g) {
		std::unordered_map<uint32_t, uint32_t> *new_task_map;
		new_task_map = ta.getAssignments(g);
		uint32_t delta;
		if (!updated) {
			updated = true;
			delta = new_task_map->size();
		} else {
			delta = computeDelta(new_task_map);
		}

		delete task_map;
		task_map = new_task_map;

		VLOG(0) << "# task assignments changed: " << delta;
		return delta;
	}
};

bool CostScaling::runTaskAssignmentThreshold(uint64_t min_assignments) {
	TaskAssignmentDelta *tad = new TaskAssignmentDelta(g);
	bool success = run([min_assignments, tad, this]() -> bool {
		uint32_t num_assignments = tad->update(g);
		return num_assignments >= min_assignments;
	});
	delete tad;
	return success;
}

template <class Rep,class Period>
uint64_t DurationToMicro(std::chrono::duration<Rep,Period> &d) {
	return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

bool CostScaling::runStatistics(std::string csv_path, bool task_assignments) {
	TaskAssignmentDelta *tad = nullptr;
	if (task_assignments) {
		tad = new TaskAssignmentDelta(g);
	}

	std::fstream *csv_file;
	csv_file = new std::fstream(csv_path.c_str(),
						        std::fstream::out | std::fstream::trunc);
	if (csv_file->fail()) {
		LOG(FATAL) << "Cannot open " << csv_path;
	}
	*csv_file << "refine_iteration,refine_time,overhead_time,epsilon,cost,"
			         "task_assignments_changed" << std::endl;;

	// note the reported time for the first iteration will include some setup,
	// such as finding the maximum cost and checking for feasibility
	std::chrono::time_point<std::chrono::high_resolution_clock> *last;
	last = new std::chrono::time_point<std::chrono::high_resolution_clock>();
	*last = std::chrono::high_resolution_clock::now();
	bool success = run([csv_file, tad, last, task_assignments, this]() -> bool {
		auto after_refine = std::chrono::high_resolution_clock::now();
		auto refine_time = after_refine - *last;

		uint64_t total_cost = totalCost(g);

		int64_t num_assignments = -1;
		if (task_assignments) {
			num_assignments = tad->update(g);
		}

		auto after_update = std::chrono::high_resolution_clock::now();
		auto overhead_time = after_update - after_refine;
		*last = after_update;

		*csv_file << num_iterations << "," << DurationToMicro(refine_time) << ","
				  << DurationToMicro(overhead_time) << "," << epsilon << ","
				  << total_cost << "," << num_assignments << std::endl;;

		return true;
	});

	csv_file->flush();
	csv_file->close();

	delete tad;
	delete csv_file;

	return success;
}

} /* namespace flowsolver */
