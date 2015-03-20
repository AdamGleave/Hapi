/*
 * Strategy for incremental algorithms.
 *
 * Updates the underlying graph in response to changes.
 * Provided the solution initially satisfied reduced cost optimality conditions,
 * guarantees solution will continue to be optimal.
 *
 * The strategy may, however, violate feasibility.
 */

#ifndef LIB_DYNAMIC_MAINTAIN_OPTIMALITY_H_
#define LIB_DYNAMIC_MAINTAIN_OPTIMALITY_H_

#include "residual_network.h"

namespace flowsolver {

class DynamicMaintainOptimality {
public:
	// TODO(adam): Can I generalize this to different types of networks?
	// Note we assume the following sign convention for reduced cost:
	// RC(u,v) = C(u,v) - potentials[u] + potentials[v]
	DynamicMaintainOptimality(ResidualNetwork &g,
			                      std::vector<uint64_t> &potentials)
                            : g(g), potentials(potentials) {};
	virtual ~DynamicMaintainOptimality() {};

	Arc *getArc(uint32_t src, uint32_t dst);

	void addNode(uint32_t id);
	void setSupply(uint32_t id, int64_t supply);
	void removeNode(uint32_t id);

	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void changeArcCost(uint32_t src, uint32_t dst, int64_t cost);
	// N.B. this always returns true, as we decrease flow if necessary in order
	// to satisfy capacity constraint
  bool changeArcCapacity(uint32_t src, uint32_t dst, uint64_t capacity);
	void removeArc(uint32_t src, uint32_t dst);
private:
	ResidualNetwork &g;
	std::vector<uint64_t> &potentials;
};

} /* namespace flowsolver */

#endif /* LIB_DYNAMIC_MAINTAIN_OPTIMALITY_H_ */
