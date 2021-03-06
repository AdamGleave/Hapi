import os
from enum import Enum

from config.common import *
from visualisation.test_types import FigureTypes
from matplotlib import rc
import matplotlib.pyplot as plt

def dictFilter(d):
  return lambda k : d.get(k, None)

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
LATEX_PREAMBLE = r'\usepackage{siunitx},\usepackage{relsize}'
def set_rcs_common():
  rc('font',**{'family':'serif', 'serif':['Palatino']})
  rc('text', usetex=True)
  rc('text.latex', preamble=LATEX_PREAMBLE)
  rc('axes', linewidth=0.5)
  rc('lines', linewidth=1)
  #rc('figure.subplot', left=0.10, top=0.90, bottom=0.12, right=0.95)
  #rc('figure.subplot', left=0.10, top=0.90, bottom=0.2, right=0.95)
  rc('figure', autolayout=True)
  rc('legend', frameon=False) # disable border around legend
  
  rc('font', size=10)
  rc('legend', fontsize=8)
  
# LaTeX text width with default margins is 452 pt, or 158.85 cm for TeX
# Rounding down slightly, this gives us 6.25 in 
def set_width(width, aspect_ratio=4/3.0):
  rc('figure', figsize=(width, width/aspect_ratio))

def set_rcs_full():
  set_rcs_common()
  # make it slightly less than 4:3 so can fit two figures top-bottom
  rc('figure', figsize=(6.25, 4))
  
def set_rcs_twocol():
  set_rcs_common()  
  # Slightly less than half of onecol so there's some whitespace between
  set_width(3.1)

def set_rcs_narrow():
  '''used for e.g. single-bar bar charts'''
  set_rcs_common()
  width = 2 
  height = 3.1 * 3/4.0 # same as for twocol
  rc('figure', figsize=(width, height))
  
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
  'clusters/natural/google_trace/quincy/1hour/full_size_first.min': 'Warehouse scale (Quincy)',
  'clusters/natural/google_trace/quincy/1hour/large_first.min': 'Large (Quincy)',
  'clusters/natural/google_trace/quincy/1hour/medium_first.min': 'Medium (Quincy)',
  'clusters/natural/google_trace/quincy/1hour/small_first.min': 'Small (Quincy)',
  'clusters/natural/google_trace/octopus/1hour/full_size_first.min': 'Warehouse scale (Octopus)',
  'clusters/natural/google_trace/octopus/1hour/large_first.min': 'Large (Octopus)',
  'clusters/natural/google_trace/octopus/1hour/medium_first.min': 'Medium (Octopus)',
  'clusters/natural/google_trace/octopus/1hour/small_first.min': 'Small (Octopus)',
})

OCTOPUS_FILE_FILTER = dictFilter({
  'clusters/natural/google_trace/octopus/1hour/full_size_first.min': 'Warehouse scale',
  'clusters/natural/google_trace/octopus/1hour/large_first.min': 'Large',
  'clusters/natural/google_trace/octopus/1hour/medium_first.min': 'Medium',
  'clusters/natural/google_trace/octopus/1hour/small_first.min': 'Small',
})

QUINCY_FILE_FILTER = dictFilter({
  'clusters/natural/google_trace/quincy/1hour/full_size_first.min': 'Warehouse scale',
  'clusters/natural/google_trace/quincy/1hour/large_first.min': 'Large',
  'clusters/natural/google_trace/quincy/1hour/medium_first.min': 'Medium',
  'clusters/natural/google_trace/quincy/1hour/small_first.min': 'Small',
})

FULL_DATASETS = ['Small (Octopus)', 'Medium (Octopus)', 'Large (Octopus)', 'Warehouse scale (Octopus)',
                 'Small (Quincy)', 'Medium (Quincy)', 'Large (Quincy)', 'Warehouse scale (Quincy)',]
ONESHOT_DATASETS = ['Small', 'Medium', 'Large', 'Warehouse scale']

COMPILER_IMPLEMENTATIONS_FULL = {
  'clang_debug': 'Clang Debug',
  'clang_O0': 'Clang O0 (unoptimised)',
  'clang_O1': 'Clang O1',
  'clang_O2': 'Clang O2',
  'clang_O3': 'Clang O3',
  'gcc_debug': 'GCC Debug',
  'gcc_O0': 'GCC O0 (unoptimised)',
  'gcc_O1': 'GCC O1',
  'gcc_O2': 'GCC O2',
  'gcc_O3': 'GCC O3',
}

