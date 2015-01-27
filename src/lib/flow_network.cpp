#include "flow_network.h"

#include <cassert>


namespace flowsolver {

FlowNetwork::FlowNetwork(uint32_t num_nodes) : num_nodes(num_nodes) {
	// we index from 1, leaving index 0 as a sentinel node
	// for simplicity, size all vectors as num_nodes + 1
	balances.resize(num_nodes + 1);
	arcs.resize(num_nodes + 1);
}

FlowNetwork::~FlowNetwork() {
	for (iterator it = begin(); it != end(); ++it) {
		Arc *arc = &(*it);
		delete arc;
	}
}


uint32_t FlowNetwork::getNumNodes() const {
	return num_nodes;
}

uint32_t FlowNetwork::getNumArcs() const {
	return num_arcs;
}

void FlowNetwork::addArc(uint32_t src, uint32_t dst,
						 uint64_t capacity, int64_t cost) {
	assert(src != 0 && dst != 0);
	Arc *arc = new Arc(src, dst, capacity, cost);
	arcs[src].push_front(arc);
	arcs[dst].push_front(arc);
	num_arcs++;
}

Arc* FlowNetwork::getArc(uint32_t src, uint32_t dst) {
	assert(src != 0 && dst != 0);
	std::forward_list<Arc *>::iterator arc_it;
	for (arc_it = arcs[src].begin(); arc_it != arcs[src].end(); ++arc_it) {
		Arc *arc = *arc_it;
		if ((arc->getSrcId() == src && arc->getDstId() == dst)
		 || (arc->getSrcId() == dst && arc->getDstId() == src)) {
			return *arc_it;
		}
	}
	return 0;
}

std::forward_list<Arc *> &FlowNetwork::getAdjacencies(uint32_t src) {
	return arcs[src];
}

const std::forward_list<Arc *> &FlowNetwork::getAdjacencies(uint32_t src) const {
	return arcs[src];
}

void FlowNetwork::setSupply(uint32_t id, int64_t supply) {
	// node must be uninitialized so far
	// (this won't catch all cases, node could have zero balance despite
	// non-zero initial supply if flow has been pushed)
	assert(balances[id] == 0);

	balances[id] = supply;
}

int64_t FlowNetwork::getResidualCapacity(Arc& arc, uint32_t src_id) {
	if (arc.getSrcId() == src_id) {
		// arc is forwards
		return arc.getCapacity();
	} else if (arc.getDstId() == src_id) {
		// arc is reverse
		return arc.getInitialCapacity() - arc.getCapacity();
	} else {
		assert(false);
		// NOREACH
		return 0;
	}
}

int64_t FlowNetwork::pushFlow(Arc& arc, uint32_t src_id, uint64_t flow) {
	uint32_t dst_id = -1;
	if (arc.getSrcId() == src_id) {
		// arc is forwards
		arc.pushFlow(flow);
		dst_id = arc.getDstId();
	} else if (arc.getDstId() == src_id) {
		// arc is reverse
		arc.pushFlow(-flow);
		dst_id = arc.getSrcId();
	} else {
		assert(false);
	}
	balances[src_id] -= flow;
	balances[dst_id] += flow;

	return balances[dst_id];
}


FlowNetwork::iterator FlowNetwork::begin() {
	return iterator(this);
}

FlowNetwork::const_iterator FlowNetwork::begin() const {
	return const_iterator(this);
}

FlowNetwork::iterator FlowNetwork::end() {
	return iterator(this, true);
}

FlowNetwork::const_iterator FlowNetwork::end() const {
	return const_iterator(this, true);
}

} /* namespace flowsolver */
