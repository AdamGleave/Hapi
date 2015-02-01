#!/usr/bin/env python3

# Example config file
# Mock-up only

import os

from config.common import *

WORKING_DIRECTORY = "/tmp/flowsolver_benchmark"
RESULT_ROOT = os.path.join(PROJECT_ROOT, "benchmark")
MAKE_FLAGS = []

try:
  # allow settings to be overridden on a local basis
  from config.benchmark_local import *
except ImportError:
  pass

# This variable is not used by the suite at all: it is just for convenience
# within the config for referencing particular files
FILES = {
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

FILES["synthetic"] = FILES["synthetic_small"] + FILES["synthetic_large"]

all_files = set()
for files in FILES.values():
  all_files.update(files)
FILES["all"] = all_files

IMPLEMENTATIONS = {
  "cs_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
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
    "target": "cs2",
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
}

LEMON_ALGOS = ["scc", "mmcc", "cat", "ssp", "cas", "cos", "ns"]
for algo in LEMON_ALGOS:
  IMPLEMENTATIONS["lemon_" + algo] = {
    "version": "master",
    "target": "lemon_min_cost",
    "path": "bin/lemon_min_cost",
    "arguments": ["-" + algo]
  }

TESTS = {
  "development_only": {
    "files": FILES["development_only"],
    "iterations": 1,
    "tests": {
      "my": {
        "implementation": "cc_latest",
        "arguments": []
      },
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": []
      },
    },
  },
  "wave_vs_fifo": {
    "files": FILES["synthetic"],
    "iterations": 5,
    "tests": {
      "wave": {
        "implementation": "cs_wave",
        "arguments": []
      },
      "fifo": {
        "implementation": "cs_vertexqueue",
        "arguments": []
      },
    },
  },
  "scaling_factor": {
    "files": FILES["all"],
    "iterations": 5,
    "tests": { 
      str(x): {
        "implementation": "cs_latest",
        "arguments": ["--scaling--factor", x]
      } for x in range(2,32)
    }
  },
 "dimacs_parser_set_vs_getarc": {
    "files": FILES["synthetic_large"] + FILES["google"],
    "iterations": 5,
    "tests": {
      "set": {
        "implementation": "parser_set",
        "arguments": []
      },
      "getarc": {
        "implementation": "parser_getarc",
        "arguments": []
      },
    },
  },
  "augmenting_vs_costscaling": {
    "files": FILES["synthetic_large"] + FILES["google"],
    "iterations": 3,
    "tests": {
      "cost_scaling": {
        "implementation": "cs_latest",
        "arguments": []
      },
      "my_augmenting_path": {
        "implementation": "ap_latest",
        "arguments": []
      },
      "lemon_augmenting_path": {
        "implementation": "lemon_ssp",
        "arguments": []
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
    "files": FILES["synthetic"] + FILES["google"],
    "iterations": 5,
    "tests": {
      "big": {
        "implementation": "ap_bigheap",
        "arguments": []
      },
      "small_vector": {
        "implementation": "ap_smallheap_vector",
        "arguments": []
      },
      "small_map": {
        "implementation": "ap_smallheap_map",
        "arguments": []
      },
    },
  },
  "best_head_to_head": {
    "files": FILES["synthetic"] + FILES["google"],
    "iterations": 5,
    "tests": {
      "goldberg": {
        "implementation": "cs_goldberg",
        "arguments": []
      },
      "relaxiv": {
        "implementation": "relax_frangioni",
        "arguments": []
      },
      "my_costscaling": {
        "implementation": "cs_latest",
        "arguments": []
      },
    }
  }
}
