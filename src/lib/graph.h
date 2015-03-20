#ifndef LIB_GRAPH_H_
#define LIB_GRAPH_H_

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>

#include "arc.h"

// we don't use any variables: the code is never run
// assignments are made just to force type checking
#pragma GCC diagnostic ignored "-Wunused-variable"

namespace flowsolver {

template<class X>
struct Graph {
public:
	typedef typename X::iterator iterator;
	typedef typename X::const_iterator const_iterator;

	BOOST_CONCEPT_ASSERT((boost::Mutable_ForwardIterator<iterator>));
	BOOST_CONCEPT_ASSERT((boost::ForwardIterator<const_iterator>));

	BOOST_CONCEPT_USAGE(Graph) {
		// needed for DIMACS import
		X graph(10);
		graph.addArc(1, 2, 42, 5);
		flowsolver::Arc &arc = *graph.getArc(1,2);
		graph.setSupply(1, 20);
		int64_t supply = graph.getBalance(1);

		// needed for DIMACS export
		Graph::iterator it = graph.begin();
		it = graph.end();
		Graph::const_iterator const_it = it; // check types convertible
		const flowsolver::Arc &const_arc = *const_it;
		arc = *it;
	}
};

template<class X>
struct DynamicGraphCallbacks {
public:
	X &graph;

	// we do this so as to not require any ability to construct type X:
	// we just need to be able to manipulate an instance we are given
	DynamicGraphCallbacks(X &graph) : graph(graph) {};

	BOOST_CONCEPT_USAGE(DynamicGraphCallbacks) {
		Arc *arc = graph.getArc(1, 2);
		graph.addNode(5);
		graph.addArc(1, 5, 1, 1);
		graph.changeArcCapacity(1, 5, 3);
		graph.changeArcCost(1, 5, 2);
		graph.removeArc(1, 5);
		graph.setSupply(1, 5);
		graph.removeNode(1);
	}
};

template<class X>
struct DynamicGraph {
	BOOST_CONCEPT_ASSERT((Graph<X>));
	BOOST_CONCEPT_ASSERT((DynamicGraphCallbacks<X>));
};

}

#endif /* LIB_GRAPH_H_ */
