#!/usr/bin/env python3

# Example config file
# Mock-up only

import os, sh

from config.common import *

WORKING_DIRECTORY = "/tmp/flowsolver_benchmark"
RESULT_ROOT = os.path.join(PROJECT_ROOT, "benchmark")
FIRMAMENT_ROOT = os.path.join(os.path.dirname(PROJECT_ROOT), "firmament")
MAKE_FLAGS = []

try:
  # allow settings to be overridden on a local basis
  from config.benchmark_local import *
except ImportError:
  pass

##### Executables
 
GOOGLE_TRACE_SIMULATOR_PATH = os.path.join(FIRMAMENT_ROOT, 
                      "build", "sim", "trace-extract", "google_trace_simulator")
GOOGLE_TRACE_SIMULATOR_ARGS = ["--logtostderr"]
GOOGLE_TRACE_SIMULATOR = sh.Command(GOOGLE_TRACE_SIMULATOR_PATH) \
                           .bake(*GOOGLE_TRACE_SIMULATOR_ARGS)

##### Dataset
# Note these variables are not used by the suite at all. They are provided
# for convenience within the config, to allow us to reference particular sets
# of files by a short name.

### Full graphs
FULL_DATASET = {
  ### Networks representing clusters
  
  # these are too small to be useful for a benchmark: but handy to quickly test
  # the benchmark script itself
  "development_only": ["clusters/synthetic/firmament/graph_4m_2crs_10j.in", 
                       "clusters/synthetic/handmade/small_graph.in"],
  # 100 machine networks
  "synthetic_small": graphGlob("clusters/synthetic/firmament/graph_100m_*"),
  # 1000 machine networks
  "synthetic_large": ["clusters/synthetic/firmament/graph_1000m_32j_100t_10p.in"],
  # Network from Google cluster trace
  "google": ["clusters/natural/google_trace/google_all.in"],
  
  ### General flow networks
  ### See https://lemon.cs.elte.hu/trac/lemon/wiki/MinCostFlowData
  
  ## ROAD
  # ROAD-PATHS: all arc capacities one
  # ROAD-FLOW: capacity set to 40/60/80/100 depending on road category
  "road_paths": graphGlob("general/natural/road/road_paths_*.min"),
  "road_flow": graphGlob("general/natural/road/road_flow_*.min"),
  "road": graphGlob("general/natural/road/road_*.min"),
  
  ## VISION
  # VISION-RND: The arc costs are selected uniformly at random.
  # VISION-PROP: The cost of an arc is approximately proportional to its capacity.
  # VISION-INV: The cost of an arc is approximately inversely proportional to its capacity.
  "vision_rnd": graphGlob("general/natural/vision/vision_rnd_*.min"),
  "vision_prop": graphGlob("general/natural/vision/vision_prop_*.min"),
  "vision_inv": graphGlob("general/natural/vision/vision_inv_*.min"),
  "vision": graphGlob("general/natural/vision/vision_*.min"),
  
  ## NETGEN
  # NETGEN-8: Sparse networks, with m = 8n. Capacities and costs uniform random.
  #           Supply and demand nodes are sqrt(n);
  #           average supply per suppply node is 1000.
  # NETGEN-SR: As above, but m = n*sqrt(n) (relatively dense)
  # NETGEN-LO-8: As NETGEN-8, but with much lower average supply, around 10.
  # NETGEN-LO-SR: As NETGEN-SR, but with much lower average supply, around 10.
  # NETGEN-DEG: n=4096 fixed, average outdegree ranges from 2 to n/2. 
  #             Otherwise, as for NETGEN-8. 
  "netgen_8": graphGlob("general/synthetic/netgen/netgen_8_*.min"), 
  "netgen_sr": graphGlob("general/synthetic/netgen/netgen_sr_*.min"),
  "netgen_lo_8": graphGlob("general/synthetic/netgen/netgen_lo_8_*.min"),
  "netgen_lo_sr": graphGlob("general/synthetic/netgen/netgen_lo_sr_*.min"),
  "netgen_deg": graphGlob("general/synthetic/netgen/netgen_deg_*.min"),
  
  ## GOTO (Grid On Torus)
  # GOTO-8: Equivalent parameters to NETGEN-8.
  # GOTO-SR: Equivalent parameters to NETGEN-SR.
  "goto_8": graphGlob("general/synthetic/goto/goto_8_*.min"),
  "goto_sr": graphGlob("general/synthetic/goto/goto_sr_*.min"),
}

