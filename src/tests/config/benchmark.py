#!/usr/bin/env python3

# Example config file
# Mock-up only

import os, sh

from config.common import *

WORKING_DIRECTORY = "/tmp/flowsolver_benchmark"
RESULT_ROOT = os.path.join(PROJECT_ROOT, "benchmark")
FIRMAMENT_ROOT = os.path.join(PROJECT_ROOT, "..", "firmament")
MAKE_FLAGS = []

try:
  # allow settings to be overridden on a local basis
  from config.benchmark_local import *
except ImportError:
  pass

##### Executables
 
GOOGLE_TRACE_SIMULATOR_PATH = os.path.join(FIRMAMENT_ROOT, 
                      "build", "sim", "trace-extract", "google_trace_simulator")
GOOGLE_TRACE_SIMULATOR = sh.Command(GOOGLE_TRACE_SIMULATOR_PATH)

##### Dataset

# This variable is not used by the suite at all: it is just for convenience
# within the config for referencing particular files
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

FULL_DATASET["synthetic"] = FULL_DATASET["synthetic_small"] + FULL_DATASET["synthetic_large"]

all_files = set()
for files in FULL_DATASET.values():
  all_files.update(files)
FULL_DATASET["all"] = all_files

INCREMENTAL_DATASET = {
  "google_tiny_trace": ["clusters/natural/google_trace/tiny_trace.imin"],
  "google_small_trace": ["clusters/natural/google_trace/small_trace.imin"],
}

##### Implementations
FULL_IMPLEMENTATIONS = {
  "cs_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
  },
  "relax_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments": ["relax"]
  },
   "ap_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["augmenting_path"]
  },
  "cc_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cycle_cancelling"]
  },
  "cs_goldberg": {
    "version": "master",
    "target": "cs2.exe",
    "path": "cs2/cs2",
    "arguments" : []
  },
  "relax_frangioni": {
    "version": "master",
    "target": "RelaxIV",
    "path": "RelaxIV/RelaxIV",
    "arguments": []
  },
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
  "parser_getarc": {
    "version": "5682b385315a2175b6890b4183185f233b094e28",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
  },
   "parser_set": {
     "version": "04db7f8e109ab695f6bafc11d95a1cdd8646d6e3",
     "target": "find_min_cost",
     "path": "bin/find_min_cost",
     "arguments" : ["cost_scaling"]
  },
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
  "relax_firstworking": {
    "version": "f2cc904",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments": ["relax"]
  },                
}

LEMON_ALGOS = ["scc", "mmcc", "cat", "ssp", "cas", "cos", "ns"]
for algo in LEMON_ALGOS:
  FULL_IMPLEMENTATIONS["lemon_" + algo] = {
    "version": "master",
    "target": "lemon_min_cost",
    "path": "bin/lemon_min_cost",
    "arguments": ["-" + algo]
  }
  
INCREMENTAL_IMPLEMENTATIONS = {
  "ap_incremental_latest": {
    "version": "master",
    "target": "incremental_min_cost",
    "path": "bin/incremental_min_cost",
    "arguments": []
   },
  "relaxfi_latest": {
    "version": "master",
    "target": "RelaxIV_incremental",
    "path": "RelaxIV/RelaxIV_incremental",
    "arguments": []
  },
  "relaxfi_firstworking": {
    "version": "2aae245",
    "target": "RelaxIV_incremental",
    "path": "RelaxIV/RelaxIV_incremental",
    "arguments": []
  },
}

IMPLEMENTATIONS = mergeDicts([FULL_IMPLEMENTATIONS, INCREMENTAL_IMPLEMENTATIONS],
                            ["full", "incremental"])

##### Test cases

