#ifndef BIN_LEMON_MIN_COST_H_
#define BIN_LEMON_MIN_COST_H_

#include <lemon/arg_parser.h>
#include <lemon/dimacs.h>

typedef lemon::SmartDigraph Digraph;
DIGRAPH_TYPEDEFS(Digraph);
typedef lemon::SmartGraph Graph;

class LemonMinCost;
template<class Value, class LargeValue>
class MinCostSolver;

class LemonMinCost {
	// arguments
	lemon::ArgParser ap;
	bool report;

	// state
	std::istream *is;
	std::ostream *os;
	lemon::DimacsDescriptor desc;
public:
	LemonMinCost(int argc, const char *argv[]);

	void openFiles();
	void run();

	friend class MinCostSolver<double, double>;
	friend class MinCostSolver<long double, long double>;
	friend class MinCostSolver<long long, long long>;
	friend class MinCostSolver<int, long long>;
	friend class MinCostSolver<int, long>;
};

template<class Value, class LargeValue>
class MinCostSolver {
	LemonMinCost &lmc;

	Value infty;
	Digraph g;
	Digraph::ArcMap<Value> lower, cap, cost;
	Digraph::NodeMap<Value> sup;
public:
	MinCostSolver(LemonMinCost &lmc);

	template<class MCF>
	void solveGeneric(std::function<typename MCF::ProblemType(MCF&)> &run);
	void solveNS();
	void run();
};

#endif /* BIN_LEMON_MIN_COST_H_ */
