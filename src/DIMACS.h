/*
 * DIMACS.h
 *
 *  Created on: 16 Dec 2014
 *      Author: adam
 */

#ifndef SRC_DIMACS_H_
#define SRC_DIMACS_H_

#include <iostream>

#include "ResidualNetwork.h"

namespace flowsolver {

class DIMACS {
public:
	ResidualNetwork &readDIMACSMin(std::istream &);
	void writeDIMACSMin(ResidualNetwork &, std::ostream &);
};

} /* namespace flowsolver */

#endif /* SRC_DIMACS_H_ */
