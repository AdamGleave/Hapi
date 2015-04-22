#include <deque>
#include <glog/logging.h>

#include "arc.h"
#include "augmenting_path.h"
#include "residual_network_util.h"

namespace flowsolver {

// min heap
class BinaryHeap {
	const std::vector<uint64_t> &data;
	std::vector<uint32_t> keys, reverse;
	uint32_t size;

	uint32_t left(uint32_t index) {
		return 2*index + 1;
	}

	uint32_t right(uint32_t index) {
		return 2*index + 2;
	}

	uint32_t parent(uint32_t index) {
		return (index - 1) / 2;
	}

	void assign(uint32_t index, uint32_t value) {
		keys[index] = value;
		reverse[value] = index;
	}

	void swap(uint32_t index1, uint32_t index2) {
		uint32_t tmp = keys[index1];
		assign(index1, keys[index2]);
		assign(index2, tmp);
	}

	void heapify(uint32_t index) {
		VLOG(4) << "heapify at " << index;
		uint32_t l = left(index), r = right(index);
		uint32_t smallest = index;
		if (l < size && data[keys[l]] < data[keys[smallest]]) {
			smallest = l;
		}
		if (r < size && data[keys[r]] < data[keys[smallest]]) {
			smallest = r;
		}
		if (smallest != index) {
			VLOG(4) << "heapify swapping " << smallest << " with " << index;
			// we need to swap elements in order to maintain heap structure
			swap(index, smallest);
			heapify(smallest);
		}
	}
public:
	// PRECONDITION: data[start_id] == 0, data[i] == UINT64_MAX for all other i
	BinaryHeap(std::vector<uint64_t> &data) : data(data), size(0) { }

	void makeHeap(uint32_t start) {
		reverse.resize(data.size());
		size = 1;
		keys.resize(size);

		// first key in min-heap is start
		assign(0, start);
	}

	uint32_t extractMin() {
		CHECK(size > 0);
		uint32_t min = keys[0];

		keys[0] = keys[size - 1];
		size--;
		heapify(0);

		return min;
	}

	// PRECONDITION: the distance for node id has decreased
	void decreaseKey(uint32_t id) {
		uint32_t index = reverse[id];
		uint32_t parent_index = parent(index);
		// bubble element up whilst it is smaller than its parents
		while (index > 0 && data[keys[index]] < data[keys[parent_index]]) {
			VLOG(4) << "Bubbling " << index << " up to " << parent_index;
			swap(index, parent_index);
			index = parent_index;
			parent_index = parent(index);
		}
	}

	void insert(uint32_t id) {
		size++;
		keys.resize(size);
		assign(size - 1, id);
		decreaseKey(id);
	}

	bool empty() {
		return size == 0;
	}
};

class Djikstra {
	const ResidualNetwork &g;
	const uint32_t num_nodes;
	const std::vector<uint64_t>& potentials;
	std::vector<uint64_t> distances;
	std::vector<uint32_t> parents;
	std::set<uint32_t> permanently_labelled;
	BinaryHeap priority_queue;

	// initializes structures; also clears up any state left from previous rounds
	void reset(uint32_t source) {
		distances.assign(num_nodes + 1, UINT64_MAX);
		distances[source] = 0;
		priority_queue.makeHeap(source);
		permanently_labelled.clear();
		// note there's no need to reset parents: we only read it to trace back
		// the path from the sink node, and the elements touched in this will have
		// been overwritten during run()
	}

	uint64_t reducedCost(uint32_t src, uint32_t dst, Arc *arc) {
		int64_t cost = arc->getCost() - potentials[src] + potentials[dst];
		CHECK_GE(cost, 0) << "all reduced costs should be non-negative, "
				 	 	 	 	 	    << src << "->" << dst << " has " << cost;
		return cost;
	}

