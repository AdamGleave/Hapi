import os
from enum import Enum

from config.common import *

class FigureTypes():
  optimisation_absolute = 0
  optimisation_relative = 1
  incremental_cdf = 2
  incremental_hist = 3

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
  'clusters/natural/google_trace/octopus/1hour/large_100percent.min': None,
  'clusters/natural/google_trace/octopus/1hour/medium_10percent.min': 'Medium',
  'clusters/natural/google_trace/octopus/1hour/small_1percent.min': 'Small',
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

INCREMENTAL_TEST_FILTER = dictFilter({
  'full': 'Standard',
  'incremental': 'Incremental',
})

INCREMENTAL_FIGURES = {
  'optimised_cdf': {
    'data': 'ion_optimized_head_to_head_quick',
    'type': FigureTypes.incremental_cdf,
    'test_filter': INCREMENTAL_TEST_FILTER,
    
    'trace': 'small_trace',
    'implementations': ['Standard', 'Incremental'],
    'colours': {
      'Standard': 'r',
      'Incremental': 'b',
    }
  }, 
  'optimised_hist': {
    'data': 'ion_optimized_head_to_head_quick',
    'type': FigureTypes.incremental_hist,
    'test_filter': INCREMENTAL_TEST_FILTER,
    
    'trace': 'small_trace',
    'implementations': ['Standard', 'Incremental'],
    'colours': {
      'Standard': 'r',
      'Incremental': 'b',
    }
  }, 
}

### All figures

FIGURES = mergeDicts([OPTIMISATION_FIGURES, INCREMENTAL_FIGURES], 
                     ["opt", "inc"])