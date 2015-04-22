#!/usr/bin/env python3

import config, parse
import gen_optimisation

import os, sys
from matplotlib.backends.backend_pdf import PdfPages

if __name__ == "__main__":
  if len(sys.argv) == 1:
    # no arguments
    figures = config.FIGURES
  else:
    # arguments are list of tests
    figure_names = sys.argv[1:]
    figures = {k : config.FIGURES[k] for k in figure_names}
    
  for (figname, figconfig) in config.FIGURES.items():
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
    figure_type = figconfig['type']
    if figure_type == config.FigureTypes.optimisation:
      fig = gen_optimisation.generate(data, figconfig)
    else:
      assert(false)      
    # TODO: Process and generate the graph
    
    # Export figure
    figure_fname = os.path.join(config.FIGURE_ROOT, figname + ".pdf")
    with PdfPages(figure_fname) as out:
      out.savefig(fig) 