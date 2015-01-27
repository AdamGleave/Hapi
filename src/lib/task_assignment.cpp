#include <numeric>

#include "task_assignment.h"

namespace flowsolver {

TaskAssignment::TaskAssignment(const FlowNetwork &g) {
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
	VLOG(0) << "# of task nodes: " << tasks.size();

	// find all leaf nodes
	const std::forward_list<Arc *> &sink_adjacencies = g.getAdjacencies(SINK_NODE);
	std::forward_list<Arc *>::const_iterator it;
	for (it = sink_adjacencies.begin(); it != sink_adjacencies.end(); ++it) {
		Arc *arc = *it;
		uint32_t src = arc->getSrcId();
		uint32_t dst = arc->getDstId();
		if (src == SINK_NODE) {
			leaves.insert(dst);
		} else {
			leaves.insert(src);
		}
	}
	VLOG(0) << "# of leaf nodes: " << leaves.size();
}

TaskAssignment::~TaskAssignment() { }

std::vector<std::unordered_map<uint32_t,int64_t>>
	  *TaskAssignment::extractFlow(const FlowNetwork &g) {
	std::vector<std::unordered_map<uint32_t, int64_t>> *res;
	res = new std::vector<std::unordered_map<uint32_t, int64_t>>;
	res->assign(g.getNumNodes() + 1, std::unordered_map<uint32_t, int64_t>());

	FlowNetwork::const_iterator it;
	for (it = g.begin(); it != g.end(); ++it) {
		const Arc &arc = *it;
		int64_t flow = arc.getFlow();
		if (flow > 0) {
			std::pair<uint32_t, int64_t> item(arc.getSrcId(), flow);
			std::unordered_map<uint32_t, int64_t> &flow_map = (*res)[arc.getDstId()];
			flow_map.insert(item);
			//flow_map[arc.getSrcId()] = flow;
		}
	}

	return res;
}

void TaskAssignment::decrementFlow
	(std::unordered_map<uint32_t, int64_t> &adjacencies, uint32_t id) {
	int64_t flow = adjacencies[id];
	CHECK_GT(flow, 0);
	flow--;

	if (flow == 0) {
		adjacencies.erase(id);
	} else {
		adjacencies[id] = flow;
	}
}

uint32_t TaskAssignment::findLeafAssignment
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

int addFlow(int64_t acc, std::pair<uint32_t, int64_t> p) {
	return acc + p.second;
}

std::unordered_map<uint32_t, uint32_t>
  *TaskAssignment::getAssignments(const FlowNetwork &g) {
	std::vector<std::unordered_map<uint32_t, int64_t>> *flow;
	flow = extractFlow(g);
	std::unordered_map<uint32_t, int64_t> &sink_adjacencies = (*flow)[SINK_NODE];

	std::unordered_map<uint32_t, uint32_t> *res;
	res = new std::unordered_map<uint32_t, uint32_t>();

	std::unordered_set<uint32_t>::iterator it;
	VLOG(0) << "# of nodes draining into sink: " << sink_adjacencies.size();
	VLOG(1) << "value of flow: "
			<< std::accumulate(sink_adjacencies.begin(), sink_adjacencies.end(), 0, addFlow);
	for (it = leaves.begin(); it != leaves.end(); ++it) {
		uint32_t leaf_id = *it;

		int64_t sink_flow;
		if (sink_adjacencies.count(leaf_id) > 0) {
			sink_flow = sink_adjacencies[leaf_id];
		} else {
			sink_flow = 0;
		}
		for (; sink_flow > 0; sink_flow--) {
			// leaf_id is assigned to some task. (in the case of unscheduled
			// aggregators, may be assigned to many tasks, so loop)

			uint32_t task_id = findLeafAssignment(*flow, leaf_id);
			if (task_id != 0) {
				res->insert(std::pair<uint32_t, uint32_t>(task_id, leaf_id));
				decrementFlow(sink_adjacencies, leaf_id);
			} else {
				LOG(FATAL) << "No task mapping for leaf " << leaf_id
						   << " despite positive flow "
						   << sink_adjacencies[leaf_id] << " to sink.";
			}
		}
	}

	delete flow;
	return res;
}

}
