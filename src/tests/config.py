#!/usr/bin/env python3

# Example config file
# Mock-up only

import os

SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_ROOT))

#WORKING_DIRECTORY = os.path.join(PROJECT_DIR, "tmp/tests/")
WORKING_DIRECTORY = "/tmp/flowsolver_benchmark"
DATASET_ROOT = os.path.join(PROJECT_ROOT, "src", "tests", "graphs")
RESULT_ROOT = os.path.join(PROJECT_ROOT, "benchmark")
BUILD_ROOT = "build"
SOURCE_ROOT = "src"

def prefix_list(prefix, fnames):
  return list(map(functools.partial(os.path.join(prefix)), fnames))

# This variable is not used by the suite at all: it is just for convenience
# within the config for referencing particular files
FILES = {
  # these are too small to be useful for a benchmark: but handy to quickly test
  # the benchmark script itself
  "development_only": ["firmament_synthetic/graph_4m_2crs_10j.in", 
                       "handmade/small_graph.in"],
  "synthetic_small": prefix_list("firmament_synthetic",
                      ["graph_100m_8j_100t_10p.in", 
                       "graph_100m_16j_100t_10p.in"]),
  "synthetic_large": ["firmament_synthetic/graph_1000m_32j_100t_10p.in"],
  "google": ["google_trace/google_all.in"],
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
  "cs_goldberg": {
    "version": "master",
    "target": "cs2",
    "path": "cs2/cs2",
    "arguments" : []
   },
   "cc_latest": {
    "version": "master",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cycle_cancelling"]
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
}
