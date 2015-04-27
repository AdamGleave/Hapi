import os
from enum import Enum

from config.common import *
from visualisation.test_types import FigureTypes

def dictFilter(d):
  return lambda k : d[k]

### Miscellaneous settings
CONFIDENCE_LEVEL = 0.95

### Paths

DOC_PREFIX = "doc"
DATA_PREFIX = "data"
FIGURE_PREFIX = "figures"
DOC_ROOT = os.path.join(PROJECT_ROOT, DOC_PREFIX)
DATA_ROOT = os.path.join(DOC_ROOT, DATA_PREFIX)
FIGURE_ROOT = os.path.join(DOC_ROOT, FIGURE_PREFIX)

### Optimisation test cases

OPTIMISATION_FILE_FILTER = dictFilter({
  'clusters/natural/google_trace/octopus/1hour/full_size.min': 'Warehouse Scale',
  'clusters/natural/google_trace/octopus/1hour/large.min': 'Large',
  'clusters/natural/google_trace/octopus/1hour/medium.min': 'Medium',
  'clusters/natural/google_trace/octopus/1hour/small.min': 'Small',
})

OPTIMISATION_FIGURES = {
  ### Optimisations
  ## Augmenting path
  'ap_big_vs_small_absolute': {
    'data': 'f_opt_ap_big_vs_small_heap',
    'type': FigureTypes.optimisation_absolute,
    'file_filter': OPTIMISATION_FILE_FILTER,
    'test_filter': dictFilter({
      'big': 'Big Heap',
      'small_vector': 'Small Heap',
      'small_map': None
    }),
    
    'datasets': ['Small', 'Medium'],
    'implementations': ['Big Heap', 'Small Heap'],
    'colours': {
      'Big Heap': 'r',
      'Small Heap': 'b',
    }
  },
  'ap_big_vs_small_relative': {
    'data': 'f_opt_ap_big_vs_small_heap',
    'type': FigureTypes.optimisation_relative,
    'file_filter': OPTIMISATION_FILE_FILTER,
    'test_filter': dictFilter({
      'big': 'Big Heap',
      'small_vector': 'Small Heap',
      'small_map': None
    }),
    
    'datasets': ['Small', 'Medium'],
    'implementations': ['Big Heap', 'Small Heap'],
    'baseline': 'Big Heap',
    'colours': {
      'Big Heap': 'r',
      'Small Heap': 'b',
    }
  },                      
}

### Incremental test cases

# Time, in seconds, *after* proper start of cluster.
# So 600s would be after time 12,000,000,000 in cluster.
DEFAULT_INCREMENTAL_START = 600

# Number of elements to include in moving average (for incremental_over_time)
DEFAULT_WINDOW_SIZE = 100

INCREMENTAL_TEST_FILTER = dictFilter({
  'full': 'Standard',
  'incremental': 'Incremental',
})

INCREMENTAL_FIGURES = {
  'optimised_cdf': {
    'data': 'ion_head_to_head_optimised',
    'type': FigureTypes.incremental_cdf,
    'test_filter': INCREMENTAL_TEST_FILTER,
    
    'trace': 'full_size',
    'implementations': ['Standard', 'Incremental'],
    'colours': {
      'Standard': 'r',
      'Incremental': 'b',
    }
  }, 
  'optimised_hist': {
    'data': 'ion_head_to_head_optimised',
    'type': FigureTypes.incremental_hist,
    'test_filter': INCREMENTAL_TEST_FILTER,
    
    'trace': 'full_size',
    'implementations': ['Standard', 'Incremental'],
    'colours': {
      'Standard': 'r',
      'Incremental': 'b',
    }
  },
  'optimised_over_time': {
    'data': 'ion_head_to_head_optimised',
    'type': FigureTypes.incremental_over_time,
    'test_filter': INCREMENTAL_TEST_FILTER,
    
    'trace': 'full_size',
    'implementations': ['Standard', 'Incremental'],
    'colours': {
      'Standard': 'r',
      'Incremental': 'b',
    }
  },  
}

### Approximate test cases
APPROXIMATE_ACCURACY_THRESHOLD = 90 # percent
APPROXIMATE_NUM_BINS = 10

### All figures

FIGURES = mergeDicts([OPTIMISATION_FIGURES, INCREMENTAL_FIGURES], 
                     ["opt", "inc"])