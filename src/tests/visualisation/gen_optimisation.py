import matplotlib.pyplot as plt
import numpy as np

import analysis

def generate(data, figconfig):
  # get means and standard deviation of each implementation on each dataset
  data = analysis.full_swap_file_impl(data)
  times = analysis.full_extract_time(data, 'algo')
  stats = analysis.full_summary_stats(times)      
  
  fig, ax = plt.subplots()
  
  n_groups = len(figconfig['datasets'])
  index = np.arange(n_groups)
  bar_width = 0.7 / n_groups
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}
 
  nbar = 0
  for implementation in figconfig['implementations']:
    data = analysis.flatten_dict(stats[implementation], figconfig['datasets'])
    mean, sd = analysis.extract_summary_stats(data)
    
    bars = plt.bar(index + nbar * bar_width, mean, bar_width,
                   log=True,
                   alpha=opacity,
                   color=figconfig['colours'][implementation],
                   yerr=sd,
                   error_kw=error_config,
                   label=implementation)
    
    nbar = nbar + 1
  
  plt.xlabel('Data set')
  plt.ylabel('Runtime (s)')
  plt.title('Runtimes by data set size and version')
  plt.xticks(index + bar_width, figconfig['datasets'])
  plt.legend(loc='upper left')
  
  plt.tight_layout()
  
  return fig