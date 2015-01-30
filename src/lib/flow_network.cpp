#include "flow_network.h"

#include <cassert>

namespace flowsolver {

FlowNetwork::FlowNetwork(uint32_t num_nodes) : num_nodes(num_nodes) {
	// we index from 1, leaving index 0 as a sentinel node
	// for simplicity, size all vectors as num_nodes + 1
	balances.resize(num_nodes + 1);
	arcs.resize(num_nodes + 1);
}

FlowNetwork::FlowNetwork(const ResidualNetwork &g)
																							  : FlowNetwork(g.getNumNodes()) {
	// ResidualNetwork stores each arc in the flow network as two arcs: the
	// forward and reverse.
	for (const Arc &arc_ref : g) {
		const Arc *arc = &arc_ref;
		// De-duplicate: skipping these arcs guarantee we only see each one once
		uint32_t src_id = arc->getSrcId(), dst_id = arc->getDstId();
		if (src_id > dst_id) {
			continue;
		}

		// find the paired arc
		const Arc *opposite_arc = g.getArc(dst_id, src_id);
		if (arc->getInitialCapacity() == 0) {
			// arc is the reverse arc
			std::swap(arc, opposite_arc);
			std::swap(src_id, dst_id);
		} else {
			// arc is the forward arc
		}

		// arc is now forward arc, opposite_arc the reverse arc
		// src_id, dst_id the src and dst id of the forward arc
		uint64_t capacity = arc->getCapacity() + opposite_arc->getCapacity();

		Arc &new_arc = addArcInternal(src_id, dst_id, capacity, arc->getCost());
		new_arc.pushFlow(arc->getFlow());
	}

	// clone balances
	for (uint32_t id = 1; id <= num_nodes; id++) {
		balances[id] = g.getBalance(id);
	}
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

Arc &FlowNetwork::addArcInternal(uint32_t src, uint32_t dst,
						 uint64_t capacity, int64_t cost) {
	assert(src != 0 && dst != 0);
	Arc *arc = new Arc(src, dst, capacity, cost);
	arcs[src].push_front(arc);
	arcs[dst].push_front(arc);
	num_arcs++;

	return *arc;
}

void FlowNetwork::addArc(uint32_t src, uint32_t dst,
		 	 	 	 	 	 	 	 	     uint64_t capacity, int64_t cost) {
	(void)addArcInternal(src, dst, capacity, cost);
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
	return nullptr;
}

std::forward_list<Arc *> &FlowNetwork::getAdjacencies(uint32_t src) {
	return arcs[src];
}

const std::forward_list<Arc *> &FlowNetwork::getAdjacencies(uint32_t src) const {
	return arcs[src];
}

void FlowNetwork::setSupply(uint32_t id, int64_t supply) {
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

// this, the equality and inequality operator are only used by unit tests.
// There is no reason it should ever be used by an actual algorithm.
// Consequently, efficiency is not a priority.
namespace {
void buildArcMap(uint32_t id, const std::forward_list<Arc *> arcs,
		             std::unordered_map<uint32_t, Arc*> &map) {
	for (Arc *arc : arcs) {
		map[arc->getOppositeId(id)] = arc;
	}
}
}


bool FlowNetwork::operator==(const FlowNetwork &g) const {
	if (balances == g.balances) {
		// note this implies both networks have the same number of nodes
		for (uint32_t id = 1; id <= num_nodes; id++) {
			const std::forward_list<Arc*> &our_adjacencies = arcs[id];
			const std::forward_list<Arc*> &other_adjacencies = g.arcs[id];

			std::unordered_map<uint32_t, Arc*> our_map, other_map;
			buildArcMap(id, our_adjacencies, our_map);
			buildArcMap(id, other_adjacencies, other_map);

			if (!ResidualNetwork::adjacencyEquals(our_map, other_map)) {
				return false;
			}
		}

		return true;
	} else {
		VLOG(3) << "disagree in balances";
		return false;
	}
}

bool FlowNetwork::operator!=(const FlowNetwork &g) const {
	return !(*this == g);
}

} /* namespace flowsolver */
