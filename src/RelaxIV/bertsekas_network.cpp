#include "bertsekas_network.h"

flowsolver::BertsekasNetwork::BertsekasNetwork(uint32_t num_nodes,
		uint32_t num_arcs) {
}

flowsolver::BertsekasNetwork::~BertsekasNetwork() {
}

uint32_t flowsolver::BertsekasNetwork::getNumNodes() const {
}

uint32_t flowsolver::BertsekasNetwork::getNumNodesAllocated() const {
}

uint32_t flowsolver::BertsekasNetwork::getNumArcs() const {
}

uint32_t flowsolver::BertsekasNetwork::getNumArcsAllocated() const {
}

int64_t flowsolver::BertsekasNetwork::getBalance(uint32_t id) const {
}

int64_t flowsolver::BertsekasNetwork::getSupply(uint32_t id) const {
}

Arc* flowsolver::BertsekasNetwork::getArc(uint32_t src, uint32_t dst) const {
}

void flowsolver::BertsekasNetwork::addNode(uint32_t id) {
}

void flowsolver::BertsekasNetwork::removeNode(uint32_t id) {
}

void flowsolver::BertsekasNetwork::addArc(uint32_t src, uint32_t dst,
		uint64_t capacity, int64_t cost) {
}

void flowsolver::BertsekasNetwork::changeArcCost(uint32_t src, uint32_t dst,
		int64_t cost) {
}

bool flowsolver::BertsekasNetwork::changeArcCapacity(uint32_t src, uint32_t dst,
		uint64_t capacity) {
}

void flowsolver::BertsekasNetwork::removeArc(uint32_t src, uint32_t dst) {
}

void flowsolver::BertsekasNetwork::setSupply(uint32_t id, int64_t supply) {
}
