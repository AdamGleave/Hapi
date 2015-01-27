/*
 * Arc.h
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#ifndef LIB_ARC_H_
#define LIB_ARC_H_

#include <cinttypes>

namespace flowsolver {

class Arc {
public:
	Arc(uint32_t src_id, uint32_t dst_id, uint64_t capacity, int64_t cost) :
		src_id(src_id), dst_id(dst_id), capacity(capacity),
		initial_capacity(capacity), cost(cost) {};
	virtual ~Arc() { }

	uint32_t getDstId() const {
		return dst_id;
	}

	uint32_t getSrcId() const {
		return src_id;
	}

	uint32_t getOppositeId(uint32_t id) const;

	uint64_t getCapacity() const {
		return capacity;
	}

	int64_t getCost() const {
		return cost;
	}

	uint64_t getInitialCapacity() const {
		return initial_capacity;
	}

	int64_t getFlow() const {
		return getInitialCapacity() - getCapacity();
	}
private:
	void pushFlow(int64_t amount);

	uint32_t src_id, dst_id;
	uint64_t capacity;
	uint64_t initial_capacity;
	int64_t cost;

	friend class ResidualNetwork;
	friend class FlowNetwork;
};

} /* namespace flowsolver */

#endif /* LIB_ARC_H_ */
