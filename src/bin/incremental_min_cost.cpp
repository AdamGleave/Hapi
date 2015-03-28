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
	// parse command line arguments
	namespace po = boost::program_options;

	po::options_description global("Global options");
	global.add_options()
			("help", "produce help message")
			("debug-partial-dir", po::value<std::string>(),
			 "debug option: enable generation of partial traces,"
			 " that is the state of the flow network before reoptimization but after"
			 " the incremental changes, maintaining optimality (but not feasibility)")
			("flow", po::value<bool>()->default_value(true), "Output flow solution.")
			("potentials", po::bool_switch()->default_value(false),
			 "Output dual solution. Must be used in conjunction with flow.")
			("quiet", po::bool_switch()->default_value(false),
			 "Suppress all but the highest priority logging messages.")
			("command", po::value<std::string>(),
			 "command to execute: augmenting_path or relax.");

	po::positional_options_description pos;
	pos.add("command", 1);

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

	bool flow = vm["flow"].as<bool>();
	bool potentials = vm["potentials"].as<bool>();
	bool quiet = vm["quiet"].as<bool>();

	// initialise logging

	FLAGS_logtostderr = true;
	if (quiet) {
		FLAGS_minloglevel = google::ERROR;
	}
	google::InitGoogleLogging(argv[0]);

	// for timing algorithms
	boost::timer::auto_cpu_timer t(std::cerr, TIMER_FORMAT);
	t.stop();

	// load full file
	ResidualNetwork *g = DIMACSIncrementalFullImporter<ResidualNetwork>
                                                              (std::cin).read();

	// solve full problem
	IncrementalSolver *is = NULL;
	t.start();
	if (cmd == "augmenting_path") {
		is = new AugmentingPath(*g);
	} else if (cmd == "relax") {
		is = new RELAX(*g);
	} else {
		// should have been caught during command-line parsing
		assert(false);
	}
	is->run();
	t.stop(); t.report();

	DIMACSPotentialExporter<ResidualNetwork> exporter(*g, *is, std::cout);
	if (flow) {
		exporter.writeFlow();
		if (potentials) {
			exporter.writePotentials();
		}
	}
	std::cout << "c EOI" << std::endl;
	std::cout.flush();

	// now solve incremental problem
	DynamicMaintainOptimality dynamic(*g, *is);
	DIMACSIncrementalDeltaImporter<DynamicMaintainOptimality>
	                              incremental_importer(std::cin, dynamic);

	// process stream of incremental deltas, solving incremental problem
	uint64_t num_iterations = 0;
	while (incremental_importer.read()) {
		if (!partial_dir.empty()) {
			// generate debug trace before reoptimization
			std::string fname = partial_dir + "/debug-"
					                  + std::to_string(num_iterations) + ".min";
			std::ofstream partial_file(fname);
			DIMACSPotentialExporter<ResidualNetwork> partial_exporter(*g, *is,
					                                                      partial_file);
			partial_exporter.write();
			partial_exporter.writeFlow();
			partial_exporter.writePotentials();
		}
		t.start();
		is->reoptimize();
		t.stop();
		t.report();
		if (flow) {
			exporter.writeFlow();
			if (potentials) {
				exporter.writePotentials();
			}
		}
		std::cout << "c EOI" << std::endl;
		std::cout.flush();
		t.start();

		num_iterations++;
	}
	t.stop();

	return 0;
}
