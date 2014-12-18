/*
 * CycleCancelling.h
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#ifndef CYCLECANCELLING_H_
#define CYCLECANCELLING_H_

#include <queue>

#include "ResidualNetwork.h"
#include "BellmanFord.h"
#include "EdmondsKarp.h"

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

#endif /* CYCLECANCELLING_H_ */
