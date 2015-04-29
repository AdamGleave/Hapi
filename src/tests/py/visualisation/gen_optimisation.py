import matplotlib.pyplot as plt
import numpy as np
import functools

import config.visualisation as config
from visualisation import analysis

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
  
  fig = plt.figure()

  index = np.arange(n_groups)
  bar_width = group_width / n_bars_in_group
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}
  
  for i in range(len(means)):
    if errorsZero(errors[i]):
      yerr = None 
    else:
      yerr = errors[i]
      
    label = bar_labels[i]

    bars = plt.bar(index + i * bar_width, means[i], bar_width,
               alpha=opacity,
               color=colours[label],
               yerr=yerr,
               error_kw=error_config,
               label=label,
               **kwargs)
    
  plt.xticks(index + group_width / 2, group_labels)
  plt.legend(loc='upper left')
    
  return fig

def analyse_generic_start(data):
  # get means and standard deviation of each implementation on each dataset
  times = analysis.full_extract_time(data, 'algo')
  means = analysis.full_map_on_iterations(np.mean, times)
  errors = analysis.full_map_on_iterations(
            functools.partial(analysis.t_error, config.CONFIDENCE_LEVEL), times)
  
  return (means, errors)

def analyse_generic_end(means, errors, group1, group2):
  key_matrix = [group1, group2]
  means = analysis.flatten_dict(means, key_matrix)
  errors = analysis.flatten_dict(errors, key_matrix)
  errors = analysis.interval_to_upper_lower(errors)
  
  return (means, errors)

def analyse_absolute(data, group1, group2):
  means, errors = analyse_generic_start(data)
  means, errors = analyse_generic_end(means, errors, group1, group2)
  
  return (means, errors)

def generate_absolute(data, figconfig):
  data = analysis.full_swap_file_impl(data)
  means, errors = analyse_absolute(data, figconfig['implementations'],
                                   figconfig['datasets'])
  
  fig = barchart(means, errors, 
                 figconfig['implementations'], figconfig['datasets'],
                 figconfig['colours'], log=True)
  
  plt.xlabel('Cluster size')
  plt.ylabel('Runtime (\si{\second})')
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

def analyse_relative(data, baseline, group1, group2):
  means, errors = analyse_generic_start(data)
  
  baseline_means = means[baseline]
  baseline_errors = errors[baseline]
  
  relative_means = {}
  relative_errors = {}
  relative_means[baseline] = {k : 1.0 for k in group2}
  relative_errors[baseline] = {k : (0.0, 0.0) for k in group2}
  for implementation in group1:
    if implementation != baseline:
      implementation_means = means[implementation]
      implementation_errors = errors[implementation]
      implementation_relative_means = {}
      implementation_relative_errors = {}
      for dataset in group2:
        mean, error = compute_relative(
                  baseline_means[dataset], baseline_errors[dataset],
                  implementation_means[dataset], implementation_errors[dataset])
        implementation_relative_means[dataset] = mean
        implementation_relative_errors[dataset] = error
      
      relative_means[implementation] = implementation_relative_means  
      relative_errors[implementation] = implementation_relative_errors
  
  relative_means, relative_errors = \
            analyse_generic_end(relative_means, relative_errors, group1, group2)
  
  return (relative_means, relative_errors)

def generate_relative(data, figconfig):
  data = analysis.full_swap_file_impl(data)
  means, errors = analyse_relative(data, figconfig['baseline'],
                            figconfig['implementations'], figconfig['datasets'])
   
  fig = barchart(means, errors, 
                 figconfig['implementations'], figconfig['datasets'],
                 figconfig['colours'])
  
  plt.xlabel('Cluster size')
  plt.ylabel('Speedup')
  plt.title('Speedups by cluster size and implementation')
  
  plt.tight_layout()
  
  return fig

def generate_scaling_factors(data, figconfig):
  data = analysis.full_swap_file_impl(data)
  dataset = figconfig['dataset']
  means, errors = analyse_absolute(data, figconfig['implementations'],
                                   [dataset])
  n_mean_factors, n_mean_datasets = np.shape(means)
  assert(n_mean_datasets == 1)
  n_error_factors, n_error_errors, n_error_datasets = np.shape(errors)
  assert(n_error_errors == 2 and n_error_datasets == 1)
  assert(n_mean_factors == n_error_factors)
  
  n_factors = n_mean_factors
  means = np.reshape(means, (n_factors, ))
  errors = np.reshape(errors, (n_factors, 2))
  errors = np.transpose(errors)

  fig = plt.figure()

  index = np.arange(n_factors)
  bar_width = 1.0
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}

  bars = plt.bar(index, means, bar_width,
                 alpha=opacity,
                 color=figconfig['colours'],
                 yerr=errors,
                 error_kw=error_config)
      
  plt.xticks(index + bar_width / 2, figconfig['implementations'])
  
  plt.xlabel('Scaling factor')
  plt.ylabel('Runtime (\si{\second})')
  plt.title('Runtimes by scaling factor')
  
  plt.tight_layout()
  
  return fig