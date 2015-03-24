#include "dimacs.h"

namespace flowsolver_bertsekas {

// TODO(adam): how to handle upper bound 0 arcs? may be worth representing
void DIMACS::ReadInitial(MCFClass::Index *out_tn, MCFClass::Index *out_tm,
	MCFClass::FRow *out_tU, MCFClass::CRow *out_tC, MCFClass::FRow *out_tDfct,
	MCFClass::Index_Set *out_tStartn, MCFClass::Index_Set *out_tEndn)
{
 // read first non-comment line - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 char c;
 for(;;) {
	if( ! ( is >> c ) )
		LOG(FATAL) << "LoadDMX: error reading the input stream";

	if( c != 'c' )  // if it's not a comment
	 break;

	do              // skip the entire line
	 if( ! is.get( c ) )
		LOG(FATAL) << "LoadDMX: error reading the input stream";
	while( c != '\n' );
	}

 if( c != 'p' )
	LOG(FATAL) << "LoadDMX: format error in the input stream";

 char buf[ 3 ];    // free space
 is >> buf;     // skip (in has to be "min")

 MCFClass::Index tn;
 if( ! ( is >> tn ) )
	LOG(FATAL) << "LoadDMX: error reading number of nodes";

 MCFClass::Index tm;
 if( ! ( is >> tm ) )
	LOG(FATAL) << "LoadDMX: error reading number of arcs";

 // allocate memory - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 arc_table.resize(tn + 1);

 MCFClass::FRow      tU      = new MCFClass::FNumber[ tm ];  // arc upper capacities
 MCFClass::FRow      tDfct   = new MCFClass::FNumber[ tn ];  // node deficits
 MCFClass::Index_Set tStartn = new MCFClass::Index[ tm ];    // arc start nodes
 MCFClass::Index_Set tEndn   = new MCFClass::Index[ tm ];    // arc end nodes
 MCFClass::CRow      tC      = new MCFClass::CNumber[ tm ];  // arc costs

 for( MCFClass::Index i = 0 ; i < tn ; )           // all deficits are 0
	tDfct[ i++ ] = 0;                      // unless otherwise stated

 // read problem data - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 MCFClass::Index i = 0;  // arc counter
 for(;;) {
	if( ! ( is >> c ) )
	 break;

	switch( c ) {
	 case( 'c' ):  // comment line- - - - - - - - - - - - - - - - - - - - - - -
	 {
		std::string line;
		if (!getline(is, line)) {
			LOG(FATAL) << "LoadDMX: error reading the input stream";
		}

		boost::trim(line);
		if (line == "EOI") {
			goto end;
		}
		break;
	 }

	 case( 'n' ):  // description of a node - - - - - - - - - - - - - - - - - -
		MCFClass::Index j;
		if( ! ( is >> j ) )
		 LOG(FATAL) << "LoadDMX: error reading node name";

		if( ( j < 1 ) || ( j > tn ) )
		 LOG(FATAL) << "LoadDMX: invalid node name";

		MCFClass::FNumber Dfctj;
		if( ! ( is >> Dfctj ) )
		 LOG(FATAL) << "LoadDMX: error reading deficit";

		tDfct[ j - 1 ] -= Dfctj;

		// skip over node type
		do {
			 if( ! is.get( c ) )
					 LOG(FATAL) << "LoadDMX: error reading the input stream";
		} while( c != '\n' );
		break;

	 case( 'a' ):  // description of an arc - - - - - - - - - - - - - - - - - -
		if( i == tm )
		 LOG(FATAL) << "LoadDMX: too many arc descriptors";

		if( ! ( is >> tStartn[ i ] ) )
		 LOG(FATAL) << "LoadDMX: error reading start node";

		if( ( tStartn[ i ] < 1 ) || ( tStartn[ i ] > tn ) )
		 LOG(FATAL) << "LoadDMX: invalid start node";

		if( ! ( is >> tEndn[ i ] ) )
		 LOG(FATAL) << "LoadDMX: error reading end node";

		if( ( tEndn[ i ] < 1 ) || ( tEndn[ i ] > tn ) )
		 LOG(FATAL) << "LoadDMX: invalid end node";

		if( tStartn[ i ] == tEndn[ i ] )
		 LOG(FATAL) << "LoadDMX: self-loops not permitted";

		MCFClass::FNumber LB;
		if( ! ( is >> LB ) )
		 LOG(FATAL) << "LoadDMX: error reading lower bound";

		if( ! ( is >> tU[ i ] ) )
		 LOG(FATAL) << "LoadDMX: error reading upper bound";

		if( ! ( is >> tC[ i ] ) )
		 LOG(FATAL) << "LoadDMX: error reading arc cost";

		if( tU[ i ] < LB )
		 LOG(FATAL) << "LoadDMX: lower bound > upper bound";

		tU[ i ] -= LB;
		tDfct[ tStartn[ i ] - 1 ] += LB;
		tDfct[ tEndn[ i ] - 1 ] -= LB;
		#if( USENAME0 )
		 tStartn[ i ]--;  // in the DIMACS format, node names start from 1
		 tEndn[ i ]--;
		#endif

		arc_table[tStartn[i]][tEndn[i]] = i;

		i++;
		break;

	 default:  // invalid code- - - - - - - - - - - - - - - - - - - - - - - - -
		LOG(FATAL) << "LoadDMX: invalid code " << c;

	 }  // end( switch( c ) )
	}  // end( for( ever ) )

 end:
 if( i < tm )
	LOG(FATAL) << "LoadDMX: too few arc descriptors";

 *out_tn = tn;
 *out_tm = tm;
 *out_tU = tU;
 *out_tC = tC;
 *out_tDfct = tDfct;
 *out_tStartn = tStartn;
 *out_tEndn = tEndn;
}

bool DIMACS::ReadDelta() {
	std::string line;

	while (getline(is, line)) {
		line_num++;

		std::istringstream iss (line);
		std::string first;
		iss >> first;
		CHECK_EQ(first.length(), 1)
			<< "type not single character at L" << line_num;
		char type = first[0];

		std::ostringstream oss;
		oss << iss.rdbuf();
		// WARNING: Do NOT 'simplify' this to oss.str().c_str()
		// If you do, oss.str() will be a temporary, and will be deallocated
		// immediately afterwards. This will lead to subtle memory access bugs.
		std::string oss_str = oss.str();
		const char *remainder = oss_str.c_str();

		bool more_data = processLine(type, remainder);
		if (!more_data) {
			return true;
		}
	}

	// EOF read
	return false;
}

void DIMACS::adjustSinkCapacity(int64_t delta) {
	MCFClass::FNumber sink_excess = mcf->B[SINK_NODE];
	mcf->ChgDfct(SINK_NODE, sink_excess + delta);
}

bool DIMACS::processLine(char type, const char *remainder) {
	int num_matches = -1;
	switch (type) {
	case 'c':
		{
		// is it end of graph indicator?
		std::string comment(remainder);
		// remove whitespace
		boost::trim(comment);

		if (comment == "EOI") {
			return false;
		}
		// otherwise can ignore comments
		break;
		}
	case 'r':
		{
		// remove node
		MCFClass::Index id;
		num_matches = sscanf(remainder, "%u", &id);
		CHECK_EQ(num_matches, 1) << "malformed remove node, at line "
														 << line_num;
		// TODO(adam): is this right? this is excess not supply, but I think
		// DelNode will adjust everything so it'll work out OK
		adjustSinkCapacity(mcf->B[id]);
		mcf->DelNode(id);
		break;
		}
	case 'n':
		{
		// add a node
		MCFClass::Index id;
		MCFClass::FNumber supply;
		// node type also present as third column, but we ignore this
		num_matches = sscanf(remainder, "%u %ld", &id, &supply);
		CHECK_EQ(num_matches, 2);

		mcf->AddNode(id, supply);

		// TODO: This is a bit of a hack - should I keep this?
		// Firmament doesn't export changes in sink demand. To keep the problem
		// balanced, increase demand at the sink whenever we add a new node.
		CHECK_GE(supply, 0) << "only one node allowed to be a sink.";
		// increase demand at sink by parameter supply
		adjustSinkCapacity(-supply);
		}
		break;
	case 'x':
		{
		// change of an existing arc;
		// could be either cost or capacity, or both
		MCFClass::Index src, dst;
		MCFClass::FNumber lower_bound, upper_bound;
		MCFClass::CNumber cost;
		num_matches = sscanf(remainder, "%u %u %ld %ld %ld",
						 &src, &dst, &lower_bound, &upper_bound, &cost);
		CHECK_EQ(num_matches, 5);

		CHECK_EQ(lower_bound, 0);
		CHECK_GE(upper_bound, 0);
		std::unordered_map<uint32_t, MCFClass::Index> &adjacencies = arc_table[src];
		auto it = adjacencies.find(dst);
		if (it == adjacencies.end()) {
			// arc is new
			// SOMEDAY: Decide whether Firmament should change or not.
			// Firmament considers arcs with a zero upper bound to still be arcs.
			// We do not. So we have to special-case this.
			if (upper_bound == 0) {
				// Firmament sometimes generates arc changes when nothing has changed
				// This is the case "the arc had zero capacity before, and
				// "still doesn't". That is, it wasn't an arc before, and still isn't.
				LOG(WARNING) << "ignoring delete of non-existent arc "
										 << src << "->" << dst;
			} else {
				LOG(WARNING) << "converting change of non-existent arc "
										 << src << "->" << dst << " to an add";
				CHECK_EQ(lower_bound, 0);

				MCFClass::Index index = mcf->AddArc(src, dst, upper_bound, cost);
				CHECK(index != MCFClass::Inf<MCFClass::Index>()) << "out of arc memory.";

				arc_table[src][dst] = index;
			}
		} else {
			// arc already exists
			MCFClass::Index index = it->second;

			if (upper_bound == 0) {
				mcf->DelArc(index);
			} else {
				bool done_something = false;
				MCFClass::FNumber current_upper_bound = mcf->U[index];
				if (current_upper_bound != upper_bound) {
					mcf->ChgUCap(index, upper_bound);
					done_something = true;
				}
				MCFClass::CNumber current_cost = mcf->C[index];
				if (current_cost != cost) {
					mcf->ChgCost(index, cost);
					done_something = true;
				}
				LOG_IF(WARNING, !done_something) << "no-op arc change event "
																				 << src << "->" << dst;
			}
		}
		break;
		}
	case 'a':
		{
		// add new arc
		MCFClass::Index src, dst;
		MCFClass::FNumber lower_bound, upper_bound;
		MCFClass::CNumber cost;
		num_matches = sscanf(remainder, "%u %u %ld %ld %ld",
						 &src, &dst, &lower_bound, &upper_bound, &cost);
		CHECK_EQ(num_matches, 5);

		CHECK_EQ(lower_bound, 0);
		CHECK_GE(upper_bound, 0);

		if (upper_bound == 0) {
			LOG(WARNING) << "ignoring add of arc " << src << "->"
									 << dst << " with zero upper-bound.";
		} else {
			if (arc_table[src].count(dst) == 0) {
				MCFClass::Index index = mcf->AddArc(src, dst, upper_bound, cost);
				CHECK(index != MCFClass::Inf<MCFClass::Index>()) << "out of arc memory.";

				arc_table[src][dst] = index;
			} else {
				LOG(WARNING) << "ignoring add of arc " << src << "->" << dst
										 << " which already exists.";
			}
		}
		break;
		}
	}

	return true;
}

}
