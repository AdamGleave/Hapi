/*
 * ResidualNetworkUtil.h
 *
 *  Created on: 18 Dec 2014
 *      Author: adam
 */

#ifndef LIB_RESIDUAL_NETWORK_UTIL_H_
#define LIB_RESIDUAL_NETWORK_UTIL_H_

#include <queue>

#include "residual_network.h"

namespace flowsolver {

class ResidualNetworkUtil {
	static uint64_t augmentingFlow(std::queue<Arc *>);
	static void pushFlow(ResidualNetwork &, std::queue<Arc *>, uint64_t);
public:
	ResidualNetworkUtil() = delete;
	static void augmentPath(ResidualNetwork &, std::queue<Arc *>);
	static void cancelCycle(ResidualNetwork &, std::queue<Arc *>);
};

} /* namespace flowsolver */

#endif /* LIB_RESIDUAL_NETWORK_UTIL_H_ */
