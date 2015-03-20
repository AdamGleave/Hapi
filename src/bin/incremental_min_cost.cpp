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

	// load full file
	ResidualNetwork *g = DIMACSIncrementalFullImporter<ResidualNetwork>
	                                                            (std::cin).read();

	// solve full problem
	t.start();
	AugmentingPath ap(*g);
	ap.run();
	t.stop(); t.report();

	DIMACSExporter<ResidualNetwork> exporter(*g, std::cout);
	exporter.writeFlow();

	// now solve incremental problem
	DynamicMaintainOptimality dynamic(*g, ap.getPotentials());
	DIMACSIncrementalDeltaImporter<DynamicMaintainOptimality>
	                              incremental_importer(std::cin, dynamic);

	// process stream of incremental deltas, solving incremental problem
	// TODO: am including time spent parsing in reported ALGOTIME
	t.start();
	while (incremental_importer.read()) {
		ap.reoptimize();
		t.stop();
		t.report();
		DIMACSExporter<ResidualNetwork>(*g, std::cout).writeFlow();
		t.start();
	}
	t.stop();

	return 0;
}
