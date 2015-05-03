import os
from enum import Enum

from config.common import *
from visualisation.test_types import FigureTypes
from matplotlib import rc

def dictFilter(d):
  return lambda k : d[k]

def updateFiguresWithTypes(d, types):
  res = {}
  for type_name, (preconditions, type) in types.items():
    d_for_type = {}
    for k, v in d.items():
      preconditions_satisfied = all([p in v for p in preconditions])
      if preconditions_satisfied:
        v = v.copy()
        v.update({'type': type})
        d_for_type[k + '_' + type_name] = v 
    res.update(d_for_type)
  return res

### Appearance
LATEX_PREAMBLE = r'\usepackage{siunitx}'
def set_rcs_common():
  rc('font',**{'family':'serif', 'serif':['Palatino']})
  rc('text', usetex=True)
  rc('text.latex', preamble=LATEX_PREAMBLE)
  rc('axes', linewidth=0.5)
  rc('lines', linewidth=0.5)
  #rc('figure.subplot', left=0.10, top=0.90, bottom=0.12, right=0.95)
  #rc('figure.subplot', left=0.10, top=0.90, bottom=0.2, right=0.95)
  rc('figure', autolayout=True)
  
def set_rcs_full():
  set_rcs_common()
  
  rc('font', size=12)
  rc('legend', fontsize=7)
  rc('figure', figsize=(6,4))
  
def set_rcs_twocol():
  set_rcs_common()
  
  rc('font', size=8)
  rc('legend', fontsize=7)
  rc('figure', figsize=(3.33, 2.22))
  
DEFAULT_APPEARANCE = {
  '1col': set_rcs_full,
  '2col': set_rcs_twocol,
}

### Miscellaneous settings
CONFIDENCE_LEVEL = 0.95

### Paths

DOC_PREFIX = "doc"
DATA_PREFIX = "data"
FIGURE_PREFIX = "figures"
DOC_ROOT = os.path.join(PROJECT_ROOT, DOC_PREFIX)
DATA_ROOT = os.path.join(DOC_ROOT, DATA_PREFIX)
FIGURE_ROOT = os.path.join(DOC_ROOT, FIGURE_PREFIX)

### Compiler test cases

FULL_FILE_FILTER = dictFilter({
  'clusters/natural/google_trace/octopus/1hour/full_size.min': 'Warehouse Scale',
  'clusters/natural/google_trace/octopus/1hour/large.min': 'Large',
  'clusters/natural/google_trace/octopus/1hour/medium.min': 'Medium',
  'clusters/natural/google_trace/octopus/1hour/small.min': 'Small',
})

FULL_DATASETS = ['Small', 'Medium', 'Large', 'Warehouse Scale']

COMPILER_IMPLEMENTATIONS_FULL = {
  'clang_debug': 'Clang Debug',
  'clang_O0': 'Clang Unoptimised',
  'clang_O1': 'Clang O1',
  'clang_O2': 'Clang O2',
  'clang_O3': 'Clang O3',
  'gcc_debug': 'GCC Debug',
  'gcc_O0': 'GCC Unoptimised',
  'gcc_O1': 'GCC O1',
  'gcc_O2': 'GCC O2',
  'gcc_O3': 'GCC O3',
}

COMPILER_IMPLEMENTATIONS_DEFAULT = ['clang_O2', 'clang_O3', 'gcc_O2', 'gcc_O3']

COMPILER_IMPLEMENTATIONS_COLORS = {
  'Clang O2': 'r',
  'Clang O3': 'g',
  'GCC O2': 'b',
  'GCC O3': 'k',
}

def compiler_implementations(l, name):
  return {name + '_' + k : v if k in l else None 
          for k, v in COMPILER_IMPLEMENTATIONS_FULL.items()}

