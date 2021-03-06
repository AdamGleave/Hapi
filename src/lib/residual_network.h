#ifndef LIB_RESIDUALNETWORK_H_
#define LIB_RESIDUALNETWORK_H_

#include <cinttypes>
#include <set>
#include <unordered_map>
#include <vector>
#include <functional>

#include "arc.h"

// forward declare
namespace flowsolver {
class ResidualNetwork;
}
#include "flow_network.h"

namespace flowsolver {

class ResidualNetwork {
private:
	explicit ResidualNetwork(uint32_t num_nodes);
public:
	ResidualNetwork(uint32_t num_nodes, uint32_t num_arcs);
	ResidualNetwork(const ResidualNetwork &g);
	ResidualNetwork(const FlowNetwork &g);
	virtual ~ResidualNetwork();

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
	int64_t getBalance(uint32_t id) const;
	int64_t getSupply(uint32_t id) const;
	const std::set<uint32_t>& getSinks() const;
	const std::set<uint32_t>& getSources() const;

	/* returns NULL if no such Arc present */
	Arc *getArc(uint32_t src, uint32_t dst) const;
	const std::unordered_map<uint32_t, Arc*>& getAdjacencies(uint32_t src) const;

	void addNode(uint32_t id);
	void removeNode(uint32_t id);
	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void changeArcCost(uint32_t src, uint32_t dst, int64_t cost);
	// returns true if capacity constraint still satisfied, false otherwise
	bool changeArcCapacity(uint32_t src, uint32_t dst, uint64_t capacity);
	void removeArc(uint32_t src, uint32_t dst);

	void setSupply(uint32_t id, int64_t supply);
	void pushFlow(uint32_t src, uint32_t dst, int64_t amount);
	// favour use of the above method, but this is needed for interface
	// compatibility with FlowNetwork, used in DIMACSFlowImporter
	void pushFlow(Arc &arc, uint32_t src_id, int64_t amount);

	bool operator==(const ResidualNetwork &g) const;
	bool operator!=(const ResidualNetwork &g) const;

	static bool adjacencyEquals(const std::unordered_map<uint32_t, Arc*> &adj1,
			 	 	 	 	 	 	 	 	 	 	 	  const std::unordered_map<uint32_t, Arc*> &adj2);
private:
	template<bool is_const_iterator = true>
	class const_noconst_iterator;

public:
	typedef const_noconst_iterator<false> iterator;
	typedef const_noconst_iterator<true> const_iterator;

	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;

private:
	bool validID(uint32_t id) const;
	void changeBalance(uint32_t id, int64_t delta);

	uint32_t num_nodes;
	std::vector<int64_t> balances, supplies;
	std::vector<std::unordered_map<uint32_t, Arc*>> arcs;
	std::set<uint32_t> sources;
	std::set<uint32_t> sinks;
	std::set<uint32_t> free_nodes;
};

template<bool is_const_iterator>
class ResidualNetwork::const_noconst_iterator : public std::iterator
		<std::forward_iterator_tag,
		 typename std::conditional<is_const_iterator, const Arc, Arc>::type>
{
private:
	typedef typename std::vector<std::unordered_map<uint32_t, Arc*>> AT;
	typedef typename std::conditional<is_const_iterator, const AT, AT>::type
			    *AdjacencyType;

	typedef typename std::conditional<is_const_iterator,
			std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator,
				std::vector<std::unordered_map<uint32_t, Arc*>>::iterator>::type
			VectorIterator;
	typedef typename std::conditional<is_const_iterator,
					std::unordered_map<uint32_t, Arc*>::const_iterator,
						std::unordered_map<uint32_t, Arc*>::iterator>::type
					MapIterator;

	AdjacencyType arcs;
	VectorIterator vec_it;
	MapIterator map_it;

public:
	typedef typename std::conditional<is_const_iterator, const Arc, Arc>::type value_type;

	const_noconst_iterator() : arcs(0) { }

	const_noconst_iterator(AdjacencyType arcs) : arcs(arcs) {
		vec_it = arcs->begin();
		map_it = vec_it->begin();

		// The first element of the vector may be an empty map: if so, skip until
		// we find an element. But if arcs is completely empty, do nothing.
		if (vec_it != arcs->end()) {
			// arcs not empty
			nextValid();
		}
	}

	const_noconst_iterator(AdjacencyType arcs, bool) : arcs(arcs) {
		// for end sentinel
		vec_it = arcs->end();
	}

	// Copy constructor. Implicit conversion from regular iterator to
	// const_iterator.
	const_noconst_iterator(const const_noconst_iterator<false>& other) :
		arcs(other.arcs), vec_it(other.vec_it), map_it(other.map_it) {}

	value_type &operator*() const {
		VLOG(3) << "Returning " << (vec_it - arcs->begin()) << "->" << map_it->first;
		return *(map_it->second);
	}

	// keep iterating until we reach a valid element
	void nextValid() {
		while (map_it == vec_it->end()) {
			// end of map, go on to next value in vector
			// (may need to do this repeatedly if maps are empty)
			++vec_it;
			if (vec_it == arcs->end()) {
				// no elements left to iterate over: end
				break;
			}
			map_it = vec_it->begin();
		}
	}

	// prefix operator
	const_noconst_iterator<is_const_iterator> operator++() {
		map_it++;
		nextValid();
		return *this;
	}

	const_noconst_iterator<is_const_iterator> operator++(int) {
		// postfix operator
		const_noconst_iterator<is_const_iterator> old(*this);
		++(*this);
		return old;
	}

	bool operator==(const const_noconst_iterator<is_const_iterator>& it)
			const {
		if (vec_it == it.vec_it) {
			if (vec_it == arcs->end()) {
				// End iterator not unique.
				// The end sentinel has map_it set to default value.
				// By contrast, the end iterator reached by successive
				// applications of the ++ operator has map_it set to the
				// last legal value.
				return true;
			} else {
				return map_it == it.map_it;
			}
		} else {
			return false;
		}
	}

	bool operator!=(const const_noconst_iterator<is_const_iterator>& it)
			const {
		return !(*this == it);
	}

	friend class const_noconst_iterator<true>;
};

} /* namespace flowsolver */

#endif /* LIB_RESIDUALNETWORK_H_ */