FULL_DATASET["synthetic"] = FULL_DATASET["synthetic_small"] \
                          + FULL_DATASET["synthetic_large"]

all_files = set()
for files in FULL_DATASET.values():
  all_files.update(files)
FULL_DATASET["all"] = all_files

### Incremental graphs
INCREMENTAL_DATASET = {
  # Not real data, and too small to be useful. Good for testing benchmark script.
  "development_only": ["clusters/natural/google_trace/tiny_trace.imin"],
  # CS2 on the small Google trace. Only the first 7 deltas.
  "google_small_trace_truncated": ["clusters/natural/google_trace/small_trace.imin"],
}

### Google cluster trace(s)
TRACE_DATASET = {
  "tiny_trace": 
  {
    "dir": os.path.join(TRACE_ROOT, "tiny_trace"),
    "num_files": 1,
  },
  "small_trace": {
    "dir": os.path.join(TRACE_ROOT, "small_trace"),
    "num_files": 1,
  },
}

SECOND = 10**6 # 1 second in microseconds
TRACE_START = 600*SECOND
# timestamp of last event in trace
TRACE_LENGTH = 2506199602822
RUNTIME_MAX = 2**64 - 1

RUNTIME_50_ITERATIONS_1PER = 760953480
RUNTIME_100_ITERATIONS_1PER = 870117760
RUNTIME_100_ITERATIONS = 608159450
FIRST_10K_EVENTS = 3995607400 # corresponds to 1470 iterations at 10 us
def percentRuntime(p):
  return TRACE_START + (TRACE_LENGTH - TRACE_START) * (p / 100.0)

##### Implementations

