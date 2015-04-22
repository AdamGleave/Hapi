import matplotlib.pyplot as plt
import numpy as np
import functools
import config

import analysis

def barchart(means, errors, bar_labels, group_labels, colours, 
             group_width=0.7, **kwargs):
  n_groups = len(group_labels)
  n_bars_in_group = len(bar_labels)
  assert(len(means) == len(errors) == len(bar_labels))
  
  fig, ax = plt.subplots()

  index = np.arange(n_groups)
  bar_width = group_width / n_bars_in_group
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}
  
  for i in range(len(means)):
    label = bar_labels[i]
    print(errors[i])
    bars = plt.bar(index + i * bar_width, means[i], bar_width,
               alpha=opacity,
               color=colours[label],
               yerr=errors[i],
               error_kw=error_config,
               label=label,
               **kwargs)
    
  plt.xticks(index + bar_width, group_labels)
  plt.legend(loc='upper left')
    
  return (fig, ax)

def analyse(data, figconfig):
  # get means and standard deviation of each implementation on each dataset
  data = analysis.full_swap_file_impl(data)
  times = analysis.full_extract_time(data, 'algo')
  means = analysis.full_map_on_iterations(np.mean, times)
  errors = analysis.full_map_on_iterations(
            functools.partial(analysis.t_error, config.CONFIDENCE_LEVEL), times)
 
  key_matrix = [figconfig['implementations'], figconfig['datasets']]
  means = analysis.flatten_dict(means, key_matrix)
  errors = analysis.flatten_dict(errors, key_matrix)
  errors = analysis.interval_to_upper_lower(errors)
  
  return (means, errors)

def generate(data, figconfig):
  means, errors = analyse(data, figconfig)
  
  fig, ax = barchart(means, errors, 
                     figconfig['implementations'], figconfig['datasets'],
                     figconfig['colours'], log=True)
  
  plt.xlabel('Data set')
  plt.ylabel('Runtime (s)')
  plt.title('Runtimes by data set size and version')
  
  plt.tight_layout()
  
  return fig