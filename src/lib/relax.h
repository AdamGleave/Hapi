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

	void run();
	void reoptimize();
protected:
	virtual std::vector<uint64_t> &getPotentials() override {
		return potentials;
	}
private:
	class cut_arcs_iterator
		  : public std::iterator<std::forward_iterator_tag, std::uint32_t>  {
	public:
		cut_arcs_iterator(RELAX &algo);
		cut_arcs_iterator(RELAX &algo, bool);
		Arc* operator*() const;
		cut_arcs_iterator operator++();
		cut_arcs_iterator operator++(int);
		bool operator==(const cut_arcs_iterator &it);
		bool operator!=(const cut_arcs_iterator &it);

	private:
		void updateArcsIt();
		void nextValid();

		RELAX &algo;
		std::set<uint32_t>::const_iterator node_it;
		uint32_t cur_node;
		std::unordered_map<uint32_t, Arc*>::const_iterator arcs_it, arcs_it_end;
	};

	void init();

	int64_t compute_reduced_cost(Arc *arc, bool allow_negative=false);
	uint64_t compute_residual_cut();
	void update_residual_cut(uint32_t new_node);
	cut_arcs_iterator beginCutArcs();
	cut_arcs_iterator endCutArcs();

	void adjust_potential();
	void adjust_flow(uint32_t src_id, uint32_t dst_id);

	// returns true if feasible solution exists, and updates g accordingly;
	// false if no feasible solution, g left in undefined state
	std::queue<Arc *> predecessorPath(uint32_t source, uint32_t sink,
																					const std::vector<uint32_t>& parents);

	ResidualNetwork &g;
	std::vector<uint64_t> potentials;
	std::vector<uint32_t> predecessors;

	std::set<uint32_t> tree_nodes;
	uint64_t tree_excess;
	uint64_t tree_residual_cut;
};

} /* namespace flowsolver */

#endif /* LIB_RELAX_H_ */
