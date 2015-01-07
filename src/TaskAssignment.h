#ifndef TASKASSIGNMENT_H_
#define TASKASSIGNMENT_H_

#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include <set>

#include <glog/logging.h>

namespace flowsolver {

class TaskAssignment {
	/*
	 * Assumptions:
	 *  - No source node
	 * 	- Sink node has ID 1
	 * 	- A node is a task iff. it has supply 1.
	 * 	- Leaf nodes are all those nodes adjacent to the sink node.
	 * 	  (This includes both computers and unscheduled aggregators.)
	 */
	const uint32_t SINK_NODE = 1;
	// TODO: unordered_set?
	std::set<uint32_t> tasks, leaves;

	std::vector<std::unordered_map<uint32_t,int64_t>>
	  *extractFlow(const FlowNetwork &g) {
		std::vector<std::unordered_map<uint32_t, int64_t>> *res;
		res = new std::vector<std::unordered_map<uint32_t, int64_t>>;
		res->reserve(g.getNumNodes());

		FlowNetwork::const_iterator it;
		for (it = g.begin(); it != g.end(); ++it) {
			const Arc &arc = *it;
			int64_t flow = arc.getFlow();
			if (flow > 0) {
				std::pair<uint32_t, int64_t> item(arc.getSrcId(), flow);
				(*res)[arc.getDstId()].insert(item);
			}
		}

		return res;
	}

	void decrementFlow(std::unordered_map<uint32_t, int64_t> &adjacencies,
					   uint32_t id) {
		int64_t flow = adjacencies[id];
		flow--;

		if (flow == 0) {
			adjacencies.erase(id);
		} else {
			adjacencies[id] = flow;
		}
	}

	uint32_t findLeafAssignment
	  (std::vector<std::unordered_map<uint32_t,int64_t>> &flows, uint32_t id) {
		std::unordered_map<uint32_t, int64_t> &adjacencies = flows[id];

		// try to find a direct edge to worker or root
		std::unordered_map<uint32_t, int64_t>::iterator map_it;
		for (map_it = adjacencies.begin();
			 map_it != adjacencies.end(); ++map_it) {
			uint32_t src_id = map_it->first;
			if (tasks.count(src_id) > 0) {
				// src_id is root or worker task

				// note we mutate the collection in the iterator loop. This is
				// safe since we return, so do not use the iterator again.
				decrementFlow(adjacencies, src_id);

				return src_id;
			}
		}

		// no direct edge: go back a layer
		if (!adjacencies.empty()) {
			map_it = adjacencies.begin();
			uint32_t src_id = map_it->first;

			decrementFlow(adjacencies, src_id);

			return findLeafAssignment(flows, src_id);
		}

		// leaf node not assigned.
		return 0;
	}
public:
	TaskAssignment(const FlowNetwork &g) {
		CHECK(g.getBalance(SINK_NODE) < 0) << "node 1 is not the sink";

		// find all task nodes
		uint32_t num_nodes = g.getNumNodes();
		for (uint32_t id = 1; id <= num_nodes; id++) {
			if (id == SINK_NODE) {
				continue;
			}

			uint32_t balance = g.getBalance(id);
			if (balance > 0) {
				CHECK_EQ(balance, 1) << "node " << id
									 << " has balance " << balance
									 << " (does graph contain a source node?)";
				tasks.insert(id);
			} else {
				CHECK_EQ(balance,0) << "non-sink node " << id
									<< " has negative balance" << balance;
			}
		}

		// find all leaf nodes
		// TODO: this is not part of generic Graph interface
		// could add getAdjacenciesBegin and getAdjacenciesEnd?
		leaves(g.getAdjacenciesBegin(SINK_NODE), g.getAdjacenciesEnd(SINK_NODE));
	}

	std::unordered_map<uint32_t, uint32_t> *getAssignments(const FlowNetwork &g) {
		std::vector<std::unordered_map<uint32_t, int64_t>> *flow;
		flow = extractFlow(g);
		std::unordered_map<uint32_t, int64_t> &sink_adjacencies = (*flow)[SINK_NODE];

		std::unordered_map<uint32_t, uint32_t> *res;
		res = new std::unordered_map<uint32_t, uint32_t>();

		std::set<uint32_t>::iterator it;
		for (it = leaves.begin(); it != leaves.end(); ++it) {
			uint32_t leaf_id = *it;

			if (sink_adjacencies.count(leaf_id) > 0) {
				// leaf_id is assigned to some task

				uint32_t task_id = findLeafAssignment(*flow, leaf_id);
				if (task_id != 0) {
					res->insert(std::pair<uint32_t, uint32_t>(task_id, leaf_id));
				} else {
					LOG(FATAL) << "No task mapping for leaf " << leaf_id
							   << " despite positive flow to sink.";
				}
			}
		}

		delete flow;
		return res;
	}

	virtual ~TaskAssignment();
};

} /* namespace flowsolver */

#endif /* TASKASSIGNMENT_H_ */
