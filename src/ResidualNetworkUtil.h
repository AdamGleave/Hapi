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
public:
	static void augmentPath(ResidualNetwork &g, std::queue<Arc *>);
};

} /* namespace flowsolver */

#endif /* RESIDUALNETWORKUTIL_H_ */