FULL_TESTS = {
  "development_only_full": {
    "files": FULL_DATASET["development_only"],
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "cc_latest",
      },
      "goldberg": {
        "implementation": "cs_goldberg",
      },
    },
  },
  "wave_vs_fifo": {
    "files": FULL_DATASET["synthetic"],
    "iterations": 5,
    "tests": {
      "wave": {
        "implementation": "cs_wave",
      },
      "fifo": {
        "implementation": "cs_vertexqueue",
      },
    },
  },
  "scaling_factor": {
    "files": FULL_DATASET["all"],
    "iterations": 5,
    "tests": { 
      str(x): {
        "implementation": "cs_latest",
        "arguments": ["--scaling--factor", x]
      } for x in range(2,32)
    }
  },
 "dimacs_parser_set_vs_getarc": {
    "files": FULL_DATASET["synthetic_large"] + FULL_DATASET["google"],
    "iterations": 5,
    "tests": {
      "set": {
        "implementation": "parser_set",
      },
      "getarc": {
        "implementation": "parser_getarc",
      },
    },
  },
  "augmenting_vs_costscaling": {
    "files": FULL_DATASET["synthetic_large"] + FULL_DATASET["google"],
    "iterations": 3,
    "tests": {
      "cost_scaling": {
        "implementation": "cs_latest",
      },
      "my_augmenting_path": {
        "implementation": "ap_latest",
      },
      "lemon_augmenting_path": {
        "implementation": "lemon_ssp",
      }
    },
  },
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
  "augmenting_big_vs_small_heap": {
    "files": FULL_DATASET["synthetic"] + FULL_DATASET["google"],
    "iterations": 5,
    "tests": {
      "big": {
        "implementation": "ap_bigheap",
      },
      "small_vector": {
        "implementation": "ap_smallheap_vector",
      },
      "small_map": {
        "implementation": "ap_smallheap_map",
      },
    },
  },
  "best_head_to_head": {
    "files": FULL_DATASET["synthetic"] + FULL_DATASET["google"],
    "iterations": 5,
    "tests": {
      "goldberg": {
        "implementation": "cs_goldberg",
      },
      "relaxiv": {
        "implementation": "relax_frangioni",
      },
      "my_costscaling": {
        "implementation": "cs_latest",
      },
    }
  },
  "relax": {
    "files": FULL_DATASET["synthetic_small"],
    "iterations": 3,
    "tests": {
      "relax_mine": {
        "implementation": "relax_latest",
      },  
      "relax_original": {
        "implementation": "relax_firstworking",
      },
      "ap_mine": {
        "implementation": "ap_latest",
      },
      "cs_goldberg": {
        "implementation": "cs_goldberg",
      },
      "relax_frangioni": {
        "implementation": "relax_frangioni",
      },
    },
  },
}

INCREMENTAL_TESTS_OFFLINE = {
  "development_only_incremental_offline": {
    "files": INCREMENTAL_DATASET["google_tiny_trace"],
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "ap_incremental_latest",
      },
      "goldberg": {
        "implementation": "cs_goldberg",
      },
    },
  },
  "incremental_vs_cs_offline": {
    "files": INCREMENTAL_DATASET["google_small_trace"],
    "iterations": 10,
    "tests": {
      "my_incremental": {
        "implementation": "ap_incremental_latest",
      },
      "my_costscaling": {
        "implementation": "cs_latest",
      },
      "goldberg": {
        "implementation": "cs_goldberg",
      },
    },
  },
}

TRACE_ROOT = "/data/adam/"
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

def percentRuntime(p):
  return TRACE_START + (TRACE_LENGTH - TRACE_START) * p

INCREMENTAL_TESTS_HYBRID = {
  "development_only_incremental_hybrid": {
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
        "implementation": "ap_incremental_latest",
        "arguments": [],
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": [],
      },
    },
  },        
  "incremental_vs_cs_hybrid": {
    "traces": [
      {
        "name": "small_trace",
        "runtime": RUNTIME_MAX
      },
    ],
    "granularity": 10, # in microseconds
    "iterations": 5,
    "tests": {
      "my_incremental": {
        "implementation": "ap_incremental_latest",
        "arguments": []
      },
      "my_costscaling": {
        "implementation": "cs_latest",
        "arguments": []
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": []
      },
    },
  },
  "incremental_vs_cs_quick": {
    "traces": [
      {
        "name": "small_trace",
        # this runtime corresponds to the first 10,000 events
        "runtime": 3995607400
      },
    ],
    "granularity": 10, # in microseconds
    "percentage": 1, # percentage of events to retain
    "iterations": 5,
    "tests": {
      "my_incremental": {
        "implementation": "ap_incremental_latest",
        "arguments": []
      },
      "my_costscaling": {
        "implementation": "cs_latest",
        "arguments": []
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": []
      },
    },
  },
  "relaxfi_vs_best_quick": {
    "traces": [
      {
        "name": "small_trace",
        # this runtime corresponds to the first 10,000 events
        "runtime": 3995607400
      },
    ],
    "granularity": 10, # in microseconds
    "percentage": 1, # percentage of events to retain
    "iterations": 5,
    "tests": {
      "relaxfi": {
        "implementation": "relaxfi_latest",
        "arguments": [] 
      },
      "relaxfi_firstworking": {
        "implementation": "relaxfi_firstworking",
        "arguments": [] 
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": []
      },
    },
  },                                       
}

INCREMENTAL_TESTS_ONLINE = {
  "development_only_incremental_online": {
    "traces": [
      {
       "name": "tiny_trace",
       "runtime": RUNTIME_MAX
      },
    ],
    "granularity": 10, # in microseconds
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "ap_incremental_latest",
        "arguments": [],
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": [],
      },
    },
  },
  "incremental_vs_cs_online": 
    INCREMENTAL_TESTS_HYBRID["incremental_vs_cs_hybrid"].copy()
}

TESTS = mergeDicts(
  [FULL_TESTS, 
   INCREMENTAL_TESTS_OFFLINE, 
   INCREMENTAL_TESTS_HYBRID,
   INCREMENTAL_TESTS_ONLINE], 
  ["full", "incremental_offline", "incremental_hybrid", "incremental_online"])