### Full solvers
FULL_IMPLEMENTATIONS = {
  ### My implementations - latest
  "ap_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["augmenting_path", "--quiet"],
    "offline_arguments": ["--flow", "false"]
  },
  "cc_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cycle_cancelling", "--quiet"], 
    "offline_arguments": ["--flow", "false"]
  },
  "cs_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments": ["cost_scaling", "--quiet"],
    "offline_arguments": ["--flow", "false"]
  },
  "relax_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments": ["relax", "--quiet"],
    "offline_arguments": ["--flow", "false"]
  },

  ### Reference implementations
  ### (Not all of them -- LEMON included below)
  "cs_goldberg": {
    "version": "master",
    "target": "cs2.exe",
    "path": "cs2/cs2",
    "offline_arguments" : ["-f", "false"]
  },
  "relax_frangioni": {
    "version": "master",
    "target": "RelaxIV_original",
    "path": "RelaxIV/RelaxIV_original",
    "offline_arguments": ["--flow", "false"]
  },
                        
  ### My implementations - specific versions
  ### These are used for testing particular optimisations which have been applied
  
  # Augmenting path    
  "ap_bigheap": {
     "version": "1540fc0",
     "target": "find_min_cost",
     "path": "bin/find_min_cost",
     "arguments" : ["augmenting_path"]
  },
   "ap_smallheap_vector": {
     "version": "41c6852",
     "target": "find_min_cost",
     "path": "bin/find_min_cost",
     "arguments" : ["augmenting_path"]
  },
   "ap_smallheap_map": {
     "version": "cd42c6e",
     "target": "find_min_cost",
     "path": "bin/find_min_cost",
     "arguments" : ["augmenting_path"]
  }, 
  # Cost scaling
  "cs_wave": {
   "version": "cs_wave",
   "target": "find_min_cost",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
  "cs_vertexqueue": {
   "version": "5784157",
   "target": "find_min_cost",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
  # Parser
  "parser_getarc": {
    "version": "5682b38",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
  },
   "parser_set": {
     "version": "04db7f8",
     "target": "find_min_cost",
     "path": "bin/find_min_cost",
     "arguments" : ["cost_scaling"]
  },
  # RELAX
  "relax_firstworking": {
    "version": "f2cc904",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments": ["relax"]
  },                
}

# Reference implementations - LEMON
LEMON_ALGOS = ["scc", "mmcc", "cat", "ssp", "cas", "cos", "ns"]
for algo in LEMON_ALGOS:
  FULL_IMPLEMENTATIONS["lemon_" + algo] = {
    "version": "master",
    "target": "lemon_min_cost",
    "path": "bin/lemon_min_cost",
    "arguments": ["-" + algo]
  }

### Incremental solvers
MY_INCREMENTAL_OFFLINE_ARGS = ["--flow", "false"]
INCREMENTAL_IMPLEMENTATIONS = {
  ### My solvers - latest
  "ap_latest": {
    "version": "master",
    "target": "incremental_min_cost",
    "path": "bin/incremental_min_cost",
    "arguments": ["augmenting_path", "--quiet"],
    "offline_arguments": MY_INCREMENTAL_OFFLINE_ARGS,
   },
   "relax_latest": {
    "version": "master",
    "target": "incremental_min_cost",
    "path": "bin/incremental_min_cost",
    "arguments": ["relax", "--quiet"],
    "offline_arguments": MY_INCREMENTAL_OFFLINE_ARGS,
   },
  ### RELAX Frangioni with incremental additions
  # Latest
  "relaxf_latest": {
    "version": "master",
    "target": "RelaxIV_incremental",
    "path": "RelaxIV/RelaxIV_incremental",
    "arguments": ["--quiet"],
    "offline_arguments": MY_INCREMENTAL_OFFLINE_ARGS,
  },
  # First version that fully works. Includes bugfix to solve uninitialized value
  # access in tfstin/tnxtin/etc. 
  "relaxf_firstworking": {
    "version": "b5721bb",
    "target": "incremental",
    "path": "RelaxIV/RelaxIV_incremental",
  },
}

IMPLEMENTATIONS = mergeDicts([FULL_IMPLEMENTATIONS, INCREMENTAL_IMPLEMENTATIONS],
                             ["full", "incremental"], ["f", "i"])

##### Test cases

### Tests on full graphs, comparing only full solvers
FULL_TESTS = {
  # For testing benchmark suite only
  "development_only": {
    "files": FULL_DATASET["development_only"],
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "f_cc_latest",
      },
      "goldberg": {
        "implementation": "f_cs_goldberg",
      },
    },
  },
              
  ### Optimisation tests
  ## Augmenting path
               
  # Big heap: keeps all vertices in the priority queue. O(n) to create, but
  # all operations are O(n lg n) afterwards.
  # Small heap: keeps only vertices with finite distance in priority queue.
  # O(1) to create, but has to pay insertion cost. Operations cheaper provided
  # there are vertices not in queue.
  # Performance will ultimately depend on how many vertices get explored before
  # Djikstra quits.
  # Additionally, have the choice between maintaining the reverse index
  # as a vector or a map. Vector will give guaranteed O(1) lookup, and we 
  # don't care about the memory consumption, but map may actually perform 
  # better since it can be better cached.
  "ap_big_vs_small_heap": {
    "files": FULL_DATASET["synthetic"] + FULL_DATASET["google"],
    "iterations": 5,
    "tests": {
      "big": {
        "implementation": "f_ap_bigheap",
      },
      "small_vector": {
        "implementation": "f_ap_smallheap_vector",
      },
      "small_map": {
        "implementation": "f_ap_smallheap_map",
      },
    },
  },
              
  ## Cost scaling
  "cs_wave_vs_fifo": {
    "files": FULL_DATASET["synthetic"],
    "iterations": 5,
    "tests": {
      "wave": {
        "implementation": "f_cs_wave",
      },
      "fifo": {
        "implementation": "f_cs_vertexqueue",
      },
    },
  },
  "cs_scaling_factor": {
    "files": FULL_DATASET["all"],
    "iterations": 5,
    "tests": { 
      str(x): {
        "implementation": "f_cs_latest",
        "arguments": ["--scaling--factor", x]
      } for x in range(2,32)
    }
  },
              
 ## DIMACS parser
 "parser_set_vs_getarc": {
    "files": FULL_DATASET["synthetic_large"] + FULL_DATASET["google"],
    "iterations": 5,
    "tests": {
      "set": {
        "implementation": "f_parser_set",
      },
      "getarc": {
        "implementation": "f_parser_getarc",
      },
    },
  },
 
  ### Comparison tests
}

INCREMENTAL_TESTS_OFFLINE = {
  # For testing benchmark suite only.
  "development_only": {
    "files": INCREMENTAL_DATASET["development_only"],
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "i_ap_latest",
      },
      "goldberg": {
        "implementation": "f_cs_goldberg",
      },
    },
  },
                            
  # 
  "incremental_vs_cs": {
    "files": INCREMENTAL_DATASET["google_small_trace_truncated"],
    "iterations": 10,
    "tests": {
      "my_incremental": {
        "implementation": "i_ap_latest",
      },
      "my_costscaling": {
        "implementation": "f_cs_latest",
      },
      "goldberg": {
        "implementation": "f_cs_goldberg",
      },
    },
  },
  "relaxfi_vs_best": {
    "files": INCREMENTAL_DATASET["google_small_trace_truncated"],
    "iterations": 5,
    "tests": {
      "relaxfi": {
        "implementation": "i_relaxf_latest",
        "arguments": [] 
      },
      "goldberg": {
        "implementation": "f_cs_goldberg",
        "arguments": []
      },
    },
  },
}

