import os
from enum import Enum

class FigureTypes():
  optimisation = 0

# SCRIPT_ROOT = PROJECT_ROOT/src/tests/visualisation
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_ROOT)))

DOC_PREFIX = "doc"
DATA_PREFIX = "data"
FIGURE_PREFIX = "figures"
DOC_ROOT = os.path.join(PROJECT_ROOT, DOC_PREFIX)
DATA_ROOT = os.path.join(DOC_ROOT, DATA_PREFIX)
FIGURE_ROOT = os.path.join(DOC_ROOT, FIGURE_PREFIX)

FIGURES = {
  ### Optimisations
  ## Augmenting path
  'opt_ap_big_vs_small': {
    'data': 'f_ap_big_vs_small_heap',
    'figure_type': FigureTypes.optimisation,
  },
}