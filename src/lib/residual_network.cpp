#include "residual_network.h"

#include <cassert>

#include <glog/logging.h>

namespace flowsolver {

ResidualNetwork::ResidualNetwork(uint32_t num_nodes) : num_nodes(num_nodes) {
	// initialize all supply values to zero
	balance.resize(num_nodes + 1);
	arcs.resize(num_nodes + 1);
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

uint32_t ResidualNetwork::getNumNodes() const {
	return num_nodes;
}

uint32_t ResidualNetwork::getNumArcs() const {
	uint32_t num_arcs = 0;

	std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator vec_it;
	for (vec_it = arcs.begin(); vec_it != arcs.end(); ++vec_it) {
		num_arcs += vec_it->size();
	}

	CHECK(num_arcs % 2 == 0);
	// we double count arcs, as we consider both forward and reverse arc
	return num_arcs / 2;
}

int64_t ResidualNetwork::getBalance(uint32_t id) const {
	assert(validID(id));
	return balance[id];
}

const std::set<uint32_t>& ResidualNetwork::getSinks() const {
	return sinks;
}

const std::set<uint32_t>& ResidualNetwork::getSources() const {
	return sources;
}

Arc *ResidualNetwork::getArc(uint32_t src, uint32_t dst) const {
	assert(validID(src) && validID(dst));
	std::unordered_map<uint32_t, Arc*>::const_iterator arcIt = arcs[src].find(dst);
	if (arcIt == arcs[src].end()) {
		return nullptr;
	} else {
		return arcIt->second;
	}
}

const std::unordered_map<uint32_t, Arc*>& ResidualNetwork::getAdjacencies
																													(uint32_t src) const {
	assert(validID(src));
	return arcs[src];
}

uint32_t ResidualNetwork::addNode() {
	uint32_t id;

	num_nodes++;

	if (free_nodes.empty()) {
		balance.push_back(0);
		arcs.push_back(std::unordered_map<uint32_t, Arc*>());
		id = num_nodes;
		// check for 'node leaks': free nodes not in the free list
		assert(id == balance.size() - 1 && id == arcs.size() - 1);
	} else {
		std::set<uint32_t>::iterator it = free_nodes.begin();
		id = *it;
		free_nodes.erase(it);
	}

	// should not be any stale state from previous instances of this node ID
	assert(balance[id] == 0);
	assert(arcs[id].empty());
	assert(sources.count(id) == 0 && sinks.count(id) == 0);

	return id;
}

// SOMEDAY(adam): size of arcs & balance vector will grow without bound:
// we never compact the graph after having removed nodes.
void ResidualNetwork::removeNode(uint32_t id) {
	// check node currently exists
	assert(validID(id));

	// remove edges
	std::unordered_map<uint32_t, Arc*> &adjacencies = arcs[id];

	// remove the incoming edges
	for (auto &adjacency : adjacencies) {
		uint32_t dst_id = adjacency.first;
		// erase reverse pointer
		uint32_t num_erased = arcs[dst_id].erase(id);
		assert(num_erased = 1);
	 }

	// remove the outgoing edges
	adjacencies.clear();

	// remove any other references to it
	sources.erase(id);
	sinks.erase(id);

	// clear up state
	balance[id] = 0;

	// node now free
	num_nodes--;
	free_nodes.insert(id);
}

void ResidualNetwork::addArc(uint32_t src, uint32_t dst,
							  uint64_t capacity, int64_t cost) {
	// indices in range
	assert(validID(src) && validID(dst));
	// arc does not already exist in graph
	assert(arcs[src].count(dst) == 0);
	// forward arc
	arcs[src][dst] = new Arc(src, dst, capacity, cost);
	// reverse arc
	arcs[dst][src] = new Arc(dst, src, 0, -cost);
}

bool ResidualNetwork::changeArc(uint32_t src, uint32_t dst,
							  uint64_t capacity, int64_t cost) {
	// indices in range
	assert(validID(src) && validID(dst));

	// reverse arcs capacity remains 0 (the residual capacity on reverse arc
	// is flow on forward arc, which is unchanged)
	arcs[dst][src]->setCost(-cost);

	Arc *forward = arcs[src][dst];
	forward->setCost(cost);
	return forward->setCapacity(capacity);
}

void ResidualNetwork::removeArc(uint32_t src, uint32_t dst) {
	// indices in range
	assert(validID(src) && validID(dst));
	uint32_t num_erased = arcs[src].erase(dst);
	assert(num_erased == 1); // check arc did exist
	num_erased = arcs[dst].erase(src);
	assert(num_erased == 1);
}

void ResidualNetwork::setSupply(uint32_t id, int64_t supply) {
	assert(validID(id));

	if (balance[id] > 0) {
		sources.erase(id);
	} else if (balance[id] < 0) {
		sinks.erase(id);
	}

	balance[id] = supply;
	if (balance[id] > 0) {
		sources.insert(id);
	} else if (balance[id] < 0) {
		sinks.insert(id);
	}
}

void ResidualNetwork::pushFlow(uint32_t src, uint32_t dst, int64_t amount) {
	assert(validID(src) && validID(dst));
	arcs[src][dst]->pushFlow(amount);
	arcs[dst][src]->pushFlow(-amount);

	setSupply(src, balance[src] - amount);
	setSupply(dst, balance[dst] + amount);
}

bool ResidualNetwork::validID(uint32_t id) const {
	return id < arcs.size() && free_nodes.count(id) == 0;
}

ResidualNetwork::iterator ResidualNetwork::begin() {
	return iterator(this);
}

ResidualNetwork::const_iterator ResidualNetwork::begin() const {
	return const_iterator(this);
}

ResidualNetwork::iterator ResidualNetwork::end() {
	return iterator(this, true);
}

ResidualNetwork::const_iterator ResidualNetwork::end() const {
	return const_iterator(this, true);
}

} /* namespace flowsolver */