COMPILER_LEVELS = ['O0 (unoptimised)', 'O1', 'O2', 'O3']
COMPILER_LEVEL_COLOURS = {
  'O0 (unoptimised)': 'r', 
  'O1': 'g',
  'O2': 'b',
  'O3': 'k'
}
COMPILER_COMPILERS = ['GCC', 'Clang']


def compiler_implementations(name):
  return {name + '_' + k : v for k, v in COMPILER_IMPLEMENTATIONS_FULL.items()}

# TODO: datasets, implementations, colours?
# TODO: baseline doesn't really make sense, maybe need new class
COMPILER_FIGURES = {
  'ap': {
    'data': 'f_compilers_ap',
    'dataset': 'Medium',
  },
  'cc': {
    'data': 'f_compilers_cc',
    'dataset': 'Mini',
    'file_filter': dictFilter({'clusters/natural/google_trace/quincy/1hour/small_last.min': 'Mini'})
  },
  'cs': {
    'data': 'f_compilers_cs',
    'dataset': 'Medium',
  },
  'cs_with_legend': {
    'data': 'f_compilers_cs',
    'test_filter': dictFilter(compiler_implementations('cs')),
    'dataset': 'Medium',
    'legend_loc': 'upper right',
  },
  'relax': {
    'data': 'f_compilers_relax',
    'dataset': 'Large',
  },
  # others implementations
  'goldberg': {
    'data': 'f_compilers_cs_goldberg',
    'dataset': 'Warehouse scale',
  },       
  'frangioni': {
    'data': 'f_compilers_relax_frangioni',
    'dataset': 'Medium',
  },
}

def apply_compiler_defaults(d):
  for k, v in d.items():
    if 'file_filter' not in v:
      v['file_filter'] = QUINCY_FILE_FILTER
    if 'test_filter' not in v:
      filter_dict = compiler_implementations(k)
      v['test_filter'] = dictFilter(filter_dict)
    if 'compilers' not in v:
      v['compilers'] = COMPILER_COMPILERS
      v['levels'] = COMPILER_LEVELS
      v['colours'] = COMPILER_LEVEL_COLOURS
      
apply_compiler_defaults(COMPILER_FIGURES)

def updateCompilerFigures(d):
  types = {'compiler': ([], FigureTypes.optimisation_compilers)}
  return updateFiguresWithTypes(d, types)

COMPILER_FIGURES = updateCompilerFigures(COMPILER_FIGURES)

### Optimisation figures

OPTIMISATION_FIGURES = {
  ### Optimisations
  ## Augmenting path
  'ap': {
    'data': 'f_opt_ap',
    'test_filter': dictFilter({
      'fulldjikstra_big': 'Standard Djikstra, big heap',
      'fulldjikstra_small': 'Standard Djikstra, small heap',
      'partialdjikstra_big': 'Partial Djikstra, big heap',
      'partialdjikstra_small_vector': 'Partial Djikstra, small heap (array)',
      'partialdjikstra_small_map': 'Partial Djikstra, small heap (hash table)', 
    }),
    'datasets': ['Small', 'Medium'],
    'implementations': ['Standard Djikstra, small heap', 
                        'Partial Djikstra, big heap',  
                        'Partial Djikstra, small heap (hash table)',
                        'Partial Djikstra, small heap (array)'],
    'baseline': 'Standard Djikstra, big heap',
    'colours': {
      'Standard Djikstra, small heap': 'r',
      'Partial Djikstra, big heap': 'g',
      'Partial Djikstra, small heap (hash table)': 'b',
      'Partial Djikstra, small heap (array)': 'k',
    },
    'legend': 'upper right',
  },
  'cs_wave_vs_fifo': {
    'data': 'f_opt_cs_wave_vs_fifo',
    'test_filter': dictFilter({
      'wave': 'Wave',
      'fifo': 'FIFO',
    }),
    
    'datasets': ['Small', 'Medium'],
    'implementations': ['FIFO'],
    'baseline': 'Wave',
    'colours': {
      'Wave': 'r',
      'FIFO': 'b', 
    },
    'legend': None,
  },
  # TODO: This is gonna need some careful formatting
  # TODO: Octopus is a somewhat bogus model for this, since you reach optimality
  # so early.
  'relax_arc_cache_quincy': {
    'data': 'f_opt_relax_cache_arcs_quincy',
    'test_filter': dictFilter({'none': 'No cache',
                               'all': 'Cache arcs'}),
    
    # TODO: Boost timeout, get data for 'Medium' too
    'datasets': ['Medium', 'Large', 'Warehouse scale'],
    'implementations': ['Cache arcs'],
    'baseline': 'No cache',
    'colours': { 
      'Cache arcs': 'b',
    },
    'legend': None,
  },
  'relax_arc_cache_octopus': {
    'data': 'f_opt_relax_cache_arcs_octopus',
    'test_filter': dictFilter({'none': 'No cache',
                               'all': 'Cache arcs'}),
    'file_filter': OCTOPUS_FILE_FILTER,
    'datasets': ['Small'],
    'implementations': ['Cache arcs'],
    'baseline': 'No cache',
    'colours': {
      'Cache arcs': 'b',
    },
    'legend': None,
    'appearance': extend_dict(DEFAULT_APPEARANCE, {'narrow': set_rcs_narrow}),
  },
#   'parser_set_vs_getarc': {
#     'data': 'f_opt_parser_set_vs_getarc',
#     'test_filter': dictFilter({'getarc': 'Linked list',
#                                'set': 'Set'}),
#                            
#     'datasets': ['Large', 'Warehouse scale'],
#     'implementations': ['Set'],
#     'baseline': 'Linked list',
#     'colours': {
#       'Linked list': 'r',
#       'Set': 'b',
#     }
#   },
}

