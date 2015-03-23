#ifndef LIB_INCREMENTAL_SOLVER_H_
#define LIB_INCREMENTAL_SOLVER_H_

#include <cstdint>
#include <vector>

namespace flowsolver {

// interface
class IncrementalSolver {
public:
	// Runs algorithm from 'cold', i.e. on a network that has just been loaded.
	void run();
	// Reoptimizes. Network must have a pseudoflow satisfying the reduced-cost
	// optimality conditions.
	void reoptimize();

protected:
	virtual std::vector<uint64_t> &getPotentials() = 0;
	IncrementalSolver() {}
	virtual ~IncrementalSolver() {}

	friend class DynamicMaintainOptimality;
};

} /* namespace flowsolver */

#endif /* LIB_INCREMENTAL_SOLVER_H_ */
