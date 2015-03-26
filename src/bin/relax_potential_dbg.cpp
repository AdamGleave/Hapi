/* Parses a standard DIMACS input file, solves the min-cost flow problem.
 * Parses an extended DIMACS input file specifying change(s) to the graph,
 * and solves it using an incremental approach. Outputs DIMACS representation
 * of the solution. */

#include "augmenting_path.h"
#include "relax.h"

#include <iostream>
#include <fstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "residual_network.h"
#include "dynamic_maintain_optimality.h"
#include "dimacs.h"

const static std::string TIMER_FORMAT = "ALGOTIME: %w\n";

using namespace flowsolver;

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	namespace po = boost::program_options;

	po::options_description global("Global options");
	global.add_options()
			("help", "produce help message")
			("debug-partial-dir", po::value<std::string>(),
			 "debug option: enable generation of partial traces,"
			 " that is the state of the flow network before reoptimization but after"
			 " the incremental changes, maintaining optimality (but not feasibility)")
			 ("command", po::value<std::string>(),
				"command to execute: augmenting_path or relax.")
			 ("graph", po::value<std::string>(), "graph file.")
			 ("state", po::value<std::string>(), "state file.")
			 ("delta", po::value<std::string>(), "delta file.");

	po::positional_options_description pos;
	pos.add("command", 1);
	pos.add("graph", 1);
	pos.add("state", 1);
	pos.add("delta", 1);

	po::variables_map vm;

	po::parsed_options parsed = po::command_line_parser(argc, argv).
				options(global).
				positional(pos).
				run();
	po::store(parsed, vm);

	if (vm.count("help")) {
		std::cout << global << std::endl;
		return 0;
	}

	if (!vm.count("command")) {
		std::cerr << "must specify command" << std::endl;
		std::cerr << global << std::endl;
		return -1;
	}

	std::string cmd = vm["command"].as<std::string>();
	if (cmd != "augmenting_path" && cmd != "relax") {
		std::cerr << "unrecognised comand: " << cmd << std::endl;
		std::cerr << global << std::endl;
		return -1;
	}

	if (!vm.count("graph")) {
		std::cerr << "must specify graph" << std::endl;
		std::cerr << global << std::endl;
		return -1;
	}

	if (!vm.count("state")) {
			std::cerr << "must specify state" << std::endl;
			std::cerr << global << std::endl;
			return -1;
	}

	if (!vm.count("delta")) {
			std::cerr << "must specify graph" << std::endl;
			std::cerr << global << std::endl;
			return -1;
	}

	std::string graph_file = vm["graph"].as<std::string>();
	std::ifstream graph(graph_file);
	if (!graph.is_open()) {
		LOG(FATAL) << "Could not open for reading " << graph_file;
	}

	std::string state_file = vm["state"].as<std::string>();
	std::ifstream state(state_file);
	if (!state.is_open()) {
		LOG(FATAL) << "Could not open for reading " << state_file;
	}

	std::string delta_file = vm["delta"].as<std::string>();
	std::ifstream delta(delta_file);
	if (!delta.is_open()) {
		LOG(FATAL) << "Could not open for reading " << delta_file;
	}

	std::string partial_dir;
	if (vm.count("debug-partial-dir")) {
		partial_dir = vm["debug-partial-dir"].as<std::string>();
		struct stat st;
		if (stat(partial_dir.c_str(), &st) == -1) {
			// directory doesn't exist
			if (mkdir(partial_dir.c_str(), 0700) < 0) {
				LOG(FATAL) << "Error creating directory " << partial_dir
						       << ": " << strerror(errno);
			}
		} else {
			LOG(WARNING) << "Not overwriting previous traces in " << partial_dir
					         << " please remove them or specify a different directory.";
			partial_dir = "";
		}
	}

	// load full file
	ResidualNetwork *g = DIMACSOriginalImporter<ResidualNetwork>(graph).read();

	// solve full problem
	IncrementalSolver *is = NULL;
	if (cmd == "augmenting_path") {
		is = new AugmentingPath(*g);
	} else if (cmd == "relax") {
		is = new RELAX(*g);
	} else {
		// should have been caught during command-line parsing
		assert(false);
	}

	DIMACSFlowPotentialImporter<ResidualNetwork>(state, *g, *is).read();

	is->run();

	DIMACSPotentialExporter<ResidualNetwork> exporter(*g, std::cout, *is);

	exporter.writeFlow();
	exporter.writePotentials();

	std::cout << "c EOI" << std::endl;
	std::cout.flush();

	// now solve incremental problem
	DynamicMaintainOptimality dynamic(*g, *is);
	DIMACSIncrementalDeltaImporter<DynamicMaintainOptimality>
	                              incremental_importer(delta, dynamic);

	// process stream of incremental deltas, solving incremental problem
	// TODO: am including time spent parsing in reported ALGOTIME
	uint64_t num_iterations = 0;
	while (incremental_importer.read()) {
		if (!partial_dir.empty()) {
			// generate debug trace before reoptimization
			std::string fname = partial_dir + "/debug-"
					                  + std::to_string(num_iterations) + ".min";
			std::ofstream partial_file(fname);
			DIMACSExporter<ResidualNetwork> partial_exporter(*g, partial_file);
			partial_exporter.write();
			partial_exporter.writeFlow();
		}

		is->reoptimize();
		exporter.writeFlow();
		std::cout << "c EOI" << std::endl;
		std::cout.flush();

		num_iterations++;
	}

	return 0;
}