def apply_optimisation_defaults(d):
  for k, v in d.items():
    if 'file_filter' not in v:
      v['file_filter'] = QUINCY_FILE_FILTER
      if 'datasets' not in v:
        v['datasets'] = ONESHOT_DATASETS
        
apply_optimisation_defaults(OPTIMISATION_FIGURES)

def updateOptimisationFigures(d):
  types = {#'absolute': ([], FigureTypes.optimisation_absolute),
           'relative': ([], FigureTypes.optimisation_relative)}
  return updateFiguresWithTypes(d, types)

OPTIMISATION_FIGURES = updateOptimisationFigures(OPTIMISATION_FIGURES)

SCALING_FACTORS = list(range(2,9)) + [10] + list(range(12,32,4))
OPTIMISATION_FIGURES['cs_scaling_factor'] = {
  'data': 'f_opt_cs_scaling_factor',
  'type': FigureTypes.optimisation_scaling_factors,
  'file_filter': QUINCY_FILE_FILTER,
  'test_filter': dictFilter({str(k): str(k) for k in range(2,32)}),
  
  'dataset': 'Medium',
  'implementations': [str(k) for k in SCALING_FACTORS],
  'colours': ['b'],
  'baseline': '2',
}

OPTIMISATION_FIGURES['cs_goldberg_scaling_factor'] = {
  'data': 'f_opt_cs_goldberg_scaling_factor',
  'type': FigureTypes.optimisation_scaling_factors,
  'file_filter': QUINCY_FILE_FILTER,
  'test_filter': dictFilter({str(k): str(k) for k in range(2,32)}),
   
  'dataset': 'Medium',
  'implementations': [str(k) for k in SCALING_FACTORS],
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
  'full': 'From scratch',
  'incremental': 'Incremental',
})

