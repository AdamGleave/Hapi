/*
 * ResidualNetwork.cpp
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#include <cassert>

#include "ResidualNetwork.h"

namespace flowsolver {

ResidualNetwork::ResidualNetwork(uint32_t num_nodes) {
	this->num_nodes = num_nodes;
	// initialize all supply values to zero
	this->supply = new int64_t[num_nodes]();
	arcs.resize(num_nodes);
}

uint32_t ResidualNetwork::getNumNodes() const {
	return num_nodes;
}

uint32_t ResidualNetwork::getNumArcs() const {
	uint32_t num_arcs = 0;

	std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator vec_it;
	for (vec_it = this->arcs.begin(); vec_it != this->arcs.end(); vec_it++) {
		num_arcs += vec_it->size();
	}
	// TODO: if we turn this into a residual network with reverse arcs,
	// this will become more complicated. We would want to disappear any
	// reverse arcs with no flow. (Or we could just be naive and output all.)

	return num_arcs;
}

int64_t ResidualNetwork::getSupply(uint32_t id) const {
	id--; // id's are 1-indexed
	assert(id < this->num_nodes);
	return this->supply[id];
}

void ResidualNetwork::setSupply(uint32_t id, int64_t supply) {
	id--; // id's are 1-indexed
	assert(id < this->num_nodes);
	this->supply[id] = supply;
}

void ResidualNetwork::addEdge(uint32_t src, uint32_t dst,
							  uint64_t capacity, int64_t cost) {
	arcs[src][dst] = new Arc(src, dst, capacity, cost);
}

void ResidualNetwork::pushFlow(uint32_t src, uint32_t dst, int64_t amount) {
	arcs[src][dst]->pushFlow(amount);
}

ResidualNetwork::~ResidualNetwork() {
}

} /* namespace flowsolver */
