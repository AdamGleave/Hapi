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
  template<bool is_const_iterator = true>
  class const_noconst_iterator;


  typedef const_noconst_iterator<false> iterator;
  typedef const_noconst_iterator<true> const_iterator;

	void init();

	iterator beginZeroCostCut();
	iterator endZeroCostCut();
	const_iterator beginZeroCostCut() const;
	const_iterator endZeroCostCut() const;
	iterator beginPositiveCostCut();
	iterator endPositiveCostCut();
	const_iterator beginPositiveCostCut() const;
	const_iterator endPositiveCostCut() const;

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

template<bool is_const_iterator>
 class RELAX::const_noconst_iterator : public std::iterator
     <std::forward_iterator_tag,
      typename std::conditional<is_const_iterator, const Arc, Arc>::type>
 {
 private:
   typedef typename std::vector<std::unordered_map<uint32_t, Arc*>> AT;
   typedef typename std::conditional<is_const_iterator, const AT, AT>::type
           *AdjacencyType;

   typedef typename std::conditional<is_const_iterator,
           std::unordered_map<uint32_t, Arc*>::const_iterator,
             std::unordered_map<uint32_t, Arc*>::iterator>::type
           MapIterator;

   AdjacencyType arcs;
   std::set<uint32_t>::const_iterator tree_it;
   MapIterator map_it, map_it_end;
   const std::set<uint32_t>* tree_nodes;

 public:
   typedef typename std::conditional<is_const_iterator, const Arc, Arc>::type value_type;

   const_noconst_iterator() : arcs(0) { }

   const_noconst_iterator(AdjacencyType arcs,
                          const std::set<uint32_t>* tree_nodes)
                          : arcs(arcs), tree_nodes(tree_nodes) {
     tree_it = tree_nodes->begin();
     uint32_t tree_node = *tree_it;
     map_it = (*arcs)[tree_node].begin();
     map_it_end = (*arcs)[tree_node].end();

     if (tree_it != tree_nodes->end()) {
       // if set completely empty, then do nothing
       // otherwise, skip until we find a valid element, if not already on one
       nextValid();
     }
   }

   const_noconst_iterator(const std::set<uint32_t>* tree_nodes, bool)
                                                      : tree_nodes(tree_nodes) {
     // for end sentinel
     tree_it = tree_nodes->end();
   }

   // Copy constructor. Implicit conversion from regular iterator to
   // const_iterator.
   const_noconst_iterator(const const_noconst_iterator<false>& other) :
     arcs(other.arcs), tree_it(other.tree_it),
     map_it(other.map_it), map_it_end(other.map_it_end),
     tree_nodes(other.tree_nodes) {}

   value_type &operator*() const {
     return *(map_it->second);
   }

   // keep iterating until we reach a valid element
   void nextValid() {
     while (map_it == map_it_end) {
       // end of map, go on to next value in vector
       // (may need to do this repeatedly if maps are empty)
       ++tree_it;
       if (tree_it == tree_nodes->end()) {
         // no elements left to iterate over: end
         break;
       }
       uint32_t tree_node = *tree_it;
       map_it = (*arcs)[tree_node].begin();
       map_it_end = (*arcs)[tree_node].end();
     }
   }

   // prefix operator
   const_noconst_iterator<is_const_iterator>& operator++() {
     map_it++;
     nextValid();
     return *this;
   }

   const_noconst_iterator<is_const_iterator> operator++(int) {
     // postfix operator
     const_noconst_iterator<is_const_iterator> old(*this);
     ++(*this);
     return old;
   }

   bool operator==(const const_noconst_iterator<is_const_iterator>& it)
       const {
     if (tree_it == it.tree_it) {
       if (tree_it == tree_nodes->end()) {
         // End iterator not unique.
         // The end sentinel has map_it set to default value.
         // By contrast, the end iterator reached by successive
         // applications of the ++ operator has map_it set to the
         // last legal value.
         return true;
       } else {
         return map_it == it.map_it;
       }
     } else {
       return false;
     }
   }

   bool operator!=(const const_noconst_iterator<is_const_iterator>& it)
       const {
     return !(*this == it);
   }

   friend class const_noconst_iterator<true>;
 };

} /* namespace flowsolver */

#endif /* LIB_RELAX_H_ */
