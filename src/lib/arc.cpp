/*
 * Arc.cpp
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#include <cassert>

#include "arc.h"

namespace flowsolver {

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

bool Arc::setCapacity(uint64_t new_capacity) {
	// note we are setting the capacity of the arc
	// but capacity is the *residual* capacity of the arc
	int64_t delta = new_capacity - initial_capacity;
	capacity += delta;
	initial_capacity = new_capacity;

	// is capacity constraint satisfied?
	// (i.e. is flow < capacity of arc)?
	return capacity >= 0;
}

bool Arc::operator==(const Arc &arc) const {
	return src_id == arc.src_id && dst_id == arc.dst_id
			   && capacity == arc.capacity && initial_capacity == arc.initial_capacity
				 && cost == arc.cost;
}

bool Arc::operator!=(const Arc &arc) const {
	return !(*this == arc);
}

} /* namespace flowsolver */