# TODO: datasets, implementations, colours?
# TODO: baseline doesn't really make sense, maybe need new class
COMPILER_FIGURES = {
  'ap': {
    'data': 'f_compilers_ap',
    'datasets': ['Small', 'Medium']
  },
  'cc': {
    'data': 'f_compilers_cc',
    'datasets': ['Small', 'Medium']
  },
  'cs': {
    'data': 'f_compilers_cs',
    'datasets': ['Large', 'Warehouse Scale']
  },
  'relax': {
    'data': 'f_compilers_relax',
    'datasets': ['Small']
  },
  # others implementations
  'goldberg': {
    'data': 'f_compilers_cs_goldberg',
    'datasets': ['Large', 'Warehouse Scale']
  },       
  'frangioni': {
    'data': 'f_compilers_relax_frangioni',
    'datasets': ['Medium', 'Large']
  },
}

def apply_compiler_defaults(d):
  for k, v in d.items():
    if 'file_filter' not in v:
      v['file_filter'] = FULL_FILE_FILTER
      if 'datasets' not in v:
        v['datasets'] = FULL_DATASETS
    if 'test_filter' not in v:
      filter_dict = compiler_implementations(COMPILER_IMPLEMENTATIONS_DEFAULT, k)
      v['test_filter'] = dictFilter(filter_dict)
      assert('implementations' not in v)
      v['implementations'] = [COMPILER_IMPLEMENTATIONS_FULL[k] 
                              for k in COMPILER_IMPLEMENTATIONS_DEFAULT]
      v['colours'] = COMPILER_IMPLEMENTATIONS_COLORS
      
apply_compiler_defaults(COMPILER_FIGURES)

def updateCompilerFigures(d):
  types = {'absolute': ([], FigureTypes.optimisation_absolute)}
  return updateFiguresWithTypes(d, types)

COMPILER_FIGURES = updateCompilerFigures(COMPILER_FIGURES)

### Optimisation figures

OPTIMISATION_FIGURES = {
  ### Optimisations
  ## Augmenting path
  'ap_big_vs_small': {
    'data': 'f_opt_ap_big_vs_small_heap',
    'test_filter': dictFilter({
      'big': 'Big heap',
      'small_vector': 'Small heap with vector',
      'small_map': 'Small heap with map',
    }),
    
    'datasets': ['Small', 'Medium'],
    'implementations': ['Small heap with vector', 'Small heap with map'],
    'baseline': 'Big heap',
    'colours': {
      'Big heap': 'r',
      'Small heap with vector': 'g',
      'Small heap with map': 'b',
    }
  },
  'ap_full_vs_partial': {
    'data': 'f_opt_ap_full_vs_partial_djikstra',
    'test_filter': dictFilter({
      'full': 'Full',
      'partial': 'Partial'
    }),
                         
    'datasets': ['Small', 'Medium'],
    'implementations': ['Partial'],
    'baseline': 'Full',
    'colours': {
      'Full': 'r',
      'Partial': 'b',
    }
  },
  'cs_wave_vs_fifo': {
    'data': 'f_opt_cs_wave_vs_fifo',
    'test_filter': dictFilter({
      'wave': 'Wave',
      'fifo': 'FIFO',
    }),
    
    'datasets': ['Small', 'Medium', 'Large'],
    'implementations': ['FIFO'],
    'baseline': 'Wave',
    'colours': {
      'Wave': 'r',
      'FIFO': 'b', 
    }
  },
  # TODO: This is gonna need some careful formatting
  # TODO: Octopus is a somewhat bogus model for this, since you reach optimality
  # so early.
  'relax_arc_cache': {
    'data': 'f_opt_relax_cache_arcs',
    'test_filter': dictFilter({'none': 'No caching',
                               'cache_zerorc': 'Cache zero cost arcs',
                               'cache_all': 'Cache all arcs'}),
    
    # TODO: Boost timeout, get data for 'Medium' too
    'datasets': ['Small'],
    'implementations': ['Cache zero cost arcs', 'Cache all arcs'],
    'baseline': 'No caching',
    'colours': {
      'No caching': 'r', 
      'Cache zero cost arcs': 'g',
      'Cache all arcs': 'b',
    }
  },
  'parser_set_vs_getarc': {
    'data': 'f_opt_parser_set_vs_getarc',
    'test_filter': dictFilter({'getarc': 'Linked list',
                               'set': 'Set'}),
                           
    'datasets': ['Large', 'Warehouse Scale'],
    'implementations': ['Set'],
    'baseline': 'Linked list',
    'colours': {
      'Linked list': 'r',
      'Set': 'b',
    }
  },
}

