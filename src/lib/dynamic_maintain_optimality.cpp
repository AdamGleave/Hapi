#include <cassert>
#include <unordered_map>

#include <glog/logging.h>

#include "dynamic_maintain_optimality.h"

namespace flowsolver {

Arc *DynamicMaintainOptimality::getArc(uint32_t src, uint32_t dst) {
	return g.getArc(src, dst);
}
void DynamicMaintainOptimality::addNode(uint32_t id) {
	// adding a node does not change the min-cost solution, since initially the
	// arcs has no edges. pass through
	g.addNode(id);
	if (id >= potentials.size()) {
		potentials.resize(id + 1);
		// note the new potential cells will be initialized to zero
	}
}

void DynamicMaintainOptimality::setSupply(uint32_t id, int64_t supply) {
  // Changing supply cannot violate optimality.
	// It may, however, break feasibility.
	return g.setSupply(id, supply);
}

void DynamicMaintainOptimality::removeNode(uint32_t id) {
	// Removing a node itself has no effect on the min-cost solution:
	// it is removing the associated arcs that changes the solution

	// Note we must copy the adjacencies since we want to iterate over them,
	// and will be deleting arcs as we go.
	const std::unordered_map<uint32_t, Arc*> adjacencies(g.getAdjacencies(id));
	for (auto elt : adjacencies) {
		uint32_t opposite_id = elt.first;
		Arc *arc = elt.second;

		if (arc->getInitialCapacity() > 0) {
			// id->opposite_id is the forward arc
			removeArc(id, opposite_id);
		} else {
			// id->opposite_id is the reverse arc
			removeArc(opposite_id, id);
		}
	}

	// Now the node is disconnected from the network, remove it directly.
	g.removeNode(id);

	potentials[id] = 0;
}

void DynamicMaintainOptimality::addArc(uint32_t src, uint32_t dst,
		uint64_t capacity, int64_t cost) {
	// Adding an arc of 0 capacity has no impact on the min-cost solution
	// Therefore, adding an arc of capacity x is equivalent to first adding an arc
	// with the same cost but capacity 0, and then increasing its capacity to x

	g.addArc(src, dst, 0, cost);
	changeArcCapacity(src, dst, capacity);
}

void DynamicMaintainOptimality::changeArcCost(uint32_t src, uint32_t dst,
																						  int64_t cost) {
	Arc *arc = g.getArc(src, dst);
	CHECK(arc != nullptr) << "trying to change non-existent arc "
			                  << src << "->" << dst;

	int64_t old_cost = arc->getCost();
	if (cost != old_cost) {
		// For optimality, we care only about the sign of the reduced cost
		// If the sign remains the same, we need not do anything.
		// Furthermore, if the sign is currently zero, we can do nothing:
		// any flow value is legal when the reduced cost is zero.

		int64_t old_reduced_cost = old_cost - potentials[src] + potentials[dst];
		int64_t new_reduced_cost = cost - potentials[src] + potentials[dst];

		if (new_reduced_cost > 0) {
			// flow needs to be zero
			if (old_reduced_cost < 0) {
				// arc currently saturated
				assert(arc->getFlow() == arc->getCapacity());
				g.pushFlow(src, dst, -arc->getFlow());
			} else if (old_reduced_cost == 0) {
				// flow could be anything, check
				int64_t flow = arc->getFlow();
				if (flow > 0) {
					g.pushFlow(src, dst, -flow);
				}
			}
		} else if (new_reduced_cost < 0 && old_reduced_cost >= 0) {
			// arc needs to be saturated
			// (may already be saturated if old_reduced_cost == 0,
			// but in that case arc capacity will be zero, so no-op)
			g.pushFlow(src, dst, arc->getCapacity());
		}
	}
	g.changeArcCost(src, dst, cost);
}

bool DynamicMaintainOptimality::changeArcCapacity(uint32_t src, uint32_t dst,
																								  uint64_t capacity) {
	Arc *arc = g.getArc(src, dst);
	CHECK(arc != nullptr) << "trying to change non-existent arc "
										    << src << "->" << dst;

	int64_t old_capacity = arc->getInitialCapacity();
	int64_t new_capacity = static_cast<int64_t>(capacity);
	VLOG(1) << "Changing capacity of " << src << "->" << dst
			    << " from " << old_capacity << " to " << new_capacity;
	if (new_capacity < old_capacity) {
		// Decrease in capacity.
		// This can never violate optimality, but may make the flow no longer feasible.
		bool capacity_constraint = g.changeArcCapacity(src, dst, new_capacity);
		if (!capacity_constraint) {
			VLOG(2) << "Pushing " << arc->getCapacity();
			// must reduce flow to satisfy capacity constraint, may break feasibility
			// note arc->getCapacity() here is NEGATIVE
			g.pushFlow(src, dst, arc->getCapacity());
		}
	} else if (new_capacity > old_capacity) {
		/*
		 * Increase in capacity. This may violate optimality. Three cases:
		 * - Positive reduced cost. Flow was previously zero, and should remain zero.
		 * - Zero reduced cost. Any flow is OK.
		 * - Negative reduced cost. Arc previously saturated, and so was not in the
		 *   residual network. Now it is no longer saturated. We must increase the
		 *   flow, which will break feasibility.
		 */
		//TODO(adam): alternative approach is to just resaturate the arc if was
		//previously saturated. This means breaking feasibility in the case of zero
		//reduced cost, but we only need to check the current capacity which is
		//cheaper than computing reduced cost

		int64_t reduced_cost = arc->getCost() - potentials[src] + potentials[dst];

		bool capacity_constraint = g.changeArcCapacity(src, dst, new_capacity);
		assert(capacity_constraint);
		if (reduced_cost < 0) {
			// saturate arc
			g.pushFlow(src, dst, new_capacity - old_capacity);
		}
	}

	// if the capacity constraint was violated, we've fixed it up
	return true;
}

void DynamicMaintainOptimality::removeArc(uint32_t src, uint32_t dst) {
	// Logically, removing an arc is the same as setting it to have 0 capacity
	Arc *arc = g.getArc(src, dst);
	CHECK(arc != nullptr) << "trying to remove non-existent arc "
			                  << src << "->" << dst;
	changeArcCapacity(src, dst, 0);

	// Now delete it to save memory
	g.removeArc(src, dst);
}

} /* namespace flowsolver */
