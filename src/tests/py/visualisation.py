#!/usr/bin/env python3
 
import config.visualisation as config
from visualisation import parse, gen_optimisation, gen_incremental, gen_approximate
from visualisation.test_types import FigureTypes

import os, sys, re
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib import rc
import matplotlib.pyplot as plt

figure_generators = {
  FigureTypes.optimisation_absolute : gen_optimisation.generate_absolute,
  FigureTypes.optimisation_relative : gen_optimisation.generate_relative,
  FigureTypes.optimisation_scaling_factors : gen_optimisation.generate_scaling_factors,
  FigureTypes.incremental_cdf : gen_incremental.generate_cdf,
  FigureTypes.incremental_only_incremental_cdf : gen_incremental.generate_incremental_only_cdf,
  FigureTypes.incremental_hist : gen_incremental.generate_hist,
  FigureTypes.incremental_over_time : gen_incremental.generate_over_time,
  FigureTypes.approximate_oracle_policy : gen_approximate.generate_oracle_policy_interpolate,
  FigureTypes.approximate_policy_accuracy : gen_approximate.generate_terminating_condition_accuracy_plot,
  FigureTypes.approximate_error_cdf : gen_approximate.generate_terminating_condition_accuracy_distribution,
  FigureTypes.approximate_speed_cdf : gen_approximate.generate_terminating_condition_speed_distribution,
  FigureTypes.approximate_cost_vs_time : gen_approximate.generate_cost_vs_time_plot,
}

LATEX_PREAMBLE = r'\usepackage{siunitx}'
def set_rcs():
  rc('font',**{'family':'serif', 'serif':['Palatino'], 'size':12})
  rc('text', usetex=True)
  rc('text.latex', preamble=LATEX_PREAMBLE)
  rc('legend', fontsize=7)
  rc('figure', figsize=(6,4))
  rc('figure.subplot', left=0.10, top=0.90, bottom=0.12, right=0.95)
  rc('axes', linewidth=0.5)
  rc('lines', linewidth=0.5)
  
#   rc('pgf', preamble=LATEX_PREAMBLE)

if __name__ == "__main__":
  set_rcs()
  
  figure_names = None
  if len(sys.argv) == 1:
    # no arguments
    figure_names = config.FIGURES.keys()
  else:
    # arguments are list of figure patterns
    figure_patterns = sys.argv[1:]
    figure_names = set()
    for pattern_regex in figure_patterns:
      pattern = re.compile(pattern_regex + "$")
      for figure_name in config.FIGURES:
        if pattern.match(figure_name):
          figure_names.add(figure_name)
  print("Generating: ", figure_names)
  figures = {k : config.FIGURES[k] for k in figure_names}
    
  for (figname, figconfig) in figures.items():
    # Import CSV input
    data_name = figconfig['data']
    test_type = data_name.split("_")[0]
    
    file_filter = figconfig.get("file_filter", parse.identity)
    test_filter = figconfig.get("test_filter", parse.identity)
    
    data_fname = os.path.join(config.DATA_ROOT, data_name + ".csv")
    data = None
    if test_type == "f":
      data = parse.full(data_fname, file_filter, test_filter)
    elif test_type == "iof" or test_type == "ihy":
      data = parse.incremental_offline(data_fname, file_filter, test_filter)
    elif test_type == "ion":
      data = parse.incremental_online(data_fname, file_filter, test_filter)
    elif test_type == "af":
      data = parse.approximate_full(data_fname, file_filter)
    elif test_type == "aio" or test_type == "aih":
      data = parse.approximate_full(data_fname, file_filter)
    else:
      assert(False)

    # Process the data, generate the graph
    generate_function = figure_generators[figconfig['type']]
    # new figure
    plt.figure()
    fig = generate_function(data, figconfig)
    
    # Export figure
    figure_fname = os.path.join(config.FIGURE_ROOT, figname + ".pdf")
    with PdfPages(figure_fname) as out:
      out.savefig(fig)
      
#     pgf_figure_fname = os.path.join(config.FIGURE_ROOT, figname + '.pgf')
#     plt.savefig(pgf_figure_fname)
    
    # release memory for current figure 
    plt.close()