INCREMENTAL_FIGURES = {
  # TODO: bigger datasets?
  'same_ap': {
    'data': 'ion_same_ap',
    'trace': 'medium',
    'annotate_means': {
      'From scratch': {'horizontalalignment': 'right', 'xytext': (-1, 0)},
      'Incremental': None,
    },
  },
  'same_relax': {
    'data': 'ion_same_relax',
    'trace': ['medium', 'large'],
    'annotate_means': {
      'From scratch': {'horizontalalignment': 'right', 'xytext': (-1, 0)},
      'Incremental': None,
    },
  },
  'same_relaxf': {
    'data': 'ion_same_relaxf',
    'trace': ['medium', 'large', 'full_size'],
    'window_size': '60s',
  },
  # TODO: inc_ap works OK on full_size, too
  'head_to_head_my': {
    'data': 'ion_head_to_head_my',
    'trace': ['medium', 'large', 'full_size'],
    'window_size': '5s',
    'test_filter': dictFilter({'full': 'Cost scaling (from scratch)', 
                               'inc_ap': 'SSP (incremental)',
                               'inc_relax': 'Relaxation (incremental)'}),
    'implementations': ['Cost scaling (from scratch)',
                       'SSP (incremental)'],#, 'Relaxation (incremental)'],
    'means': ['Cost scaling (from scratch)', 'SSP (incremental)'],
    'incremental_implementation': 'SSP (incremental)',
    'colours': {'Cost scaling (from scratch)': 'r',
                'SSP (incremental)': 'b',
                'Relaxation (incremental)': 'g'},
    'annotate_means': {
      'Cost scaling (from scratch)': {'y_loc': 0.5, 
                                      'xytext': (-1, 0),
                                      'horizontalalignment': 'right'},
      'SSP (incremental)': {'y_loc': 0.5 }
    }
  },
  'head_to_head_optimised': {
    'data': 'ion_head_to_head_optimised',
    'trace': ['medium', 'large', 'full_size'],
    'window_size': '5s',
    'test_filter': dictFilter({'full': 'CS2 (from scratch)', 
                               'incremental': 'RelaxIV (incremental)'}),
    'implementations': ['CS2 (from scratch)', 'RelaxIV (incremental)'],
    'means': ['CS2 (from scratch)', 'RelaxIV (incremental)'],
    'incremental_implementation': 'RelaxIV (incremental)',
    'target_latency': 1.0,
    'target_latency_min_prob': 90,
    'target_latency_max_latency': 6.0,
    'colours': {'CS2 (from scratch)': 'r',
                'RelaxIV (incremental)': 'b'},
    'annotate_means': {
      'CS2 (from scratch)': None,
      'RelaxIV (incremental)': {'y_loc': 0.3, 'xytext': (3, 0)},
    }
  },
#   'head_to_head_merged': {
#     'data': 'ion_head_to_head_merged',
#     'trace': 'full_size',
#     'window_size': '5s',
#     'test_filter': dictFilter({'full': 'Standard cost scaling',
#                                'inc_ap': 'My incremental augmenting path',
#                                'inc_relax': 'My incremental relaxation', 
#                                'incremental': 'Incremental relaxation (Frangioni)'}),
#     'implementations': ['Standard cost scaling', 'My incremental augmenting path',
#                         'My incremental relaxation', 'Incremental relaxation (Frangioni)'],
#     'means': ['Standard cost scaling', 'My incremental augmenting path',
#                'My incremental relaxation', 'Incremental relaxation (Frangioni)'],
#     'incremental_implementation': 'Incremental relaxation (Frangioni)',
#     'colours': {'Standard cost scaling': 'r',
#                 'My incremental augmenting path': 'g',
#                 'My incremental relaxation': 'b',
#                 'Incremental relaxation (Frangioni)': 'k',
#                },
#   },
} 

def applyIncrementalDefault(d):
  for k, v in d.items():
    if 'test_filter' not in v:
      v['test_filter'] = INCREMENTAL_TEST_FILTER
    if 'implementations' not in v:
      v['implementations'] = ['From scratch', 'Incremental']
      if 'means' not in v:
        v['means'] = ['From scratch']
      if 'incremental_implementation' not in v:
        v['incremental_implementation'] = 'Incremental'
        
    if 'colours' not in v:
      v['colours'] = {
        'From scratch': 'r',
        'Incremental': 'b',
      }

def incrementalExpandConfig(d):
  new_d = {}
  for k, v in d.items():
    if type(v['trace']) == list:
      for trace in v['trace']:
        new_v = v.copy()
        new_v['trace'] = trace
        new_d[k + "_" + trace] = new_v
    else:
      new_d[k] = v
  return new_d
        
applyIncrementalDefault(INCREMENTAL_FIGURES)
INCREMENTAL_FIGURES = incrementalExpandConfig(INCREMENTAL_FIGURES)

def updateIncrementalFigures(d):
  types = {'cdf': ([], FigureTypes.incremental_cdf),
           'incremental_only_cdf': (['incremental_implementation'], FigureTypes.incremental_only_incremental_cdf),
           'incremental_only_target_latency_cdf': (['incremental_implementation', 'target_latency', 'target_latency_min_prob'], FigureTypes.incremental_only_incremental_target_latency_cdf),
           #'hist': ([], FigureTypes.incremental_hist),
           #'over_time': ([], FigureTypes.incremental_over_time)
           }
  return updateFiguresWithTypes(d, types)

INCREMENTAL_FIGURES = updateIncrementalFigures(INCREMENTAL_FIGURES)

### Approximate test cases
APPROXIMATE_ACCURACY_THRESHOLD = 98 # percent
APPROXIMATE_TARGET_ACCURACY = 99
APPROXIMATE_NUM_BINS = 10

