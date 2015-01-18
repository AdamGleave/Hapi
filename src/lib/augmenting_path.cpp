#include <deque>
#include <glog/logging.h>

#include "arc.h"
#include "augmenting_path.h"
#include "residual_network_util.h"

namespace flowsolver {

// min heap
class BinaryHeap {
	const std::vector<uint64_t> &data;
	std::vector<uint32_t> keys;
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
			std::iter_swap(keys.begin() + index, keys.begin() + smallest);
			heapify(smallest);
		}
	}
public:
	// PRECONDITION: data[start_id] == 0, data[i] = UINT64_MAX for all other i
	BinaryHeap(std::vector<uint64_t> &data) : data(data), size(0) { }

	void makeHeap(uint32_t start) {
		size = data.size();
		VLOG(1) << "Building heap of size " << size << " with start " << start;
		keys.resize(size);

		// first key in min-heap is start
		keys[0] = start;

		// all other keys have the same value, and so can appear in any order
		uint32_t i;
		for (i = 1; i <= start; i++) {
			keys[i] = i - 1;
		}
		for (; i < size; i++) {
			keys[i] = i;
		}
	}

	uint32_t extractMin() {
		CHECK(size > 0);
		uint32_t min = keys[0];

		keys[0] = keys[size - 1];
		size--;
		heapify(0);

		return min;
	}

	// PRECONDITION: data[id] <= the original value of data[id]
	void decreaseKey(uint32_t id) {
		uint32_t parent_id = parent(id);
		// bubble element up whilst it is smaller than its parents
		while (id > 0 && data[keys[id]] < data[keys[parent_id]]) {
			VLOG(4) << "Bubbling " << id << " up to " << parent_id;
			std::iter_swap(keys.begin() + id, keys.begin() + parent_id);
			id = parent_id;
			parent_id = parent(id);
		}
	}

	bool empty() {
		return size == 0;
	}
};

// TODO(adam): make templated class for min-priority queue?
// TODO(adam): facility to reset Djikstra rather than re-create everything?
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
		// XXX(adam): heap doesn't need to be initialized to contain all vertices,
		// just add them in as we explore them
		priority_queue.makeHeap(source);
		permanently_labelled.clear();
		// TODO(adam): is this necessary?
		parents.assign(num_nodes + 1, 0);
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
		if (distance_via_arc < distances[dst]) {
			distances[dst] = distance_via_arc;
			priority_queue.decreaseKey(dst);
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

AugmentingPath::AugmentingPath(ResidualNetwork &g)
																						: g(g), num_nodes(g.getNumNodes()) {
	//potentials.resize(num_nodes + 1);
	potentials.assign(num_nodes + 1, 0);
	// note flow is initially zero (default in ResidualNetwork),
	// and potentials initialized to constant zero
}

// TODO(adam): potentially returning a big object, will compiler inline this?
std::queue<Arc *> AugmentingPath::predecessorPath
			(uint32_t source, uint32_t sink, const std::vector<uint32_t>& parents) {
	std::deque<Arc *> path;
	uint32_t cur, prev;

	cur = sink;
	while (cur != source) {
		prev = parents[cur];
		Arc *arc = g.getArc(prev, cur);
		path.push_front(arc);
		cur = prev;
	}

	return std::queue<Arc *>(path);
}

// TODO(adam): we might want to maintain sources ourselves,
// when we implement delta-scaling, as then not all excess vertices
// are eligible to be used as starting vertex
// TODO(adam): what happens if there's no feasible solution?
void AugmentingPath::run() {
	const std::set<uint32_t> &sources = g.getSources();
	while (!sources.empty()) {
		// select some initial source
		uint32_t source = *sources.begin();

		// compute shortest path distances
		Djikstra shortest_paths(g, potentials);
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
		std::queue<Arc*> augmenting_path = predecessorPath(source, sink, parents);
		ResidualNetworkUtil::augmentPath(g, augmenting_path);
	}
}

AugmentingPath::~AugmentingPath() { }

} /* namespace flowsolver */
