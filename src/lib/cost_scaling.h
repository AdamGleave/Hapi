#ifndef LIB_COST_SCALING_H_
#define LIB_COST_SCALING_H_

#include <vector>
#include <queue>
#include <forward_list>
#include <functional>

#include "flow_network.h"
#include "task_assignment.h"

namespace flowsolver {

class CostScaling {
public:
	explicit CostScaling(FlowNetwork &g);
	CostScaling(FlowNetwork &g, uint32_t scaling_factor);
	virtual ~CostScaling();

	/**
	 * All the run algorithms return true if a min-cost flow is found;
	 * false if no feasible solution exists. Side-effect that graph is updated
	 * to contain the solution found.
	 */
	/**
	 * Exact (optimal algorithm).
	 *
	 */
	bool runOptimal();
	/*
	 * Exact (optimal algorithm). Computes all factors used by approximate
	 * algorithm and reports them, along with other statistics, for each
	 * iteration in CSV format into file csv_path.
	 */
	bool runStatistics(std::string csv_path);
	/**
	 * Approximate algorithm. Terminates when epsilon is < threshold.
	 */
	bool runEpsilonOptimal(double threshold);
	/**
	 * Approximate algorithm. Terminates after at most max_iterations.
	 */
	bool runFixedIterations(uint64_t max_iterations);
	/**
	 * Approximate algorithm. Terminates when cost is reduced by less than
	 * minimum_factor between two successive iterations.
	 */
	bool runCostThreshold(double minimum_factor);
	/**
	 * Approximate algorithm. Terminates when number of task assignments changed
	 * is less than min_assignments between two successive iterations.
	 */
	bool runTaskAssignmentThreshold(uint64_t min_assignments);
private:
	void relabel(uint32_t id);
	// returns true if potential of id increases
	bool pushOrUpdate(uint32_t id);
	void discharge(uint32_t id);
	bool costThreshold(double minimum_factor);
	void refine();
	bool run(std::function<bool()> continue_running);
	int64_t reducedCost(Arc &arc, uint32_t src_id);

	FlowNetwork &g;
	uint64_t epsilon, num_iterations;
	const uint64_t SCALING_FACTOR, COST_SCALING_FACTOR;
	std::vector<int64_t> potentials;
	// TODO: implementation of the queue? dequeue and list both possible
	std::queue<uint32_t> active_vertices;
	std::vector<std::forward_list<Arc *>::iterator> current_edges;
};

} /* namespace flowsolver */

#endif /* LIB_COST_SCALING_H_ */
