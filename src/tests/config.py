#!/usr/bin/env python3

# Example config file
# Mock-up only

EXECUTABLES_ROOT = "build/"
DATASET_ROOT = "src/tests/graphs/"

# This variable is not used by the suite at all: it is just for convenience
# within the config for referencing particular files
files = {
  "synthetic": ["graph_1000m_32j_100t_10p.in", 
                "graph_100m_16j_100t_10p.in", "graph_100m_8j_100t_10p.in"],
  "google": ["google_all.in"],
}
files["all"] = sum(files.values(), [])

implementations = {
  "cs_latest": {
    "version": "HEAD",
    "path": "bin/find_min_cost",
    "arguments" : ["cost_scaling"]
   },
  "cs_goldberg": {
    "version": "HEAD",
    "path": "cs2/cs2",
    "arguments" : []
   },
   "cc_latest": {
    "version": "HEAD",
    "path": "bin/find_min_cost",
    "arguments" : ["cycle_cancelling"]
   },
  "cs_wave": {
   "version": "31a0d47",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
  "cs_vertexqueue": {
   "version": "7bbf977",
   "path": "bin/find_min_cost",
   "arguments" : ["cost_scaling"]
  },
}

tests = {
  "wave_vs_fifo": 
  {
    "files": files["all"],
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
    "files": files["all"],
    "iterations": 10,
    "tests": { 
      str(x): {
        "implementation": "cs_latest",
        "arguments": ["--scaling--factor", x]
      } for x in range(2,32)
    }
  },
}