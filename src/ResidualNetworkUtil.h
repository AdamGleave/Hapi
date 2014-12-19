/*
 * ResidualNetworkUtil.h
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#ifndef RESIDUALNETWORKUTIL_H_
#define RESIDUALNETWORKUTIL_H_

#include <queue>

#include "ResidualNetwork.h"

namespace flowsolver {

class ResidualNetworkUtil {
	static uint64_t augmentingFlow(std::queue<Arc *>);
	static void pushFlow(ResidualNetwork &, std::queue<Arc *>, uint64_t);
public:
	static void augmentPath(ResidualNetwork &, std::queue<Arc *>);
	static void cancelCycle(ResidualNetwork &, std::queue<Arc *>);
};

} /* namespace flowsolver */

#endif /* RESIDUALNETWORKUTIL_H_ */
