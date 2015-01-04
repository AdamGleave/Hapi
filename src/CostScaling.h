#ifndef COSTSCALING_H_
#define COSTSCALING_H_

#include <vector>
#include <forward_list>
#include <functional>

#include "FlowNetwork.h"

namespace flowsolver {

class CostScaling {
	FlowNetwork &g;
	uint64_t epsilon, num_iterations;
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
	void refine();
	bool run(std::function<bool()> continue_running);
public:
	CostScaling(FlowNetwork &g);
	/**
	 * Side-effect: changes graph capacities to encode flow.
	 *
	 * Returns true if a min-cost flow is found; false if no feasible
	 * solution exists.
	 */
	bool runOptimal();
	bool runEpsilonOptimal(double threshold);
	bool runFixedIterations(uint64_t max_iterations);
	virtual ~CostScaling();
};

} /* namespace flowsolver */

#endif /* COSTSCALING_H_ */
