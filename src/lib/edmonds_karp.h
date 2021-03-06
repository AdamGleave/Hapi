/*
 * EdmondsKarp.h
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#ifndef LIB_EDMONDS_KARP_H_
#define LIB_EDMONDS_KARP_H_

#include <queue>

#include "residual_network.h"

namespace flowsolver {

class EdmondsKarp {
public:
	explicit EdmondsKarp(ResidualNetwork &);
	// side-effect: updates graph
	virtual ~EdmondsKarp();

	void run();
private:
	std::queue<Arc *> predecessorPath(uint32_t);
	std::queue<Arc *> bfs();

	ResidualNetwork &g;
	uint32_t numNodes;
	std::vector<uint32_t> predecessors;
};

} /* namespace flowsolver */

#endif /* LIB_EDMONDS_KARP_H_ */
