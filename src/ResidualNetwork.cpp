#include "ResidualNetwork.h"

#include <cassert>

namespace flowsolver {

ResidualNetwork::ResidualNetwork(uint32_t num_nodes) : num_nodes(num_nodes) {
	// initialize all supply values to zero
	this->supply = new int64_t[num_nodes]();
	this->initial_supply = new int64_t[num_nodes]();
	arcs.resize(num_nodes);
}

uint32_t ResidualNetwork::getNumNodes() const {
	return num_nodes;
}

uint32_t ResidualNetwork::getNumArcs() const {
	uint32_t num_arcs = 0;

	std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator vec_it;
	for (vec_it = this->arcs.begin(); vec_it != this->arcs.end(); ++vec_it) {
		num_arcs += vec_it->size();
	}

	return num_arcs;
}

int64_t ResidualNetwork::getSupply(uint32_t id) const {
	id--; // id's are 1-indexed
	assert(id < this->num_nodes);
	return this->supply[id];
}

int64_t ResidualNetwork::getInitialSupply(uint32_t id) const {
	id--; // id's are 1-indexed
	assert(id < this->num_nodes);
	return this->initial_supply[id];
}

void ResidualNetwork::setSupply(uint32_t id, int64_t supply) {
	id--; // id's are 1-indexed
	assert(id < this->num_nodes);

	// can only set supply once
	// (although may be modified via pushFlow)
	assert(this->initial_supply[id] == 0);
	this->supply[id] = supply;
	this->initial_supply[id] = supply;
	if (supply < 0) {
		sinks.insert(id + 1);
	} else if (supply > 0) {
		sources.insert(id + 1);
	}
}

const std::set<uint32_t>& ResidualNetwork::getSinks() const {
	return sinks;
}

const std::set<uint32_t>& ResidualNetwork::getSources() const {
	return sources;
}

void ResidualNetwork::addEdge(uint32_t src, uint32_t dst,
							  uint64_t capacity, int64_t cost) {
	// id's are 1-indexed
	src--;
	dst--;
	// indices in range
	assert(src < this->num_nodes && dst < this->num_nodes);
	// arc does not already exist in graph
	assert(arcs[src].count(dst) == 0);
	// forward arc
	arcs[src][dst] = new Arc(src+1, dst+1, capacity, cost);
	// reverse arc
	arcs[dst][src] = new Arc(dst+1, src+1, 0, -cost);
}

void ResidualNetwork::updateSupply(uint32_t index, int64_t delta) {
	if (supply[index] > 0) {
		sources.erase(index + 1);
	} else if (supply[index] < 0) {
		sinks.erase(index + 1);
	}

	supply[index] += delta;
	if (supply[index] > 0) {
		sources.insert(index + 1);
	} else if (supply[index] < 0) {
		sinks.insert(index + 1);
	}
}

void ResidualNetwork::pushFlow(uint32_t src, uint32_t dst, int64_t amount) {
	src--;
	dst--;
	assert(src < this->num_nodes && dst < this->num_nodes);
	arcs[src][dst]->pushFlow(amount);
	arcs[dst][src]->pushFlow(-amount);

	updateSupply(src, -amount);
	updateSupply(dst, amount);
}

std::unordered_map<uint32_t, Arc*> ResidualNetwork::getAdjacencies(uint32_t src) {
	src--;
	assert(src < this->num_nodes);
	return arcs[src];
}

Arc *ResidualNetwork::getArc(uint32_t src, uint32_t dst) {
	src--;
	dst--;
	assert(src< this->num_nodes && dst < this->num_nodes);
	return arcs[src][dst];
}

ResidualNetwork::~ResidualNetwork() {
	delete this->supply;
	delete this->initial_supply;

	std::vector<std::unordered_map<uint32_t, Arc*>>::iterator vec_it;
	for (vec_it = this->arcs.begin(); vec_it != this->arcs.end(); ++vec_it) {
		std::unordered_map<uint32_t, Arc*>::iterator map_it;
		for (map_it = vec_it->begin(); map_it != vec_it->end(); ++map_it) {
			Arc *arc = map_it->second;
			delete arc;
		}
	}
}

} /* namespace flowsolver */
