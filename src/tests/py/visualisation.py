#!/usr/bin/env python3
 
import config.visualisation as config
from visualisation import parse, gen_optimisation, gen_incremental, gen_approximate
from visualisation.test_types import FigureTypes

import os, sys
from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

figure_generators = {
  FigureTypes.optimisation_absolute : gen_optimisation.generate_absolute,
  FigureTypes.optimisation_relative : gen_optimisation.generate_relative,
  FigureTypes.incremental_cdf : gen_incremental.generate_cdf,
  FigureTypes.incremental_hist : gen_incremental.generate_hist,
  FigureTypes.incremental_over_time : gen_incremental.generate_over_time,
  FigureTypes.approximate_oracle_policy : gen_approximate.generate_oracle_policy,  
}

if __name__ == "__main__":
  if len(sys.argv) == 1:
    # no arguments
    figures = config.FIGURES
  else:
    # arguments are list of tests
    figure_names = sys.argv[1:]
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
    else:
      assert(false)

    # Process the data, generate the graph
    generate_function = figure_generators[figconfig['type']]
    # new figure
    plt.figure()
    fig = generate_function(data, figconfig)
    
    # Export figure
    figure_fname = os.path.join(config.FIGURE_ROOT, figname + ".pdf")
    with PdfPages(figure_fname) as out:
      out.savefig(fig) 