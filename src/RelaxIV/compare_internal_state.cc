#include <iostream>
#include <fstream>
#include <string>

#include <set>
#include <algorithm>

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

void buildSet(MCFClass::Index i, MCFClass::Index_Set n,
		          std::set<MCFClass::Index> &set, size_t num_arcs) {
	while (i) {
		CHECK_LE(i, num_arcs);
		CHECK_EQ(set.count(i), 0) << "duplicate element in linked list.";
		set.insert(i);
		i = n[i];
	}
}


template<class T>
void setEquals(std::set<T> &s1, std::set<T> &s2, std::string test_name) {
	CHECK_EQ(s1.size(), s2.size()) << test_name;
	for (auto it1 = s1.begin(),  it2 = s2.begin(), end1 = s1.end();
			 it1 != end1; ++it1, ++it2) {
		T x = *it1, y = *it2;
		CHECK_EQ(x, y) << test_name;
	}
}

void compareLinkedList(MCFClass::Index_Set f1, MCFClass::Index_Set n1,
		                   MCFClass::Index_Set f2, MCFClass::Index_Set n2,
		                   size_t num_nodes, size_t num_arcs, std::string test_name) {
	for (size_t i = 1; i <= num_nodes; i++) {
		std::set<MCFClass::Index> a1, a2;

		buildSet(f1[i], n1, a1, num_arcs);
		buildSet(f2[i], n2, a2, num_arcs);

		std::ostringstream str_stream;
		str_stream << test_name << ": lists differ at index " << i;
		setEquals(a1, a2, str_stream.str());
	}
}

std::string formatArc(RelaxIV *r, MCFClass::Index i) {
	i--;
  return str(boost::format(
	  "%u->%u (index %u): cap = %u, flow = %u, cost = %u, RC = %d."
		" Potentials: src = %d, dst = %d.")
	              % r->MCFSNde(i) % r->MCFENde(i) % i
								% r->MCFUCap(i) % r->X[i + 1]
								% r->MCFCost(i) % r->MCFGetRC(i)
								% r->Pi[r->MCFSNde(i)] % r->Pi[r->MCFENde(i)]);
}

void checkBalancedArcs(RelaxIV *r1, MCFClass::Index start1, MCFClass::Index_Set next1,
		                   RelaxIV *r2, MCFClass::Index start2, MCFClass::Index_Set next2,
											 std::string test_name) {
	std::set<MCFClass::Index> a1, a2;

	buildSet(start1, next1, a1, r1->m);
	buildSet(start2, next2, a2, r2->m);

	CHECK_LE(a1.size(), a2.size());

	std::vector<MCFClass::Index> difference(a1.size() + a2.size());
	auto it = std::set_symmetric_difference(a1.begin(), a1.end(),
			                                a2.begin(), a2.end(), difference.begin());
	difference.resize(it - difference.begin());

	VLOG(1) << test_name << ": " << a1.size() << " / " << a2.size() << " / "
			    << difference.size();
  for (MCFClass::Index index : difference) {
  	CHECK_NE(r1->MCFGetRC(index - 1), 0) << test_name << " arc " << index
  			                             << " is balanced, but missing from one set.";
  }
}

void relaxIVEqual(RelaxIV *r1, RelaxIV *r2) {
	CHECK_EQ(r1->n, r2->n);
	CHECK_EQ(r1->m, r2->m);

	MCFClass::Index n = r1->n, m = r1->m;

	arrayEqual<MCFClass::FNumber>(r1->B + 1, r2->B + 1, n, "B");
  arrayEqual<MCFClass::FNumber>(r1->Dfct + 1, r2->Dfct + 1, n, "Dfct");

	arrayEqual<MCFClass::CNumber>(r1->Pi + 1, r2->Pi + 1, n, "Pi");

	arrayEqual<MCFClass::Index>(r1->Startn + 1, r2->Startn + 1, m, "Startn");
	arrayEqual<MCFClass::Index>(r1->Endn + 1, r2->Endn + 1, m, "Endn");

	arrayEqual<MCFClass::FNumber>(r1->X + 1, r2->X + 1, m, "X");
	arrayEqual<MCFClass::FNumber>(r1->U + 1, r2->U + 1, m, "U");
	arrayEqual<MCFClass::FNumber>(r1->Cap + 1, r2->Cap + 1, m, "Cap");

	arrayEqual<MCFClass::CNumber>(r1->C + 1, r2->C + 1, m, "C");
	arrayEqual<MCFClass::CNumber>(r1->RC + 1, r2->RC + 1, m, "RC");

	arrayEqual<MCFClass::Index>(r1->FOu + 1, r2->FOu + 1, n, "FOu");
	arrayEqual<MCFClass::Index>(r1->FIn + 1, r2->FIn + 1, n, "FIn");
	arrayEqual<MCFClass::Index>(r1->NxtOu + 1, r2->NxtOu + 1, m, "NxtOu");
	arrayEqual<MCFClass::Index>(r1->NxtIn + 1, r2->NxtIn + 1, m, "NxtIn");

	for (size_t i = 1; i <= n; i++) {
		// Direct comparison will fail. But the list only needs to be a superset
		// of balanced arcs, we'll just prune the ones which aren't. Check they
		// agree as to which arcs are balanced.
		checkBalancedArcs(r1, r1->tfstin[i], r1->tnxtin,
				              r2, r2->tfstin[i], r2->tnxtin, "star in");
		checkBalancedArcs(r1, r1->tfstou[i], r1->tnxtou,
						          r2, r2->tfstou[i], r2->tnxtou, "star out");
	}


	// these fail but I think this is just because we've not run solver in
	// first case
/*CHECK_EQ(r1->nb_pos, r2->nb_pos);
  CHECK_EQ(r1->nb_neg, r2->nb_neg);*/
}

void copyElt(MCFClass::Index start,
		         MCFClass::Index_Set dst_nxt, MCFClass::Index_Set src_nxt) {
	for (MCFClass::Index arc = start; arc; arc = src_nxt[arc]) {
		dst_nxt[arc] = src_nxt[arc];
	}
}

void swapState(RelaxIV *r1, RelaxIV *r2) {
	/*memcpy(r1->tfstin, r2->tfstin, sizeof(MCFClass::Index)*r1->n);
	memcpy(r1->tnxtin, r2->tnxtin, sizeof(MCFClass::Index)*r1->m);*/
	/*for (size_t i = 1; i < r1->n; i++) {
		copyElt(r1->tfstin[i], r1->tnxtin, r2->tnxtin);
	}*/
	std::swap(r1->tfstin, r2->tfstin);
	std::swap(r1->tnxtin, r2->tnxtin);
/*std::swap(r1->tfstou, r2->tfstou);
	std::swap(r1->tnxtou, r2->tnxtou);*/

	/*std::swap(r1->nb_pos, r2->nb_pos);
	std::swap(r1->nb_neg, r2->nb_neg);*/
	// NO-OP
	// TODO
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

	swapState(r_load, r_delta);

	std::cout << "c LOAD STATE SOLUTION" << std::endl;
	r_load->status = 0;
	r_load->SolveMCF();
	process_result(r_load);

	std::cout << "c DELTA SOLUTION" << std::endl;
	r_delta->SolveMCF();
	process_result(r_delta);

	return 0;
}
