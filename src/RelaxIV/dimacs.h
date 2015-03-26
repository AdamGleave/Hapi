#ifndef LIB_DIMACS_H_
#define LIB_DIMACS_H_

#include <cstring>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>

#include "RelaxIV_incremental.h"

namespace flowsolver_bertsekas {

// TODO(adam): how to handle upper bound 0 arcs? may be worth representing
class DIMACS {
public:
	DIMACS(istream &is, RelaxIV *mcf) : is(is), mcf(mcf) {};
	~DIMACS() {};

	void ReadInitial(MCFClass::Index *out_tn, MCFClass::Index *out_tm,
    MCFClass::FRow *out_tU, MCFClass::CRow *out_tC, MCFClass::FRow *out_tDfct,
	  MCFClass::Index_Set *out_tStartn, MCFClass::Index_Set *out_tEndn);

	RelaxIV::RIVState *ReadState(istream &state,
			                         MCFClass::Index n, MCFClass::Index m);

	// returns true if read something, false otherwise
	bool ReadDelta();
private:
	const static MCFClass::Index SINK_NODE = 1;

	void changeSinkDeficit(int64_t delta);
	bool processLine(char type, const char *remainder);

	std::istream &is;
	unsigned int line_num = 0;

	std::vector<std::unordered_map<uint32_t, MCFClass::Index>> arc_table;
	RelaxIV *mcf;
};

}

#endif /* LIB_DIMACS_H_ */
