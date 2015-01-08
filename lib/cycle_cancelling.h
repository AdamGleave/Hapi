/*
 * CycleCancelling.h
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#ifndef CYCLE_CANCELLING_H_
#define CYCLE_CANCELLING_H_

#include <queue>

#include "bellman_ford.h"
#include "edmonds_karp.h"
#include "residual_network.h"

namespace flowsolver {

class CycleCancelling {
	ResidualNetwork &g;
public:
	CycleCancelling(ResidualNetwork &g) : g(g) { };

	/**
	 * Side-effect: changes graph capacities to encode flow.
	 */
	void run();
};

} /* namespace flowsolver */

#endif /* CYCLE_CANCELLING_H_ */
