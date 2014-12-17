/*
 * EdmondsKarp.h
 *
 *  Created on: 17 Dec 2014
 *      Author: adam
 */

#ifndef EDMONDSKARP_H_
#define EDMONDSKARP_H_

#include "ResidualNetwork.h"

namespace flowsolver {

class EdmondsKarp {
	ResidualNetwork &g;
	uint32_t numNodes;
	std::vector<uint32_t> predecessors;
	// TODO: Would source & sink be better off being maintained by ResidualNetwork?
	std::set<uint32_t> source;
	std::set<uint32_t> sink;

	uint64_t augmenting_flow(std::queue<Arc *>);
	std::queue<Arc *> predecessor_path(uint32_t);
	std::queue<Arc *> bfs();
public:
	EdmondsKarp(ResidualNetwork &);
	// side-effect: updates graph
	void run();
	virtual ~EdmondsKarp();
};

} /* namespace flowsolver */

#endif /* EDMONDSKARP_H_ */
