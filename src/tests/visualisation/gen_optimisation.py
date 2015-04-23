import matplotlib.pyplot as plt
import numpy as np
import functools
import config

import analysis

def errorsZero(e):
  def arrayZero(a):
    return all(map(lambda x : x == 0.0, a))
  lower, upper = e
  return arrayZero(lower) and arrayZero(upper)

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
    if errorsZero(errors[i]):
      yerr = None 
    else:
      yerr = errors[i]
    print(yerr)
    bars = plt.bar(index + i * bar_width, means[i], bar_width,
               alpha=opacity,
               color=colours[label],
               yerr=yerr,
               error_kw=error_config,
               label=label,
               **kwargs)
    
  plt.xticks(index + bar_width, group_labels)
  plt.legend(loc='upper left')
    
  return (fig, ax)

def analyse_absolute(data, figconfig):
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

def generate_absolute(data, figconfig):
  means, errors = analyse_absolute(data, figconfig)
  
  fig, ax = barchart(means, errors, 
                     figconfig['implementations'], figconfig['datasets'],
                     figconfig['colours'], log=True)
  
  plt.xlabel('Cluster size')
  plt.ylabel('Runtime (s)')
  plt.title('Runtimes by cluster size and implementation')
  
  plt.tight_layout()
  
  return fig

def compute_relative(m1, e1, m2, e2):
  '''m1, m2 are the mean of quantity 1 and 2
     e1, e2 are errors in form (lower bound, upper bound) containing 0
     So the confidence interval for quantity 1 is (m1 + e1[0], m1 + e1[1]).
     
     Returns mean and error in the aboe form for quantity 1 / quantity 2'''
  l1 = m1 + e1[0]
  u1 = m1 + e1[1]
  l2 = m2 + e2[0]
  u2 = m2 + e2[1]

  
  mres = m1 / m2
  lres = l1 / u2
  ures = u1 / l2
  eres = (lres - mres, ures - mres)
  return (mres, eres)

def compute_relative_error(m1, e1, m2, e2):
  _, eres = compute_relative(m1, e1, m2, e2)
  return eres

def analyse_relative(data, figconfig):
  # get means and standard deviation of each implementation on each dataset
  data = analysis.full_swap_file_impl(data)
  times = analysis.full_extract_time(data, 'algo')
  means = analysis.full_map_on_iterations(np.mean, times)
  errors = analysis.full_map_on_iterations(
            functools.partial(analysis.t_error, config.CONFIDENCE_LEVEL), times)
  
  baseline = figconfig['baseline']
  baseline_means = means[baseline]
  baseline_errors = errors[baseline]
  
  relative_means = {}
  relative_errors = {}
  for implementation in figconfig['implementations']:
    if implementation == baseline:
      relative_means[implementation] = {k : 1.0 for k in figconfig['datasets']}
      relative_errors[implementation] = {k : (0.0, 0.0) for k in figconfig['datasets']}
    else:
      implementation_means = means[implementation]
      relative_means[implementation] = {k : baseline_means[k] / v  
                                        for k, v in implementation_means.items()}
      implementation_errors = errors[implementation]
      relative_errors[implementation] = {k : compute_relative_error( 
                              baseline_means[k], baseline_errors[k],
                              implementation_means[k], v)  
                              for k, v in implementation_errors.items()}
  
  key_matrix = [figconfig['implementations'], figconfig['datasets']]
  relative_means = analysis.flatten_dict(relative_means, key_matrix)
  print(relative_errors)
  relative_errors = analysis.flatten_dict(relative_errors, key_matrix)
  print(relative_errors)
  relative_errors = analysis.interval_to_upper_lower(relative_errors)
  
  return (relative_means, relative_errors)

def generate_relative(data, figconfig):
  means, errors = analyse_relative(data, figconfig)
  
  fig, ax = barchart(means, errors, 
                   figconfig['implementations'], figconfig['datasets'],
                   figconfig['colours'])
  
  plt.xlabel('Cluster size')
  plt.ylabel('Speedup')
  plt.title('Speedups by cluster size and implementation')
  
  plt.tight_layout()
  
  return fig