/*
 * FlowNetwork.h
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#ifndef SRC_FLOWNETWORK_H_
#define SRC_FLOWNETWORK_H_

#include <cinttypes>
#include <vector>
#include <unordered_map>

#include "Arc.h"

namespace flowsolver {

class FlowNetwork {
public:
	FlowNetwork(uint32_t num_nodes);
	uint32_t getNumNodes() const;
	uint32_t getNumArcs() const;
	int64_t getSupply(uint32_t id) const;
	void setSupply(uint32_t id, int64_t supply);
	void addEdge(uint32_t src, uint32_t dst, uint64_t capacity, int64_t cost);
	void pushFlow(uint32_t src, uint32_t dst, int64_t amount);
	virtual ~FlowNetwork();

	class iterator;
	friend class iterator;

	class iterator {
	private:
		FlowNetwork &g;
		std::vector<std::unordered_map<uint32_t, Arc*>>::iterator vec_it;
		std::unordered_map<uint32_t, Arc*>::iterator map_it;

	public:
		iterator(FlowNetwork &g) : g(g) {
			vec_it = g.arcs.begin();
			map_it = vec_it->begin();
		};

		iterator(FlowNetwork &g, bool) : g(g) {
			// for end sentinel
			vec_it = g.arcs.end();
		}

		Arc &operator*() const {
			return *(map_it->second);
		}

		Arc &operator++() {
			// prefix operator
			if (map_it == vec_it->end()) {
				vec_it++;
				map_it = vec_it->begin();
			} else {
				map_it++;
			}
			return *(map_it->second);
		}

		Arc &operator++(int) {
			// postfix operator
			Arc &arc = *(map_it->second);
			++(*this);
			return arc;
		}

		bool operator==(const iterator& it) const {
			if (vec_it == it.vec_it) {
				if (vec_it == g.arcs.end()) {
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

		bool operator!=(const iterator& it) const {
			return !(*this == it);
		}
	};

	iterator begin() { return iterator(*this); }
	iterator end() { return iterator(*this, true); }

private:
	uint32_t num_nodes;
	int64_t *supply;
	std::vector<std::unordered_map<uint32_t, Arc*>> arcs;
};

} /* namespace flowsolver */

#endif /* SRC_FLOWNETWORK_H_ */
