import os
from enum import Enum

from config.common import *

class FigureTypes():
  optimisation_absolute = 0
  optimisation_relative = 1

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
  'clusters/synthetic/firmament/graph_100m_8j_100t_10p.in': 'Small',
  'clusters/synthetic/firmament/graph_100m_16j_100t_10p.in': 'Medium',
  'clusters/synthetic/firmament/graph_1000m_32j_100t_10p.in': 'Large',
  'clusters/synthetic/firmament/google_all.in': None, 
})

OPTIMISATION_FILE_FILTER = dictFilter({
  'clusters/synthetic/firmament/graph_100m_8j_100t_10p.in': 'Small',
  'clusters/synthetic/firmament/graph_100m_16j_100t_10p.in': 'Medium',
  'clusters/synthetic/firmament/graph_1000m_32j_100t_10p.in': 'Large',
  'clusters/natural/google_trace/google_all.in': None, 
})

OPTIMISATION_FIGURES = {
  ### Optimisations
  ## Augmenting path
  'ap_big_vs_small_absolute': {
    'data': 'f_ap_big_vs_small_heap',
    'type': FigureTypes.optimisation_absolute,
    'file_filter': OPTIMISATION_FILE_FILTER,
    'test_filter': dictFilter({
      'big': 'Big Heap',
      'small_vector': 'Small Heap',
      'small_map': None
    }),
    
    'datasets': ['Small', 'Medium', 'Large'],
    'implementations': ['Big Heap', 'Small Heap'],
    'colours': {
      'Big Heap': 'r',
      'Small Heap': 'b',
    }
  },
  'ap_big_vs_small_relative': {
    'data': 'f_ap_big_vs_small_heap',
    'type': FigureTypes.optimisation_relative,
    'file_filter': OPTIMISATION_FILE_FILTER,
    'test_filter': dictFilter({
      'big': 'Big Heap',
      'small_vector': 'Small Heap',
      'small_map': None
    }),
    
    'datasets': ['Small', 'Medium', 'Large'],
    'implementations': ['Big Heap', 'Small Heap'],
    'baseline': 'Big Heap',
    'colours': {
      'Big Heap': 'r',
      'Small Heap': 'b',
    }
  },                      
}

### All figures

FIGURES = mergeDicts([OPTIMISATION_FIGURES], 
                     ["opt"])