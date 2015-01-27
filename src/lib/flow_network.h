#ifndef LIB_FLOW_NETWORK_H_
#define LIB_FLOW_NETWORK_H_

#include <cinttypes>
#include <vector>
#include <forward_list>

#include <glog/logging.h>

#include "arc.h"

namespace flowsolver {

class FlowNetwork {
	uint32_t num_nodes, num_arcs;
	std::vector<int64_t> balances;
	std::vector<std::forward_list<Arc *>> arcs;
public:
	FlowNetwork(uint32_t num_nodes);
	// constant time
	uint32_t getNumNodes() const;
	// linear in number of arcs
	uint32_t getNumArcs() const;
	// constant time
	void addArc(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	/*
	 * Linear in number of edges in src.
	 * Returns NULL if no such Arc present
	 */
	Arc *getArc(uint32_t src, uint32_t dst);
	std::forward_list<Arc *> &getAdjacencies(uint32_t src);
	const std::forward_list<Arc *> &getAdjacencies(uint32_t src) const;
	void setSupply(uint32_t id, int64_t supply);

	// SOMEDAY: This is inline as getBalance bottleneck; unnecessary if switch
	// to using list of active vertices
	inline int64_t getBalance(uint32_t id) const {
		CHECK_NE(id, 0) << "0 id is reserved";
		return balances[id];
	}

	int64_t getResidualCapacity(Arc &arc, uint32_t src_id);
	// returns balance of the node flow is pushed to
	int64_t pushFlow(Arc &arc, uint32_t src_id, uint64_t flow);

	// iterator
	friend class const_noconst_iterator;

	template<bool is_const_iterator = true>
	class const_noconst_iterator : public std::iterator
		  <std::forward_iterator_tag,
		   typename std::conditional<is_const_iterator, const Arc, Arc>::type>
	{
	private:
		typedef typename std::conditional
				<is_const_iterator, const FlowNetwork *, FlowNetwork *>
				::type FlowNetworkType;
		typedef typename std::conditional
				<is_const_iterator, const Arc, Arc>::type ArcType;

		typedef typename std::conditional<is_const_iterator,
				std::vector<std::forward_list<Arc *>>::const_iterator,
				std::vector<std::forward_list<Arc *>>::iterator>::type
				VectorIterator;
		typedef typename std::conditional<is_const_iterator,
						std::forward_list<Arc *>::const_iterator,
						std::forward_list<Arc *>::iterator>::type
						ListIterator;
		typename std::conditional<is_const_iterator, const Arc, Arc> value_type;

		FlowNetworkType g;
		VectorIterator vec_it;
		ListIterator list_it;

	public:
		const_noconst_iterator() : g(0) { }

		const_noconst_iterator(FlowNetworkType g) : g(g) {
			vec_it = g->arcs.begin();
			list_it = vec_it->before_begin();
			++(*this);
		}

		const_noconst_iterator(FlowNetworkType g, bool) : g(g) {
			// for end sentinel
			vec_it = g->arcs.end();
		}

		// Copy constructor. Implicit conversion from regular iterator to
		// const_iterator.
		const_noconst_iterator(const const_noconst_iterator<false>& other) :
			g(other.g), vec_it(other.vec_it), list_it(other.list_it) {}

		ArcType &operator*() const {
			return **list_it;
		}

		ArcType *operator->() const {
			return *list_it;
		}

		const_noconst_iterator<is_const_iterator> operator++() {
			/*
			 * Note each Arc is stored twice: in arcs[src][dst]
			 * and arcs[dst][src]. We want to return each Arc exactly once,
			 * so only return the arcs[src][dst] copy.
			 */
			list_it++;
			// prefix operator
			while (true) {
				if (list_it != vec_it->end()) {
					// not end of map
					Arc *arc = *list_it;
					uint32_t vec_index = vec_it - g->arcs.begin();
					if (arc->getSrcId() == vec_index) {
						// arcs[src][dst] copy
						break;
					} else {
						// arcs[dst][src] copy
						list_it++;
					}
				} else {
					// end of map
					// go on to next value in vector
					vec_it++;
					if (vec_it == g->arcs.end()) {
						// no elements left to iterate over: end
						break;
					}
					list_it = vec_it->begin();
				}
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
					return list_it == it.list_it;
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

	virtual ~FlowNetwork();
};

} /* namespace flowsolver */

#endif /* LIB_FLOW_NETWORK_H_ */
