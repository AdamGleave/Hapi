#include <iostream>
#include <fstream>
#include <string>

#include <boost/format.hpp>
#include <glog/logging.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// DANGER -- this is ugly, only used in debugging
#define private public
#define protected public
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

RelaxIV *loadState(std::ifstream &graph, std::ifstream &state_file) {
	// initialize relevant classes
	RelaxIV *mcf = new RelaxIV();
	DIMACS dimacs(graph, mcf);

	// load initial network
	MCFClass::Index     tn, tm; // number of nodes & arcs
	MCFClass::FRow      tU; // arc upper capacities
	MCFClass::CRow      tC; // arc costs
	MCFClass::FRow      tDfct; // node deficits
	MCFClass::Index_Set tStartn, tEndn; // arc start & end nodes

	dimacs.ReadInitial(&tn, &tm, &tU, &tC, &tDfct, &tStartn, &tEndn);

	// load network
	MCFClass::Index num_nodes_reserved = tn, num_arcs_reserved = tm;
	//compute_reserved_memory(num_nodes_reserved, num_arcs_reserved);
	mcf->LoadNet(num_nodes_reserved, num_arcs_reserved, tn, tm,
			           tU, tC, tDfct, tStartn, tEndn);

	std::cout << "c IMPORTED GRAPH" << std::endl;
	mcf->WriteMCF(std::cout, MCFClass::kDimacs);

	delete[] tU;
	delete[] tDfct;
	delete[] tStartn;
	delete[] tEndn;
	delete[] tC;

	// load state
	RelaxIV::RIVState *state = dimacs.ReadState(state_file, tn, tm);
	mcf->MCFPutState(state);
	delete state;

	std::cout << "c IMPORTED FLOW & POTENTIALS" << std::endl;
	writeFlow(mcf);

	/*// set sensible values for epsilons
	set_parameters(mcf);

	// force it to reoptimize
	mcf->status = 0;
	mcf->SolveMCF();

	std::cout << "c SOLUTION" << std::endl;
	writeFlow(mcf);

	std::cout << "c EOI" << endl;
	std::cout.flush();*/

	return mcf;
}

RelaxIV *applyDelta(std::ifstream &graph_deltas) {
	// initialize relevant classes
	RelaxIV *mcf = new RelaxIV();
	DIMACS dimacs(graph_deltas, mcf);

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

	std::cout << "c IMPORTED GRAPH" << std::endl;
	mcf->WriteMCF(std::cout, MCFClass::kDimacs);

	// set sensible values for epsilons
	set_parameters(mcf);

	// solve initial network
	mcf->SolveMCF();

	std::cout << "c INITIAL SOLUTION" << std::endl;
	writeFlow(mcf);

	// load delta
	bool success = dimacs.ReadDelta();
	CHECK(success);

	std::cout << "c GRAPH AFTER DELTAS" << std::endl;
	mcf->WriteMCF(std::cout, MCFClass::kDimacs);
	writeFlow(mcf);

  return mcf;
}

template<class T>
void arrayEqual(T *a, T *b, size_t n, std::string test_name) {
	for (size_t i = 0; i < n; i++) {
		if (a[i] != b[i]) {
			CHECK_EQ(a[i], b[i]) << test_name << ": elements differ at index " << i;
		}
	}
}

std::string formatArc(RelaxIV *r, MCFClass::Index i) {
  return str(boost::format(
	  "%u->%u (index %u): cap = %u, flow = %u, cost = %u, RC = %d."
		" Potentials: src = %d, dst = %d.")
	              % r->MCFSNde(i) % r->MCFENde(i) % i
								% r->MCFUCap(i) % r->X[i + 1]
								% r->MCFCost(i) % r->MCFGetRC(i)
								% r->Pi[r->MCFSNde(i)] % r->Pi[r->MCFENde(i)]);
}

void relaxIVEqual(RelaxIV *r1, RelaxIV *r2) {
	CHECK_EQ(r1->n, r2->n);
	CHECK_EQ(r1->m, r2->m);

	MCFClass::Index n = r1->n, m = r1->m;

	arrayEqual<MCFClass::FNumber>(r1->X + 1, r2->X + 1, m, "X");
	arrayEqual<MCFClass::FNumber>(r1->U + 1, r2->U + 1, m, "U");
	arrayEqual<MCFClass::FNumber>(r1->Cap + 1, r2->Cap + 1, m, "Cap");

	arrayEqual<MCFClass::CNumber>(r1->C + 1, r2->C + 1, m, "C");
	arrayEqual<MCFClass::CNumber>(r1->RC + 1, r2->RC + 1, m, "RC");

	arrayEqual<MCFClass::FNumber>(r1->B + 1, r2->B + 1, n, "B");
	arrayEqual<MCFClass::FNumber>(r1->Dfct + 1, r2->Dfct + 1, n, "Dfct");

  arrayEqual<MCFClass::Index>(r1->tfstin + 1, r2->tfstin + 1, n, "tfstin");
  arrayEqual<MCFClass::Index>(r1->tfstou + 1, r2->tfstou + 1, n, "tfstou");
  arrayEqual<MCFClass::Index>(r1->tnxtin + 1, r2->tnxtin + 1, m, "tnxtin");
  arrayEqual<MCFClass::Index>(r1->tnxtou + 1, r2->tnxtou + 1, m, "tnxtou");

  CHECK_EQ(r1->nb_pos, r2->nb_pos);
  CHECK_EQ(r1->nb_neg, r2->nb_neg);

	arrayEqual<MCFClass::Index>(r1->Startn + 1, r2->Startn + 1, m, "Startn");
	arrayEqual<MCFClass::Index>(r1->Endn + 1, r2->Endn + 1, m, "Endn");

	arrayEqual<MCFClass::Index>(r1->FOu + 1, r2->FOu + 1, n, "FOu");
	arrayEqual<MCFClass::Index>(r1->FIn + 1, r2->FIn + 1, n, "FIn");
	arrayEqual<MCFClass::Index>(r1->NxtOu + 1, r2->NxtOu + 1, m, "NxtOu");
	arrayEqual<MCFClass::Index>(r1->NxtIn + 1, r2->NxtIn + 1, m, "NxtIn");

	arrayEqual<MCFClass::CNumber>(r1->Pi + 1, r2->Pi + 1, m, "Pi");
}

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	if (argc != 4) {
		std::cerr << "usage: <graph> <state> <graph_with_delta>. "
				      << "Reads graph from STDIN." << std::endl;
		return -1;
	}

	std::string graph_fname(argv[1]);
	std::ifstream graph_file(graph_fname);
	if (!graph_file.is_open()) {
		LOG(FATAL) << "Could not open " << graph_fname << " for reading.";
	}

	std::string state_fname(argv[2]);
	std::ifstream state_file(state_fname);
	if (!state_file.is_open()) {
		LOG(FATAL) << "Could not open " << state_fname << " for reading.";
	}

	std::string graph_delta_fname(argv[3]);
	std::ifstream graph_delta_file(graph_delta_fname);
	if (!graph_delta_file.is_open()) {
		LOG(FATAL) << "Could not open " << graph_delta_fname << " for reading.";
	}

	RelaxIV *r_load = loadState(graph_file, state_file);
	RelaxIV *r_delta = applyDelta(graph_delta_file);
	relaxIVEqual(r_load, r_delta);

	return 0;
}
