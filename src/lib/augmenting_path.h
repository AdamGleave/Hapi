#ifndef LIB_AUGMENTING_PATH_H_
#define LIB_AUGMENTING_PATH_H_

#include <climits>
#include <vector>
#include <queue>

#include "incremental_solver.h"
#include "residual_network.h"

namespace flowsolver {

class AugmentingPath : public IncrementalSolver {
public:
	explicit AugmentingPath(ResidualNetwork &g);
	virtual ~AugmentingPath();

	virtual void run() override;
	virtual void reoptimize() override;
protected:
	virtual std::vector<uint64_t> &getPotentials() override {
		return potentials;
	}
private:
	void init();

	// returns true if feasible solution exists, and updates g accordingly;
	// false if no feasible solution, g left in undefined state
	std::queue<Arc *> predecessorPath(uint32_t source, uint32_t sink,
																					const std::vector<uint32_t>& parents);

	ResidualNetwork &g;
	std::vector<uint64_t> potentials;
};

} /* namespace flowsolver */

#endif /* LIB_AUGMENTING_PATH_H_ */
