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
	capacity -= amount;
}

} /* namespace flowsolver */
