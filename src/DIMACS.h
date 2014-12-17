/*
 * DIMACS.h
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#ifndef SRC_DIMACS_H_
#define SRC_DIMACS_H_

#include <iostream>

#include "FlowNetwork.h"

namespace flowsolver {

class DIMACS {
	DIMACS() = delete;
public:
	static FlowNetwork &readDIMACSMin(std::istream &);
	static void writeDIMACSMin(const FlowNetwork &, std::ostream &);
};

} /* namespace flowsolver */

#endif /* SRC_DIMACS_H_ */
