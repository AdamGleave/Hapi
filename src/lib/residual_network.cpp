#include "residual_network.h"
#include <cassert>

namespace flowsolver {

ResidualNetwork::ResidualNetwork(uint32_t num_nodes) : num_nodes(num_nodes) {
	// initialize all supply values to zero
	balance.resize(num_nodes + 1);
	arcs.resize(num_nodes + 1);
}

uint32_t ResidualNetwork::getNumNodes() const {
	return num_nodes;
}

uint32_t ResidualNetwork::getNumArcs() const {
	uint32_t num_arcs = 0;

	std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator vec_it;
	for (vec_it = arcs.begin(); vec_it != arcs.end(); ++vec_it) {
		num_arcs += vec_it->size();
	}

	return num_arcs;
}

int64_t ResidualNetwork::getBalance(uint32_t id) const {
	assert(id <= num_nodes);
	return balance[id];
}

void ResidualNetwork::setSupply(uint32_t id, int64_t supply) {
	assert(id <= num_nodes);

	// Can only set supply once. Check it is zero, as it must be when just
	// initialized. (Note won't catch all mistakes.)
	assert(balance[id] == 0);
	balance[id] = supply;
	if (supply < 0) {
		sinks.insert(id);
	} else if (supply > 0) {
		sources.insert(id);
	}
}

const std::set<uint32_t>& ResidualNetwork::getSinks() const {
	return sinks;
}

const std::set<uint32_t>& ResidualNetwork::getSources() const {
	return sources;
}

void ResidualNetwork::addArc(uint32_t src, uint32_t dst,
							  uint64_t capacity, int64_t cost) {
	// indices in range
	assert(src <= num_nodes && dst <= num_nodes);
	// arc does not already exist in graph
	assert(arcs[src].count(dst) == 0);
	// forward arc
	arcs[src][dst] = new Arc(src, dst, capacity, cost);
	// reverse arc
	arcs[dst][src] = new Arc(dst, src, 0, -cost);
}

void ResidualNetwork::updateSupply(uint32_t index, int64_t delta) {
	assert(index <= num_nodes);
	if (balance[index] > 0) {
		sources.erase(index);
	} else if (balance[index] < 0) {
		sinks.erase(index);
	}

	balance[index] += delta;
	if (balance[index] > 0) {
		sources.insert(index);
	} else if (balance[index] < 0) {
		sinks.insert(index);
	}
}

void ResidualNetwork::pushFlow(uint32_t src, uint32_t dst, int64_t amount) {
	assert(src <= num_nodes && dst <= num_nodes);
	arcs[src][dst]->pushFlow(amount);
	arcs[dst][src]->pushFlow(-amount);

	updateSupply(src, -amount);
	updateSupply(dst, amount);
}

std::unordered_map<uint32_t, Arc*> &ResidualNetwork::getAdjacencies(uint32_t src) {
	assert(src <= this->num_nodes);
	return arcs[src];
}

Arc *ResidualNetwork::getArc(uint32_t src, uint32_t dst) {
	assert(src <= num_nodes && dst <= num_nodes);
	std::unordered_map<uint32_t, Arc*>::iterator arcIt = arcs[src].find(dst);
	if (arcIt == arcs[src].end()) {
		return 0;
	} else {
		return arcIt->second;
	}
}

ResidualNetwork::~ResidualNetwork() {
	std::vector<std::unordered_map<uint32_t, Arc*>>::iterator vec_it;
	for (vec_it = arcs.begin(); vec_it != arcs.end(); ++vec_it) {
		std::unordered_map<uint32_t, Arc*>::iterator map_it;
		for (map_it = vec_it->begin(); map_it != vec_it->end(); ++map_it) {
			Arc *arc = map_it->second;
			delete arc;
		}
	}
}

} /* namespace flowsolver */
