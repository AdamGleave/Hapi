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
	DIMACS() = delete;
public:
	static ResidualNetwork &readDIMACSMin(std::istream &);
	static void writeDIMACSMin(const ResidualNetwork &, std::ostream &);
	static void writeDIMACSMinFlow(const ResidualNetwork &, std::ostream &);
};

} /* namespace flowsolver */

#endif /* SRC_DIMACS_H_ */
