#ifndef LIB_BERTSEKASNETWORK_h_
#define LIB_BERTSEKASNETWORK_h_

#include "RelaxIV.h"

namespace flowsolver {

// TODO(adam): Is this specialised in any way to Bertsekas? Good if you can keep
// it general.
class BertsekasNetwork {
public:
	// TODO(adam): better way than just guessing number of nodes & arcs?
	explicit BertsekasNetwork(uint32_t num_nodes, uint32_t num_arcs);
	virtual ~BertsekasNetwork();

	// SOMEDAY(adam): This is a hack. It would be good to at least rename the
	// functions to make it clearer what they do, although this would require
	// changing the interface for FlowNetwork too.
	/*
	 * getNumNodes() >= getNumNodesPresent()
	 *  The latter returns the number of nodes in the graph. The former returns
	 *  the currently allocated *capacity* of ResidualNetwork. In other words,
	 *  getNumNodesPresent() is the minimum capacity necessary to store the graph.
	 *
	 *  Initially, the two are equal. When removeNode() is invoked,
	 *  getNumNodesPresent() decreases but getNumNodes() is unchanged.
	 */
	uint32_t getNumNodes() const;
	uint32_t getNumNodesAllocated() const;
	uint32_t getNumArcs() const;
	uint32_t getNumArcsAllocated() const;
	int64_t getBalance(uint32_t id) const;
	int64_t getSupply(uint32_t id) const;

//	const std::set<uint32_t>& getSinks() const;
//	const std::set<uint32_t>& getSources() const;

	/* returns NULL if no such Arc present */
	// not strictly needed? just to guard against duplicate arcs
	Arc *getArc(uint32_t src, uint32_t dst) const;
//	const std::unordered_map<uint32_t, Arc*>& getAdjacencies(uint32_t src) const;

	void addNode(uint32_t id);
	void removeNode(uint32_t id);
	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void changeArcCost(uint32_t src, uint32_t dst, int64_t cost);
	// returns true if capacity constraint still satisfied, false otherwise
	bool changeArcCapacity(uint32_t src, uint32_t dst, uint64_t capacity);
	void removeArc(uint32_t src, uint32_t dst);

	void setSupply(uint32_t id, int64_t supply);
private:
};

}
/* namespace flowsolver */

#endif /* LIB_BERTSEKASNETWORK_h_ */
