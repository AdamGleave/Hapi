#!/usr/bin/env python3

# Example config file
# Mock-up only

import os

SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_ROOT))

#WORKING_DIRECTORY = os.path.join(PROJECT_DIR, "tmp/tests/")
WORKING_DIRECTORY = "/tmp/flowsolver_benchmark"
DATASET_ROOT = os.path.join(PROJECT_ROOT, "src", "TESTS", "graphs")
BUILD_ROOT = "build"
SOURCE_ROOT = "src"

# This variable is not used by the suite at all: it is just for convenience
# within the config for referencing particular FILES
FILES = {
  "synthetic": ["graph_1000m_32j_100t_10p.in", 
                "graph_100m_16j_100t_10p.in", "graph_100m_8j_100t_10p.in"],
  "google": ["google_all.in"],
}
FILES["all"] = sum(FILES.values(), [])

IMPLEMENTATIONS = {
  "cs_latest": {
    "version": "HEAD",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
   },
  "cs_goldberg": {
    "version": "HEAD",
    "target": "cs2",
    "path": "cs2/cs2",
    "arguments" : []
   },
   "cc_latest": {
    "version": "HEAD",
    "target": "find_min_cost",
    "path": "bin/find_min_cost",
    "arguments" : ["cycle_cancelling"]
   },
  "cs_wave": {
   "version": "31a0d47",
   "target": "find_min_cost",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
  "cs_vertexqueue": {
   "version": "7bbf977",
   "target": "find_min_cost",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
}

TESTS = {
  "wave_vs_fifo": 
  {
    "files": FILES["all"],
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
}