#!/usr/bin/env python3
 
import config.visualisation as config
from visualisation import parse, gen_optimisation, \
                          gen_incremental, gen_approximate
from visualisation.test_types import FigureTypes

import os, sys, re
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib
import matplotlib.pyplot as plt

figure_generators = {
  FigureTypes.optimisation_absolute : gen_optimisation.generate_absolute,
  FigureTypes.optimisation_relative : gen_optimisation.generate_relative,
  FigureTypes.optimisation_scaling_factors : gen_optimisation.generate_scaling_factors,
  FigureTypes.optimisation_compilers : gen_optimisation.generate_compiler,
  FigureTypes.incremental_cdf : gen_incremental.generate_cdf,
  FigureTypes.incremental_only_incremental_cdf : gen_incremental.generate_incremental_only_cdf,
  FigureTypes.incremental_only_incremental_target_latency_cdf : \
                   gen_incremental.generate_incremental_only_target_latency_cdf,
  FigureTypes.incremental_hist : gen_incremental.generate_hist,
  FigureTypes.incremental_over_time : gen_incremental.generate_over_time,
  FigureTypes.approximate_oracle_policy : gen_approximate.generate_oracle_policy_interpolate,
  FigureTypes.approximate_cost_vs_time : gen_approximate.generate_cost_vs_time_plot,
  FigureTypes.approximate_policy_combined : gen_approximate.generate_policy_combined,
}

if __name__ == "__main__":
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
      data = parse.approximate_incremental_offline(data_fname, file_filter)
    else:
      assert(False)
      
    print("Generating ", figname)

    appearance = figconfig.get('appearance', config.DEFAULT_APPEARANCE)
    for style_name, style_rc in appearance.items():
      matplotlib.rcdefaults() # reset style
      style_rc() # apply new style
    
      # Process the data, generate the graph
      generate_function = figure_generators[figconfig['type']]
      
      # new figure
      fig = plt.figure()
      res = generate_function(data, figconfig)
      if res:
        plt.close(fig)
      if not res:
        res = [("default", fig)]
      
      for (subfigname, fig) in res:
        if 'custom_cmd' in figconfig:
          figconfig['custom_cmd']()
          
        figure_fname = figname
        if subfigname != "default":
          figure_fname += "_" + subfigname
        figure_fname += "_" + style_name + ".pdf"
        figure_fname = os.path.join(config.FIGURE_ROOT, figure_fname)
        with PdfPages(figure_fname) as out:
          out.savefig(fig)
        plt.close(fig)