	void relax(uint32_t src, uint32_t dst, Arc *arc) {
		VLOG(3) << "Djikstra: relaxing " << src << "->" << dst;
		uint64_t cost = reducedCost(src, dst, arc);
		uint64_t distance_via_arc = distances[src] + cost;
		uint64_t distance_to_dst = distances[dst];
		if (distance_via_arc < distance_to_dst) {
			distances[dst] = distance_via_arc;
			if (distance_to_dst == UINT64_MAX) {
				// dst not in priority queue
				priority_queue.insert(dst);
			} else {
				// dst in queue, need to tell queue distance has decreased
				priority_queue.decreaseKey(dst);
			}
			parents[dst] = src;
			VLOG(2) << "Djikstra: relaxed " << src << "->" << dst
					    << " new distance " << distance_via_arc;
		}
	}
public:
	Djikstra(ResidualNetwork &g, std::vector<uint64_t>& potentials)
											    : g(g), num_nodes(g.getNumNodes()),
													  potentials(potentials),	priority_queue(distances) {
		distances.resize(num_nodes + 1);
		parents.resize(num_nodes + 1);
	}

	// returns ID of permanently labelled deficit node

	uint32_t run(uint32_t source) {
		reset(source);

		while (!priority_queue.empty()) {
			uint32_t id = priority_queue.extractMin();
			VLOG(2) << "Djikstra: permanently labelling " << id;
			permanently_labelled.insert(id);

			const std::unordered_map<uint32_t, Arc*>& adjacencies = g.getAdjacencies(id);
			for (auto adjacency : adjacencies) {
				VLOG(3) << "Inspecting arc to " << adjacency.first;
				Arc *arc = adjacency.second;
				if (arc->getCapacity() > 0) {
					uint32_t dst = adjacency.first;
					relax(id, dst, arc);
				} else {
					// not in residual network, ignore
				}
			}

			if (g.getBalance(id) < 0) {
				// id, which is now permanently labelled, is a deficit node
				return id;
			}
		}

		LOG(FATAL) << "Not encountered any deficit node: illegal graph";
		return 0;
	}

	const std::vector<uint64_t>& getShortestDistances() const {
		return distances;
	}
	const std::set<uint32_t>& getPermanentlyLabelledNodes() const {
		return permanently_labelled;
	}
	const std::vector<uint32_t>& getParents() const {
		return parents;
	}
};

AugmentingPath::AugmentingPath(ResidualNetwork &g) : g(g) {
	potentials.assign(g.getNumNodes() + 1, 0);
	// note flow is initially zero (default in ResidualNetwork),
	// and potentials initialized to constant zero
}

AugmentingPath::~AugmentingPath() { }

void AugmentingPath::init() {
	// saturate all negative cost arcs, so they drop out of the residual network
	// (note algorithm requires invariant that all arcs have non-negative
	// reduced cost)
	for (Arc &arc : g) {
		if (arc.getCost() < 0) {
			g.pushFlow(arc.getSrcId(), arc.getDstId(), arc.getCapacity());
		}
	}
}

void AugmentingPath::run() {
	// create a pseudoflow satisfying reduced-cost optimality conditions
  init();
  reoptimize();
}

// SOMEDAY(adam): handle networks with no feasible solutions elegantly
void AugmentingPath::reoptimize() {
	const std::set<uint32_t> &sources = g.getSources();
	Djikstra shortest_paths(g, potentials);
	while (!sources.empty()) {
		// select some initial source
		uint32_t source = *sources.begin();

		// compute shortest path distances
		uint32_t sink = shortest_paths.run(source);
		const std::vector<uint64_t>& distances =
																   shortest_paths.getShortestDistances();
		VLOG(1) << "Permanently labeled deficit node " << sink
				    << " with distance " << distances[sink];

		// update potentials
		VLOG(1) << "Updating potentials";
		const std::set<uint32_t>& permanently_labelled =
											             shortest_paths.getPermanentlyLabelledNodes();
		uint64_t sink_distance = distances[sink];
		for (uint32_t id : permanently_labelled) {
			VLOG(3) << "Distance to " << id << " is " << distances[id];
		}
		for (uint32_t id : permanently_labelled) {
			uint32_t node_distance = distances[id];
			uint64_t new_potential = potentials[id] + sink_distance - node_distance;
			potentials[id] = new_potential;
			VLOG(3) << "Updating potential of " << id << " to " << new_potential;
		}

		// augment flow along path
		VLOG(1) << "Augmenting flow ";
		const std::vector<uint32_t>& parents = shortest_paths.getParents();
		ResidualNetworkUtil::augmentPath(g, source, sink, parents);
	}
}

} /* namespace flowsolver */
