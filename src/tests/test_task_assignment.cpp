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

	const std::string GRAPH_PATH =
			CMAKE_SRC_DIR "/graphs/clusters/synthetic/handmade/";
	const std::string GRAPH_FILENAME = GRAPH_PATH + "task_assignments.in";
	const std::string SOLUTION_FILENAME = GRAPH_PATH + "task_assignments.s";
	void SetUp() {
		graph_file.open(GRAPH_FILENAME, std::fstream::in);
		ASSERT_FALSE(graph_file.fail()) << "unable to open " << GRAPH_FILENAME;
		solved_file.open(SOLUTION_FILENAME, std::fstream::in);
		ASSERT_FALSE(solved_file.fail()) << "unable to open " << SOLUTION_FILENAME;

		original = fs::DIMACS<fs::FlowNetwork>::readDIMACSMin(graph_file);
		ASSERT_NE(original, (void *)0) << "DIMACS parse error";
		augmented = new fs::FlowNetwork(*original);
		fs::DIMACS<fs::FlowNetwork>::readDIMACSMinFlow(solved_file, *augmented);
	}
};

TEST_F(TaskAssignmentTest, Constructor) {
	fs::TaskAssignment ta(*original);

	const std::unordered_set<uint32_t> &tasks = ta.getTasks();
	std::unordered_set<uint32_t> expected_tasks = {2,3,4,5,6,7};
	EXPECT_EQ(expected_tasks, tasks);

	const std::unordered_set<uint32_t> &leaves = ta.getLeaves();
	std::unordered_set<uint32_t> expected_leaves = {
			8, // unscheduled aggregator
			16,17,18,19,20,21,22,23 // cores
	};
	EXPECT_EQ(expected_leaves, leaves);
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

	EXPECT_EQ(expected_assignments, *assignments);
	delete assignments;
}
