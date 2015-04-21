/*
 * CycleCancelling.h
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#ifndef LIB_CYCLE_CANCELLING_H_
#define LIB_CYCLE_CANCELLING_H_

#include <queue>

#include "bellman_ford.h"
#include "edmonds_karp.h"
#include "residual_network.h"

namespace flowsolver {

class CycleCancelling {
public:
	explicit CycleCancelling(ResidualNetwork &g) : g(g) { };

	/**
	 * Side-effect: changes graph capacities to encode flow.
	 */
	void run();
private:
	ResidualNetwork &g;
};

} /* namespace flowsolver */

#endif /* LIB_CYCLE_CANCELLING_H_ */
