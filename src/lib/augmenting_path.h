#ifndef LIB_AUGMENTING_PATH_H_
#define LIB_AUGMENTING_PATH_H_

#include <climits>
#include <vector>
#include <queue>

#include "residual_network.h"

namespace flowsolver {

class AugmentingPath {
public:
	explicit AugmentingPath(ResidualNetwork &g);
	virtual ~AugmentingPath();

	// Performs AugmentingPath algorithm from 'cold', i.e. on a network that has
	// just been loaded
	void run();
	// Performs AugmentingPath algorithm, on a network that already has a
	// pseudoflow satisfying the reduced-cost optimality conditions.
	void reoptimize();

	// TODO(adam): better way of exposing this?
	// note this allows potentials to be mutated
	// needed by DynamicMaintainOptimality
	std::vector<uint64_t> &getPotentials() {
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
