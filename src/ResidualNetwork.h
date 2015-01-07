#ifndef SRC_RESIDUALNETWORK_H_
#define SRC_RESIDUALNETWORK_H_

#include <cinttypes>
#include <set>
#include <unordered_map>
#include <vector>
#include <functional>

#include <boost/iterator/transform_iterator.hpp>

#include "Arc.h"

namespace flowsolver {

struct ExtractArc {
	Arc *operator()(std::unordered_map<uint32_t, Arc*>::iterator it) {
		return it->second;
	}
};


class ResidualNetwork {
	uint32_t num_nodes;
	std::vector<int64_t> balance, supply;
	std::vector<std::unordered_map<uint32_t, Arc*>> arcs;
	std::set<uint32_t> sources;
	std::set<uint32_t> sinks;

	void updateSupply(uint32_t id, int64_t delta);
public:
	ResidualNetwork(uint32_t num_nodes);
	uint32_t getNumNodes() const;
	uint32_t getNumArcs() const;
	int64_t getBalance(uint32_t id) const;
	int64_t getSupply(uint32_t id) const;
	const std::set<uint32_t>& getSinks() const;
	const std::set<uint32_t>& getSources() const;
	void setSupply(uint32_t id, int64_t supply);
	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void pushFlow(uint32_t src, uint32_t dst, int64_t amount);

	std::unordered_map<uint32_t, Arc*> &getAdjacencies(uint32_t src);

	typedef boost::transform_iterator
			<std::function<Arc*(std::unordered_map<uint32_t, Arc *>::iterator)>,
			 std::unordered_map<uint32_t, Arc *>::iterator> adjacencies_iterator;
	adjacencies_iterator getAdjacenciesBegin(uint32_t src) {
		return boost::make_transform_iterator
									 (getAdjacencies(src).begin(), ExtractArc());
	}
	/*
	adjacencies_iterator getAdjacenciesEnd(uint32_t src) {
		return boost::make_transform_iterator
									 (getAdjacencies(src).end(), extractArc);
	} */

	/* returns NULL if no such Arc present */
	Arc *getArc(uint32_t src, uint32_t dst);

	friend class const_noconst_iterator;

	template<bool is_const_iterator = true>
	class const_noconst_iterator : public std::iterator
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

		const_noconst_iterator<is_const_iterator> operator++() {
			map_it++;
			// prefix operator
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

	typedef const_noconst_iterator<false> iterator;
	typedef const_noconst_iterator<true> const_iterator;

	iterator begin() { return iterator(this); }
	const_iterator begin() const { return const_iterator(this); }
	iterator end() { return iterator(this, true); }
	const_iterator end() const { return const_iterator(this, true); }

	virtual ~ResidualNetwork();
};

} /* namespace flowsolver */

#endif /* SRC_RESIDUALNETWORK_H_ */
