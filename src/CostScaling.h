#ifndef COSTSCALING_H_
#define COSTSCALING_H_

#include <vector>
#include <forward_list>
#include <functional>

#include "FlowNetwork.h"
#include "TaskAssignment.h"

namespace flowsolver {

class CostScaling {
	FlowNetwork &g;
	uint64_t epsilon, num_iterations, total_cost;
	const uint64_t SCALING_FACTOR;
	std::vector<int64_t> potentials;
	std::forward_list<uint32_t> vertices;
	std::vector<std::forward_list<Arc *>::iterator> current_edges;

	int64_t reducedCost(Arc &arc, uint32_t src_id);
	void relabel(uint32_t id);
	// returns true if potential of id increases
	bool pushOrUpdate(uint32_t id);
	// returns true if potential of id increases
	bool discharge(uint32_t id);
	bool costThreshold(double minimum_factor);
	void refine();
	bool run(std::function<bool()> continue_running);
public:
	CostScaling(FlowNetwork &g);
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
	virtual ~CostScaling();
};

} /* namespace flowsolver */

#endif /* COSTSCALING_H_ */
