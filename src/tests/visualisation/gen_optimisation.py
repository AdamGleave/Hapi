import matplotlib.pyplot as plt
import numpy as np

import analysis

def barchart(means, errors, bar_labels, group_labels, colours, **kwargs):
  n_groups = len(group_labels)
  assert(len(means) == len(errors) == len(bar_labels))
  
  fig, ax = plt.subplots()

  index = np.arange(n_groups)
  bar_width = 0.7 / n_groups
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}
  
  for i in range(len(means)):
    label = bar_labels[i]
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

def generate(data, figconfig):
  # get means and standard deviation of each implementation on each dataset
  data = analysis.full_swap_file_impl(data)
  times = analysis.full_extract_time(data, 'algo')
  stats = analysis.full_summary_stats(times)      
 
  means = []
  errors = []
  for implementation in figconfig['implementations']:
    data = analysis.flatten_dict(stats[implementation], figconfig['datasets'])
    mean, sd = analysis.extract_summary_stats(data)
    means.append(mean)
    # XXX: error is not 1 s.d.
    errors.append(np.array(sd))
  
  fig, ax  = barchart(means, errors, 
                      figconfig['implementations'], figconfig['datasets'],
                      figconfig['colours'], log=True)
  
  plt.xlabel('Data set')
  plt.ylabel('Runtime (s)')
  plt.title('Runtimes by data set size and version')
  
  plt.tight_layout()
  
  return fig