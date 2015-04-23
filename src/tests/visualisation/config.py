import os
from enum import Enum

class FigureTypes():
  optimisation_absolute = 0
  optimisation_relative = 1

def dictFilter(d):
  return lambda k : d[k]

### Miscellaneous settings
CONFIDENCE_LEVEL = 0.95

### Paths

# SCRIPT_ROOT = PROJECT_ROOT/src/tests/visualisation
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_ROOT)))

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

# For merging dictionaries, and tagging elements
def mergeDicts(dicts, prefix):
  assert(len(dicts) == len(prefix))
      
  result = {}
  for (dict,prefix) in zip(dicts,prefix):
    new_dict = {k : v.copy() for (k,v) in dict.items()}
    new_dict = {prefix + "_" + k : v for (k,v) in new_dict.items()}
    result.update(new_dict)
  
  return result

FIGURES = mergeDicts([OPTIMISATION_FIGURES], 
                     ["opt"])