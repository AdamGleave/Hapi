#ifndef LIB_INCREMENTAL_SOLVER_H_
#define LIB_INCREMENTAL_SOLVER_H_

#include <cstdint>
#include <vector>

namespace flowsolver {

template<class T> class DIMACSFlowPotentialImporter;
template<class T> class DIMACSPotentialExporter;
class ResidualNetwork;

// interface
class IncrementalSolver {
public:
	// Runs algorithm from 'cold', i.e. on a network that has just been loaded.
	virtual void run() = 0;
	// Reoptimizes. Network must have a pseudoflow satisfying the reduced-cost
	// optimality conditions.
	virtual void reoptimize() = 0;

protected:
	virtual std::vector<uint64_t> &getPotentials() = 0;
	IncrementalSolver() {}
	virtual ~IncrementalSolver() {}

	friend class DynamicMaintainOptimality;
	friend class DIMACSFlowPotentialImporter<ResidualNetwork>;
	friend class DIMACSPotentialExporter<ResidualNetwork>;
};

} /* namespace flowsolver */

#endif /* LIB_INCREMENTAL_SOLVER_H_ */
