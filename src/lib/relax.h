#ifndef LIB_RELAX_H_
#define LIB_RELAX_H_

#include <climits>
#include <vector>
#include <queue>

#include "incremental_solver.h"
#include "residual_network.h"

namespace flowsolver {

class RELAX : public IncrementalSolver {
public:
	explicit RELAX(ResidualNetwork &g);
	virtual ~RELAX();

	virtual void run() override;
	virtual void reoptimize() override;
protected:
	virtual std::vector<uint64_t> &getPotentials() override {
		return potentials;
	}
private:
	void init();

	ResidualNetwork::iterator beginZeroCostCut();
	ResidualNetwork::iterator endZeroCostCut();
	ResidualNetwork::const_iterator beginZeroCostCut() const;
	ResidualNetwork::const_iterator endZeroCostCut() const;
	ResidualNetwork::iterator beginPositiveCostCut();
	ResidualNetwork::iterator endPositiveCostCut();
	ResidualNetwork::const_iterator beginPositiveCostCut() const;
	ResidualNetwork::const_iterator endPositiveCostCut() const;

	void reset_cut();
	void update_cut(uint32_t new_node);

	int64_t compute_reduced_cost(const Arc &arc, bool allow_negative=false);
	uint64_t compute_residual_cut();

	void adjust_potential();
	void adjust_flow(uint32_t src_id, uint32_t dst_id);

	ResidualNetwork &g;
	std::vector<uint64_t> potentials;
	std::vector<uint32_t> predecessors;

	std::set<uint32_t> tree_nodes;
	uint64_t tree_excess;
	uint64_t tree_residual_cut;
	std::vector<std::unordered_map<uint32_t, Arc*>>
	                         tree_cut_arcs_zero_cost, tree_cut_arcs_positive_cost;
};

} /* namespace flowsolver */

#endif /* LIB_RELAX_H_ */
