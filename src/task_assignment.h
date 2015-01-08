#ifndef TASK_ASSIGNMENT_H_
#define TASK_ASSIGNMENT_H_

#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include <unordered_set>

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
	std::unordered_set<uint32_t> tasks, leaves;

	std::vector<std::unordered_map<uint32_t,int64_t>>
		*extractFlow(const FlowNetwork &g);

	void decrementFlow(std::unordered_map<uint32_t, int64_t> &adjacencies,
					   uint32_t id);

	uint32_t findLeafAssignment
		(std::vector<std::unordered_map<uint32_t,int64_t>> &flows, uint32_t id);
public:
	TaskAssignment(const FlowNetwork &g);
	virtual ~TaskAssignment();
	std::unordered_map<uint32_t, uint32_t> *getAssignments(const FlowNetwork &g);
};

} /* namespace flowsolver */

#endif /* TASK_ASSIGNMENT_H_ */
