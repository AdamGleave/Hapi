#include <iostream>
#include <fstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "RelaxIV_incremental.h"

#include "dimacs.h"

const static std::string TIMER_FORMAT = "ALGOTIME: %w\n";

using namespace flowsolver_bertsekas;

void compute_reserved_memory(MCFClass::Index &num_nodes,
		                         MCFClass::Index &num_arcs) {
	// TODO(adam): less crude method of reserving memory
	num_nodes *= 2;
	num_arcs *= 2;
}

template<class T>
inline T ABS( const T x )
{
 return( x >= T( 0 ) ? x : -x );
}

void writeFlow(MCFClass *mcf, bool potentials) {
	 if( ( numeric_limits<MCFClass::CNumber>::is_integer == 0 ) ||
      ( numeric_limits<MCFClass::FNumber>::is_integer == 0 ) ) {
		std::cout.setf( ios::scientific, ios::floatfield );
		std::cout.precision( 12 );
		}

	 // cost of solution
	 std::cout << "s " << mcf->MCFGetFO() << endl;

	 MCFClass::Index m = mcf->MCFm();
	 MCFClass::FRow x = new MCFClass::FNumber[m];
	 MCFClass::Index_Set active_arcs = new MCFClass::Index[m];
	 mcf->MCFGetX(x, active_arcs);
	 MCFClass::Index_Set start = new MCFClass::Index[m];
	 MCFClass::Index_Set end = new MCFClass::Index[m];
	 mcf->MCFArcs(start, end, active_arcs);
	 for(MCFClass::Index i = 0;
			 active_arcs[i] != MCFClass::Inf<MCFClass::Index>(); i++) {
		 std::cout << "f " << start[i] << " " << end[i] << " " << x[i] << endl;
	 }
	 delete[] x;

	 if (potentials) {
		 MCFClass::CRow pi = new MCFClass::CNumber[mcf->MCFn()];
		 mcf->MCFGetPi( pi );
		 for(MCFClass::Index i = 1; i <= mcf->MCFn() ; i++)
			std::cout << "p " << i << " " << pi[i-1] << endl;
		 delete[] pi;
	 }
}

bool process_result(RelaxIV *mcf, bool flow, bool potentials) {
	switch( mcf->MCFGetStatus() ) {
	 case( MCFClass::kOK ):
	{
	  if (flow) {
	  	writeFlow(mcf, potentials);
	  }

#ifdef DEBUG
		// check solution
		mcf->CheckPSol();
		mcf->CheckDSol();
#endif

		return true;
		break;
	}
	 case( MCFClass::kUnfeasible ):
	{
		std::cerr << "MCF problem unfeasible: "
				      << "error node = " << mcf->GetErrorNode()
							<< ", error info = " << mcf->GetErrorInfo()
							<< std::endl;
	  return false;
		break;
	}
	 case( MCFClass::kUnbounded ):
		std::cerr << "MCF problem unbounded." << endl;
	  return false;
		break;
	 default:
		std::cerr << "Error in the MCF solver." << endl;
		return false;
		break;
	 }
}

// timer
boost::timer::auto_cpu_timer t(std::cerr, TIMER_FORMAT);

// this looks stupid but is needed for a callback
void restart_timer() {
	t.start();
}

int main(int argc, char *argv[]) {
	// parse command line arguments
	namespace po = boost::program_options;

	po::options_description global("Global options");

	global.add_options()
		("help", "produce help message")
		("flow", po::value<bool>()->default_value(true), "Output flow solution.")
		("potentials", po::bool_switch()->default_value(false),
		 "Output dual solution. Must be used in conjunction with flow.")
		("quiet", po::bool_switch()->default_value(false),
		 "Suppress all but the highest priority logging messages.");

	po::variables_map vm;

	po::parsed_options parsed = po::command_line_parser(argc, argv).
				options(global).
				run();
	po::store(parsed, vm);

	if (vm.count("help")) {
		std::cout << global << std::endl;
		return 0;
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

	// initialize relevant classes
	RelaxIV *mcf = new RelaxIV();
	DIMACS dimacs(std::cin, mcf);

	// load initial network
	MCFClass::Index     tn, tm; // number of nodes & arcs
	MCFClass::FRow      tU; // arc upper capacities
	MCFClass::CRow      tC; // arc costs
	MCFClass::FRow      tDfct; // node deficits
	MCFClass::Index_Set tStartn, tEndn; // arc start & end nodes

	dimacs.ReadInitial(&tn, &tm, &tU, &tC, &tDfct, &tStartn, &tEndn);

	// reset timer
  t.stop(); t.start();
  // load network
	MCFClass::Index num_nodes_reserved = tn, num_arcs_reserved = tm;
	compute_reserved_memory(num_nodes_reserved, num_arcs_reserved);
	mcf->LoadNet(num_nodes_reserved, num_arcs_reserved, tn, tm,
			           tU, tC, tDfct, tStartn, tEndn);

	t.stop();

	delete[] tU;
	delete[] tDfct;
	delete[] tStartn;
	delete[] tEndn;
	delete[] tC;

  // solve network, output results, read delta, repeat
	t.resume();
	do {
#ifdef DEBUG
		std::cout << "c GRAPH" << endl;
		mcf->WriteMCF(std::cout, MCFClass::kDimacs);
#endif

		mcf->SolveMCF();
		t.stop(); t.report();

#ifdef DEBUG
		std::cout << "c END GRAPH" << endl;
#endif

		bool success = process_result(mcf, flow, potentials);
		if (!success) {
			return -1;
		}

		std::cout << "c EOI" << endl;
		std::cout.flush();
	} while (dimacs.ReadDelta(restart_timer));

	// Must stop timer. Otherwise it will stop automatically when it is destroyed
	// and report a spurious time.
	t.stop();

	return 0;
}