def apply_optimisation_defaults(d):
  for k, v in d.items():
    if 'file_filter' not in v:
      v['file_filter'] = FULL_FILE_FILTER
      if 'datasets' not in v:
        v['datasets'] = FULL_DATASETS
        
apply_optimisation_defaults(OPTIMISATION_FIGURES)

def updateOptimisationFigures(d):
  types = {'absolute': ([], FigureTypes.optimisation_absolute),
           'relative': ([], FigureTypes.optimisation_relative)}
  return updateFiguresWithTypes(d, types)

OPTIMISATION_FIGURES = updateOptimisationFigures(OPTIMISATION_FIGURES)

OPTIMISATION_FIGURES['cs_scaling_factor'] = {
  'data': 'f_opt_cs_scaling_factor',
  'type': FigureTypes.optimisation_scaling_factors,
  'file_filter': FULL_FILE_FILTER,
  'test_filter': dictFilter({str(k): str(k) for k in range(2,32)}),
  
  'dataset': 'Warehouse Scale',
  'implementations': [str(k) for k in range(2,32)],
  'colours': ['b'],
  'baseline': '2',
}

OPTIMISATION_FIGURES['cs_goldberg_scaling_factor'] = {
  'data': 'f_opt_cs_goldberg_scaling_factor',
  'type': FigureTypes.optimisation_scaling_factors,
  'file_filter': FULL_FILE_FILTER,
  'test_filter': dictFilter({str(k): str(k) for k in range(2,32)}),
  
  'dataset': 'Warehouse Scale',
  'implementations': [str(k) for k in range(2,32)],
  'colours': ['b'],
  'baseline': '12',
}

### Incremental test cases

# Time, in seconds, *after* proper start of cluster.
# So 600s would be after time 12,000,000,000 in cluster.
DEFAULT_INCREMENTAL_START = 600

# Number of elements to include in moving average (for incremental_over_time)
# s for seconds, p for points
DEFAULT_WINDOW_SIZE = '5p'

INCREMENTAL_TEST_FILTER = dictFilter({
  'full': 'Standard',
  'incremental': 'Incremental',
})

INCREMENTAL_FIGURES = {
  # TODO: bigger datasets?
  'same_ap': {
    'data': 'ion_same_ap',
    'trace': 'medium',
  },
  'same_relax': {
    'data': 'ion_same_relax',
    'trace': 'small',
  },
  'same_relaxf': {
    'data': 'ion_same_relaxf',
    'trace': 'medium',
    'window_size': '60s',
  },
  # TODO: inc_ap works OK on full_size, too
  'head_to_head_my': {
    'data': 'ion_head_to_head_my',
    'trace': 'large',
    'window_size': '5s',
    'test_filter': dictFilter({'full': 'Standard cost scaling', 
                               'inc_ap': 'Incremental augmenting path',
                               #'inc_relax': 'Incremental relaxation'
                               'inc_relax': None}),
    'implementations': ['Standard cost scaling',
                       'Incremental augmenting path'],#, 'Incremental relaxation'],
    'incremental_implementation': 'Incremental augmenting path',
    'colours': {'Standard cost scaling': 'r',
                'Incremental augmenting path': 'b',
                'Incremental relaxation': 'g'},
  },
  'head_to_head_optimised': {
    'data': 'ion_head_to_head_optimised',
    'trace': 'full_size',
    'window_size': '5s',
    'test_filter': dictFilter({'full': 'Standard cost scaling', 
                               'incremental': 'Incremental relaxation'}),
    'implementations': ['Standard cost scaling', 'Incremental relaxation'],
    'incremental_implementation': 'Incremental relaxation',
    'colours': {'Standard cost scaling': 'r',
                'Incremental relaxation': 'b'},
  },
} 

