/* Parses a standard DIMACS input file, solves the min-cost flow problem.
 * Parses an extended DIMACS input file specifying change(s) to the graph,
 * and solves it using an incremental approach. Outputs DIMACS representation
 * of the solution. */

#include "augmenting_path.h"

#include <iostream>
#include <fstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <glog/logging.h>

#include "residual_network.h"
#include "dynamic_maintain_optimality.h"
#include "dimacs.h"

#define TIMER_FORMAT "ALGOTIME: %w\n"

using namespace flowsolver;

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	// for timing algorithms
	boost::timer::auto_cpu_timer t(std::cerr, TIMER_FORMAT);
	t.stop();

	std::string usage = "usage: " + std::string(argv[0])
	                  + "<command> <full graph> <incremental delta>";
	if (argc != 4) {
		std::cerr << usage << std::endl;
		return -1;
	}

	std::string graph_fname = argv[2], incremental_fname = argv[3];
	std::ifstream graph_file(graph_fname), incremental_file(incremental_fname);
	CHECK(!graph_file.fail()) << "could not open " << graph_fname;
	CHECK(!incremental_file.fail()) << "could not open " << incremental_fname;

	// load full file
	ResidualNetwork *g = DIMACSOriginalImporter<ResidualNetwork>(graph_file).read();
	CHECK(g != nullptr) << "DIMACS parsing error in " << graph_fname;

	// solve full problem
	AugmentingPath ap(*g);
	ap.run();

	std::string command = argv[1];
	if (command == "incremental") {
		// now solve incremental problem
		t.start();
		DynamicMaintainOptimality dynamic(*g, ap.getPotentials());
		typedef DIMACSIncrementalImporter<DynamicMaintainOptimality> DIMACSImporter;
		DIMACSImporter(incremental_file, dynamic).read();
		ap.reoptimize();
		t.stop();
		t.report();
		DIMACSExporter<ResidualNetwork>(*g, std::cout).writeFlow();
	} else if (command == "partial") {
		// just maintain optimality, and print out state of graph
		DynamicMaintainOptimality dynamic(*g, ap.getPotentials());
		typedef DIMACSIncrementalImporter<DynamicMaintainOptimality> DIMACSImporter;
		DIMACSImporter(incremental_file, dynamic).read();
		DIMACSExporter<ResidualNetwork>(*g, std::cout).write();
	}

	return 0;
}
