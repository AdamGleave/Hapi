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
public:
	FlowNetwork &readDIMACSMin(std::istream &);
	void writeDIMACSMin(FlowNetwork &, std::ostream &);
};

} /* namespace flowsolver */

#endif /* SRC_DIMACS_H_ */