def applyIncrementalDefault(d):
  for k, v in d.items():
    if 'test_filter' not in v:
      v['test_filter'] = INCREMENTAL_TEST_FILTER
    if 'implementations' not in v:
      v['implementations'] = ['Standard', 'Incremental']
      if 'incremental_implementation' not in v:
        v['incremental_implementation'] = 'Incremental'
    if 'colours' not in v:
      v['colours'] = {
        'Standard': 'r',
        'Incremental': 'b',
      }
        
applyIncrementalDefault(INCREMENTAL_FIGURES)

def updateIncrementalFigures(d):
  types = {'cdf': ([], FigureTypes.incremental_cdf),
           'incremental_only_cdf': (['incremental_implementation'], FigureTypes.incremental_only_incremental_cdf),
           'hist': ([], FigureTypes.incremental_hist),
           'over_time': ([], FigureTypes.incremental_over_time)}
  return updateFiguresWithTypes(d, types)

INCREMENTAL_FIGURES = updateIncrementalFigures(INCREMENTAL_FIGURES)

### Approximate test cases
APPROXIMATE_ACCURACY_THRESHOLD = 90 # percent
APPROXIMATE_NUM_BINS = 10

APPROXIMATE_MAX_COST_PARAMETER = 0.10
APPROXIMATE_MAX_TASK_ASSIGNMENTS_PARAMETER = 20

APPROXIMATE_DEFAULT_PERCENTILES = {
  1: '$1^{\mathrm{st}}$ percentile', 
  5: '$5^{\mathrm{th}}$ percentile',
  25: '$25^{\mathrm{th}}$ percentile',
  50: 'Median',
}

APPROXIMATE_FIGURES = {
  'road': {
    'data': 'af_road',
    
    # Used by oracle policy and policy accuracy-parameter plot
    #'min_accuracy': can be used to override APPROXIMATE_ACCURACY_THRESHOLD
    
    # Used by policy accuracy-parameter plot, policy error and speed CDF generators 
    'condition': 'cost', # or 'task_assignments'
    
    # Used by policy accuracy-parameter plot
    #'max_cost_parameter': can be used to override APPROXIMATE_MAX_COST_PARAMETER
    #'max_task_assignments_parameter': can be used to override APPROXIMATE_MAX_TASK_ASSIGNMENTS_PARAMETER
    #'percentiles': can be used to override APPROXIMATE_DEFAULT_PERCENTILES
    
    # Used by policy error and speed CDF generators
    'heuristic_parameter': 0.025, # threshold used by heuristic
    'target_accuracy': 99, # in percent. Used for comparison
    
    # Used by over_time generator only
    'file': 'general/natural/road/road_flow_03_NH_e.min',
  }
}

def updateApproximateFigures(d):
  types = {'oracle_policy': ([], FigureTypes.approximate_oracle_policy),
               'policy_parameter_accuracy': (['condition'], FigureTypes.approximate_policy_accuracy),
               'policy_error_cdf': (['condition', 'heuristic_parameter', 'target_accuracy'],
                                    FigureTypes.approximate_error_cdf),
               'policy_speed_cdf': (['condition', 'heuristic_parameter', 'target_accuracy'],
                                    FigureTypes.approximate_speed_cdf),
               'over_time': (['file'], FigureTypes.approximate_cost_vs_time)}
  return updateFiguresWithTypes(d, types)
  
APPROXIMATE_FIGURES = updateApproximateFigures(APPROXIMATE_FIGURES)

### All figures

FIGURES = mergeDicts([OPTIMISATION_FIGURES,
                      COMPILER_FIGURES,
                      INCREMENTAL_FIGURES,
                      APPROXIMATE_FIGURES],
                     ["opt", 'com', "inc", "app"])