APPROXIMATE_MAX_COST_PARAMETER = 0.10
APPROXIMATE_MAX_TASK_ASSIGNMENTS_PARAMETER = 15000

# N.B. List of (key,value) pairs rather than dict so order can be specified
APPROXIMATE_DEFAULT_PERCENTILES = [
  (1, '$1^{\mathrm{st}}$ percentile', '#0000CD'), 
  (5, '$5^{\mathrm{th}}$ percentile', '#568203'),
  (25, '$25^{\mathrm{th}}$ percentile', '#AE0C00'),
  (50, 'Median', '#00FFFF'),
]
# Which percentile is the heuristic computed at?
APPROXIMATE_ACCURACY_AT_PERCENTILE = 1

APPROXIMATE_FIGURES = {
#   'road_general': {
#     'data': 'af_road',
#     'training': 10,
#     'test': 10,
#   },
#   'road_paths': {
#     'data': 'af_road_paths',
#     'training': 2,
#     'test': 3,
#     'max_cost_parameter': 0.2,
#   },
#   'road_flow': {
#     'data': 'af_road_flow',
#     'training': 2,
#     'test': 3,
#     'max_cost_parameter': 0.2,
#   },
  'netgen_8': {
    'data': 'af_netgen_8',
    'training': 250,
    'test': 750,
    'min_accuracy_terminating_condition': 96,
    'max_cost_parameter': 30,
    'parameters_legend': {'cost': 'lower left'},
    'speed_legend': 'lower right',
    'speed_annotate_means': {'Cost convergence': None},
  },
  'netgen_sr': {
    'data': 'af_netgen_sr',
    'training': 250,
    'test': 750,
    'min_accuracy_terminating_condition': 96,
    'max_cost_parameter': 25,
    'parameters_legend': None,
    'speed_legend': None,
    'speed_annotate_means': {'Cost convergence': None},
  },
  'netgen_lo_8': {
    'data': 'af_netgen_lo_8',
    'training': 250,
    'test': 750,
    'max_cost_parameter': 100,
    'parameters_legend': None,
    'speed_legend': None,
    'speed_annotate_means': {'Oracle': None, 
                             'Cost convergence': {'y_loc': 0.6,
                                                  'horizontalalignment': 'right',
                                                  'xytext': (-3, 0)}},
  },
  'netgen_lo_sr': {
    'data': 'af_netgen_lo_sr',
    'training': 250,
    'test': 750,
    'max_cost_parameter': 90,
    'parameters_legend': None,
    'speed_legend': None,
    'speed_annotate_means': {'Oracle': None, 
                             'Cost convergence': {'y_loc': 0.6,
                                                  'horizontalalignment': 'right',
                                                  'xytext': (-3, 0)}},
  },
  'goto_8': {
    'data': 'af_goto_8',
    'training': 250,
    'test': 750,
    'max_cost_parameter': 60,
    'parameters_legend': None,
    'speed_legend': None,
    'speed_annotate_means': {'Cost convergence': {'xytext': (3,0)}},
  },
  'goto_sr': {
    'data': 'af_goto_sr',
    'training': 250,
    'test': 750,
    'max_cost_parameter': 80,
    'parameters_legend': None,
    'speed_legend': None,
    'speed_annotate_means': {'Cost convergence': {'xytext': (3,0)}},
  },
  'quincy_medium': {
    'data': 'af_quincy_medium',
    'training': 250,
    'test': 750,
    'min_accuracy_terminating_condition': 0,
    'max_cost_parameter': 5,
    'max_task_assignments_parameter': 8000,
    'title': False,
    'over_time_file': 'clusters/natural/google_trace/approx_quincy/medium/217.min',
    'parameters_legend': {'cost': 'upper right', 
                          'task_assignments': 'lower left'}
  },
}

def updateApproximateFigures(d):
  types = {'oracle_policy': ([], FigureTypes.approximate_oracle_policy),
           'over_time': (['over_time_file'], FigureTypes.approximate_cost_vs_time),
           'policy': ([], FigureTypes.approximate_policy_combined)}
           
  return updateFiguresWithTypes(d, types)
  
APPROXIMATE_FIGURES = updateApproximateFigures(APPROXIMATE_FIGURES)

### All figures

FIGURES = merge_dicts([OPTIMISATION_FIGURES,
                      COMPILER_FIGURES,
                      INCREMENTAL_FIGURES,
                      APPROXIMATE_FIGURES],
                     ["opt", 'com', "inc", "app"], sep='/')
