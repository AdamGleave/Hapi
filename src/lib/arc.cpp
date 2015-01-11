/*
 * Arc.cpp
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#include <cassert>

#include "arc.h"

namespace flowsolver {

void Arc::pushFlow(int64_t amount) {
	capacity -= amount;
}

uint32_t Arc::getOppositeId(uint32_t id) const {
	if (id == src_id) {
		return dst_id;
	} else if (id == dst_id) {
		return src_id;
	} else {
		assert(false);
		return 0;
	}
}

} /* namespace flowsolver */