### Incremental tests: available either as hybrid, or pure online

COST_MODELS = {
               "trivial": 0,
               "random": 1,
               "sjf": 2,
               "quincy": 3,
               "whare": 4,
               "coco": 5,
               "octopus": 6
}

DEFAULT_COST_MODEL = "octopus"

INCREMENTAL_TESTS_ANYONLINE = {
  # Testing benchmark suite only.
  "development_only": {
    "traces": [
      {
       "name": "tiny_trace",
       "runtime": RUNTIME_MAX
      },
    ],
    "granularity": 10, # in microseconds
    "iterations": 3,
    "tests": {
      "my": {
        "implementation": "i_ap_latest",
        "arguments": [],
      },
      "goldberg": {
        "implementation": "f_cs_goldberg",
        "arguments": [],
      },
    },
   },
                               
  ### Self comparisons
  ### How does the performance of an incremental solver compare to using the
  ### same solver in a non-incremental mode? Similarly, what proportion of work
  ### must the incremental solver do?
  "ap_same": {
    # Augmenting path is slow. Give it a small dataset.
    "traces": [
      {
       "name": "small_trace",
       "runtime": RUNTIME_50_ITERATIONS_1PER,
       "percentage": 1
      }
    ],
    "granularity": 10,
    "iterations": 5,
    "tests": {
      "full":           { "implementation": "f_ap_latest" },
      "incremental":    { "implementation": "i_ap_latest" },
    },
  },
  "relax_same": {
    # This implementation of RELAX is 4-5x faster than augmenting path.
    # But still too slow for a large dataset.
    "traces": [
      {
       "name": "small_trace",
       "runtime": RUNTIME_50_ITERATIONS_1PER,
       "percentage": 1
      }
    ],
    "granularity": 10,
    "iterations": 5,
    "tests": {
      "full":           { "implementation": "f_relax_latest" },
      "incremental":    { "implementation": "i_relax_latest" },
    },
  },
  "relaxf_same": {
    # This is the optimised version of RELAX. Give it a full-size dataset.
    "traces": [
      {
       "name": "small_trace",
       "runtime": RUNTIME_100_ITERATIONS,
      }
    ],
    "granularity": 10,
    "iterations": 5,
    "tests": {
      "full":           { "implementation": "f_relax_frangioni" },
      "incremental":    { "implementation": "i_relaxf_latest" },
    },
  },
  
  ### Comparisons between types
  # Evaluation against the best of my (unoptimized) implementations.
  "my_head_to_head": {
    # Give them a small dataset
    "traces": [
      {
       "name": "small_trace",
       "runtime": RUNTIME_50_ITERATIONS_1PER,
       "percentage": 1
      }
    ],
    "granularity": 10,
    "iterations": 5,
    "tests": {
      "full":           { "implementation": "f_cs_latest" },
      "incremental":    { "implementation": "i_relax_latest" },
    }
  },
  # Fight against the best optimized implementations.
  # Goldberg for full solver, modified RelaxIV for incremental solver.
  "optimized_head_to_head": {
    # These implementations can handle a full-size dataset
    "traces": [
      {
       "name": "small_trace",
       "runtime": RUNTIME_100_ITERATIONS,
      }
    ],
    "granularity": 10,
    "iterations": 5,
    "tests": {
      "full":           { "implementation": "f_cs_goldberg" },
      "incremental":    { "implementation": "i_relaxf_latest" },
    },
  },                                  
}

INCREMENTAL_TESTS_HYBRID = INCREMENTAL_TESTS_ANYONLINE.copy()
INCREMENTAL_TESTS_ONLINE = INCREMENTAL_TESTS_ANYONLINE.copy()

TESTS = mergeDicts(
  [FULL_TESTS, 
   INCREMENTAL_TESTS_OFFLINE, 
   INCREMENTAL_TESTS_HYBRID,
   INCREMENTAL_TESTS_ONLINE], 
  ["full", "incremental_offline", "incremental_hybrid", "incremental_online"],
  ["f", "iof", "ihy", "ion"])
