#ifndef LIB_RESIDUALNETWORK_H_
#define LIB_RESIDUALNETWORK_H_

#include <cinttypes>
#include <set>
#include <unordered_map>
#include <vector>
#include <functional>

#include <boost/iterator/transform_iterator.hpp>

#include "arc.h"

namespace flowsolver {

class ResidualNetwork {
public:
	explicit ResidualNetwork(uint32_t num_nodes);
	virtual ~ResidualNetwork();

	uint32_t getNumNodes() const;
	uint32_t getNumArcs() const;
	int64_t getBalance(uint32_t id) const;
	int64_t getSupply(uint32_t id) const;
	const std::set<uint32_t>& getSinks() const;
	const std::set<uint32_t>& getSources() const;

	/* returns NULL if no such Arc present */
	Arc *getArc(uint32_t src, uint32_t dst) const;
	const std::unordered_map<uint32_t, Arc*>& getAdjacencies(uint32_t src) const;

	// returns node ID
	uint32_t addNode();
	void removeNode(uint32_t id);
	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	// returns true if capacity constraint still satisfied, false otherwise
	bool changeArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void removeArc(uint32_t src, uint32_t dst);

	void setSupply(uint32_t id, int64_t supply);
	void pushFlow(uint32_t src, uint32_t dst, int64_t amount);
private:
	friend class const_noconst_iterator;
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

	uint32_t num_nodes;
	std::vector<int64_t> balance;
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
	typedef typename std::conditional
			<is_const_iterator, const ResidualNetwork *, ResidualNetwork *>
					::type FlowNetworkType;
	typedef typename std::conditional
			<is_const_iterator, const Arc &, Arc &>::type ArcType;

	typedef typename std::conditional<is_const_iterator,
			std::vector<std::unordered_map<uint32_t, Arc*>>::const_iterator,
				std::vector<std::unordered_map<uint32_t, Arc*>>::iterator>::type
			VectorIterator;
	typedef typename std::conditional<is_const_iterator,
					std::unordered_map<uint32_t, Arc*>::const_iterator,
						std::unordered_map<uint32_t, Arc*>::iterator>::type
					MapIterator;
	typename std::conditional<is_const_iterator, const Arc, Arc> value_type;

	FlowNetworkType g;
	VectorIterator vec_it;
	MapIterator map_it;

public:
	const_noconst_iterator() : g(0) { }

	const_noconst_iterator(FlowNetworkType g) : g(g) {
		vec_it = g->arcs.begin();
		map_it = vec_it->begin();

		// The first element of the vector may be an empty map: if so, skip until
		// we find an element. But if arcs is completely empty, do nothing.
		if (vec_it != g->arcs.end()) {
			// g->arcs not empty
			nextValid();
		}
	}

	const_noconst_iterator(FlowNetworkType g, bool) : g(g) {
		// for end sentinel
		vec_it = g->arcs.end();
	}

	// Copy constructor. Implicit conversion from regular iterator to
	// const_iterator.
	const_noconst_iterator(const const_noconst_iterator<false>& other) :
		g(other.g), vec_it(other.vec_it), map_it(other.map_it) {}

	ArcType operator*() const {
		return *(map_it->second);
	}

	// keep iterating until we reach a valid element
	void nextValid() {
		while (map_it == vec_it->end()) {
			// end of map, go on to next value in vector
			// (may need to do this repeatedly if maps are empty)
			vec_it++;
			if (vec_it == g->arcs.end()) {
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
			if (vec_it == g->arcs.end()) {
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
