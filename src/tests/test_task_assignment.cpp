#include <fstream>

#include <gtest/gtest.h>

#include "flow_network.h"
#include "dimacs.h"
#include "task_assignment.h"

// TODO: test graphs with invalid format as well?
// TODO: test graphs with & without unscheduled tasks?
// TODO: death test when graph without flow?
namespace fs = flowsolver;

class TaskAssignmentTest : public ::testing::Test {
protected:
	std::fstream graph_file;
	std::fstream solved_file;

	fs::FlowNetwork *original;
	fs::FlowNetwork *augmented;

	TaskAssignmentTest() {
		graph_file.open("graph_4m_2crs_6j.in");
		solved_file.open("graph_4m_2crs_6j.s");

		original = fs::DIMACS<fs::FlowNetwork>::readDIMACSMin(graph_file);
		augmented = new fs::FlowNetwork(*original);
		fs::DIMACS<fs::FlowNetwork>::readDIMACSMinFlow(solved_file, *augmented);
	}
};

TEST_F(TaskAssignmentTest, Constructor) {
	fs::TaskAssignment ta(*original);

	const std::unordered_set<uint32_t> &tasks = ta.getTasks();
	std::unordered_set<uint32_t> expected_tasks = {2,3,4,5,6,7};
	EXPECT_EQ(tasks, expected_tasks);

	const std::unordered_set<uint32_t> &leaves = ta.getLeaves();
	std::unordered_set<uint32_t> expected_leaves = {16,17,18,19,20,21,22,23};
	EXPECT_EQ(leaves, expected_leaves);
}

TEST_F(TaskAssignmentTest, Assignments) {
	fs::TaskAssignment ta(*original);

	std::unordered_map<uint32_t, uint32_t> *assignments;
	assignments = ta.getAssignments(*augmented);

	std::unordered_map<uint32_t, uint32_t> expected_assignments = {
			{ 2, 16 },
			{ 3, 18 },
			{ 4, 20 },
			{ 5, 23 },
			{ 6, 22 },
			{ 7, 17 }
	};

	EXPECT_EQ(*assignments, expected_assignments);
	delete assignments;
}
