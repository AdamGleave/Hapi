/*
 * Arc.cpp
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#include <cassert>

#include "Arc.h"

namespace flowsolver {

void Arc::pushFlow(int64_t amount) {
	assert (amount < 0 || capacity >= (uint64_t)amount);
	capacity -= amount;
	// TODO: I think this will no longer hold true with reverse edges
	assert(capacity < initial_capacity);
}

} /* namespace flowsolver */
