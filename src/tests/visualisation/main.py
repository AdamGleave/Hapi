#!/usr/bin/env python3

import config, parse
import gen_optimisation

import os
from matplotlib.backends.backend_pdf import PdfPages

if __name__ == "__main__":
  for (figname, figconfig) in config.FIGURES.items():
    # Import CSV input
    data_name = figconfig['data']
    test_type = data_name.split("_")[0]
    
    data_fname = os.path.join(config.DATA_ROOT, data_name + ".csv")
    data = None
    if test_type == "f":
      data = parse.parseFull(data_fname)
    elif test_type == "iof" or test_type == "ihy":
      data = parse.parseIncrementalOffline(data_fname)
    elif test_type == "ion":
      data = parse.parseIncrementalOnline(data_fname)
    else:
      assert(false)

    # Process the data, generate the graph
    figure_type = figconfig['figure_type']
    if figure_type == config.FigureTypes.optimisation:
      fig = gen_optimisation.generate(data, config)
    else:
      assert(false)      
    # TODO: Process and generate the graph
    
    # Export figure
    figure_fname = os.path.join(config.FIGURE_ROOT, figname + ".pdf")
    with PdfPages(figure_fname) as out:
      out.savefig(fig) 