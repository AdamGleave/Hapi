#include "task_assignment.h"

#include <fstream>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "dimacs.h"
#include "flow_network.h"

namespace fs = flowsolver;

const std::string GRAPH_PATH =
											CMAKE_SRC_DIR "/graphs/clusters/synthetic/firmament/";
const std::string GRAPH_FILENAME = GRAPH_PATH + "graph_100m_8j_100t_10p.in";

class NetworkConversionTest : public ::testing::Test {
protected:
	std::fstream graph_file;
	fs::FlowNetwork *flow_network;
	fs::ResidualNetwork *residual_network;

	void SetUp() {
		graph_file.open(GRAPH_FILENAME, std::fstream::in);
		ASSERT_FALSE(graph_file.fail()) << "unable to open " << GRAPH_FILENAME;

		flow_network = fs::DIMACSOriginalImporter<fs::FlowNetwork>(graph_file).read();
		ASSERT_NE(flow_network, nullptr) << "DIMACS parse error";
	  // rewind
		graph_file.clear();
		graph_file.seekg(0);
		residual_network = fs::DIMACSOriginalImporter<fs::ResidualNetwork>(graph_file).read();
		ASSERT_NE(residual_network, nullptr) << "DIMACS parse error";
	}
};

TEST_F(NetworkConversionTest, FlowToResidual) {
	fs::ResidualNetwork converted(*flow_network);
	ASSERT_EQ(converted, *residual_network);
}

TEST_F(NetworkConversionTest, ResidualToFlow) {
	fs::FlowNetwork converted(*residual_network);
	ASSERT_EQ(converted, *flow_network);
}

template<class T>
class NetworkEqualityTest : public ::testing::Test {
protected:
	std::fstream graph_file;
	T *g1, *g2;

	void SetUp() {
		graph_file.open(GRAPH_FILENAME, std::fstream::in);
		ASSERT_FALSE(graph_file.fail()) << "unable to open " << GRAPH_FILENAME;

		g1 = fs::DIMACSOriginalImporter<T>(graph_file).read();

		graph_file.clear();
		graph_file.seekg(0);
		g2 = fs::DIMACSOriginalImporter<T>(graph_file).read();
	}
};

typedef ::testing::Types<fs::FlowNetwork, fs::ResidualNetwork> networks;
TYPED_TEST_CASE(NetworkEqualityTest, networks);

TYPED_TEST(NetworkEqualityTest, IdenticalGraphsEqual) {
	ASSERT_EQ(*(this->g1), *(this->g2));
}

TYPED_TEST(NetworkEqualityTest, DifferentNumberOfArcsUnequal) {
	this->g2->addArc(1, 2, 0, 1);
	ASSERT_NE(*(this->g1), *(this->g2));
}

TYPED_TEST(NetworkEqualityTest, DifferentNodeSupplyUnequal) {
	this->g2->setSupply(2, 42);
	ASSERT_NE(*(this->g1), *(this->g2));
}
