#include <iostream>
#include <fstream>
#include <string>

#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "RelaxIV_incremental.h"

#include "dimacs.h"

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

void set_parameters(MCFClass *mcf) {
	// TODO(adam): is this necessary? copied from main.cc
  // set "reasonable" values for the epsilons, if any - - - - - - - - - - - -

  MCFClass::FNumber eF = 1;
  for( register MCFClass::Index i = mcf->MCFm() ; i-- ; )
   eF = max( eF , ABS( mcf->MCFUCap( i ) ) );

  for( register MCFClass::Index i = mcf->MCFn() ; i-- ; )
   eF = max( eF , ABS( mcf->MCFDfct( i ) ) );

  MCFClass::CNumber eC = 1;
  for( register MCFClass::Index i = mcf->MCFm() ; i-- ; )
   eC = max( eC , ABS( mcf->MCFCost( i ) ) );

  mcf->SetPar( RelaxIV::kEpsFlw, (double) numeric_limits<MCFClass::FNumber>::epsilon() * eF *
		  mcf->MCFm() * 10);  // the epsilon for flows

  mcf->SetPar( RelaxIV::kEpsCst, (double) numeric_limits<MCFClass::CNumber>::epsilon() * eC *
		  mcf->MCFm() * 10);  // the epsilon for costs
}

void writeFlow(MCFClass *mcf) {
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
	 MCFClass::CRow pi = new MCFClass::CNumber[mcf->MCFn()];
	 ((RelaxIV *)mcf)->cmptprices();
	 mcf->MCFGetPi( pi );
	 for(MCFClass::Index i = 1; i <= mcf->MCFn() ; i++)
		std::cout << "p " << i << " " << pi[i-1] << endl;
	 delete[] pi;
}

bool process_result(MCFClass *mcf) {
	// TODO(adam): should measure wall time for fair comparison
	switch( mcf->MCFGetStatus() ) {
	 case( MCFClass::kOK ):
	{
		double tu, ts;
		mcf->TimeMCF( tu , ts );
		std::cerr << "Solution time (s): user " << tu << ", system " << ts << endl;
		// output overall time for benchmark suite
		std::cerr << "ALGOTIME: " << mcf->TimeMCF() << endl;

		writeFlow(mcf);

		// check solution
		mcf->CheckPSol();
		mcf->CheckDSol();

		return true;
		break;
	}
	 case( MCFClass::kUnfeasible ):
	{
		RelaxIV *relax = dynamic_cast<RelaxIV *>(mcf);
		std::cerr << "MCF problem unfeasible: error node = "
				      << relax->error_node << ", error info = " << relax->error_info
							<< endl;
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

int main(int, char *argv[]) {
	FLAGS_logtostderr = true;
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

	// load network
	MCFClass::Index num_nodes_reserved = tn, num_arcs_reserved = tm;
	compute_reserved_memory(num_nodes_reserved, num_arcs_reserved);
	mcf->LoadNet(num_nodes_reserved, num_arcs_reserved, tn, tm,
			           tU, tC, tDfct, tStartn, tEndn);

	delete[] tU;
	delete[] tDfct;
	delete[] tStartn;
	delete[] tEndn;
	delete[] tC;

	// set sensible values for epsilons
	set_parameters(mcf);

  // solve network, output results, read delta, repeat
	bool first_time = true;
	do {
#ifdef DEBUG
		std::cout << "c GRAPH" << endl;
		mcf->WriteMCF(std::cout, MCFClass::kDimacs);
#endif

		//mcf->SetMCFTime();  // reset timer
		if (!first_time) {
			mcf->CheckDSol(false);
			writeFlow(mcf);
		}
		first_time = false;
		//mcf->status = MCFClass::kUnSolved;
		mcf->SolveMCF();

#ifdef DEBUG
		std::cout << "c END GRAPH" << endl;
#endif

		bool success = process_result(mcf);
		if (!success) {
			return -1;
		}

		std::cout << "c EOI" << endl;
		std::cout.flush();
	} while (dimacs.ReadDelta());

	return 0;
}
