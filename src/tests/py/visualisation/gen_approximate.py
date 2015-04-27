import functools

import matplotlib.pyplot as plt
import numpy as np
import scipy.stats

import config.visualisation as config
from visualisation import analysis

def af_map_on_stats(func, data):
  type, d = data
  assert(type == 'approximate_full')
  return (type, analysis.dict_map(func, d))

def aih_map_on_stats(func, data):
  type, d = data
  assert(type == 'approximate_incremental_offline')
  return (type, analysis.dict_map(lambda a : list(map(func, a)), d))

def ageneric_map_on_stats(func, data):
  type, d = data
  if type == 'approximate_full':
    return af_map_on_stats(func, data)
  elif type == 'approximate_incremental_offline':
    return aih_map_on_stats(func,data)
  else:
    assert(False)

def _merge_iterations_helper(x):
  # operates on array of test iterations
  first_test_iteration = x[0]
  n_refine_iterations = len(x[0])
  
  res = []
  for i in range(n_refine_iterations):
    refine_iteration = first_test_iteration[i]
    fixed_keys = ["epsilon", "cost", "task_assignments_changed"]
    d = {k : refine_iteration[k] for k in fixed_keys}
    d["refine_time"] = list(map(lambda y : y[i]["refine_time"], x))
    d["overhead_time"] = list(map(lambda y : y[i]["overhead_time"], x))
    res.append(d)
  
  return res

def af_merge_iterations(d):
  return af_map_on_stats(_merge_iterations_helper, d)

def aih_merge_iterations(d):
  return aih_map_on_stats(_merge_iterations_helper, d)

def ageneric_merge_iterations(d):
  return ageneric_map_on_stats(_merge_iterations_helper, d)

def minimum_cost(x):
  # cost of last refine iteration
  return x[-1]['cost']

def relative_error(x):
  min_cost = minimum_cost(x)
  def calculate_relative_error(iteration):
    res = iteration.copy()
    res['relative_error'] = float(res['cost']) / min_cost - 1.0
    return res 
  return list(map(calculate_relative_error, x))

def calculate_relative_accuracy(relative_error):
  return 100.0 / (relative_error + 1.0)

def absolute_error(x):
  min_cost = minimum_cost(x)
  def calculate_absolute_error(iteration):
    res = iteration.copy()
    res['relative_error'] = res['cost'] - min_cost
    return res 
  return list(map(calculate_absolute_error, x))

def cost_change(x):
  num_refines = len(x)
  res = []
  
  first_refine = x[0].copy()
  first_refine['cost_change'] = 0.0
  res.append(first_refine)
  
  for i in range(1, num_refines):
    prev_refine = x[i-1]
    cur_refine = x[i].copy()
    cur_refine['cost_change_absolute'] = prev_refine['cost'] - cur_refine['cost']
    cur_refine['cost_change_relative'] = cur_refine['cost_change_absolute'] / prev_refine['cost']
    cur_refine['cost_change_as_in_policy'] = abs(cur_refine['cost_change_relative'])
    res.append(cur_refine)
    
  return res

def cumulative_time(x):
  first_refine_it = x[0]
  sum_refine_time = np.zeros_like(first_refine_it['refine_time'])
  sum_overhead_time = np.zeros_like(sum_refine_time)
  
  res = []
  for refine_it in x:
    refine_it = refine_it.copy()
    sum_refine_time += refine_it['refine_time']
    sum_overhead_time += refine_it['overhead_time']
    refine_it['refine_time_cumulative'] = sum_refine_time.copy()
    refine_it['overhead_time_cumulative'] = sum_overhead_time.copy()
    res.append(refine_it)
    
  return res

#perhaps don't want this, consider performing all iterations
def first_optimal(x):
  # first iteration of refine to have found minimum cost
  min_cost = minimum_cost(x)
  for i in range(len(x)):
    if x[i]['cost'] == min_cost:
      return i

def filter_first_optimal(x):
  '''only retain approximate refine iterations and the first one to attain optimality'''
  first_optimal_index = first_optimal(x)
  return x[0:first_optimal_index+1]

# requires cumulative_time having been computed
def speedup(x):
  if len(x) == 0:
    return []
  else: 
    baseline = np.array(x[first_optimal(x)]['refine_time_cumulative'])
    res = []
    for refine_it in x:
      refine_it = refine_it.copy()
      refine_it['speedup'] = (baseline / refine_it['refine_time_cumulative']) - 1.0
      res.append(refine_it)
    return res
  
def reduce_everything(data, granularity):
  type, d = data
  res = []
  for file_results in d.values():
    if type == 'approximate_incremental_offline':
      results = file_results
    else:
      results = [file_results]
    for delta_result in results:
      if granularity == 'file':
        res.append(delta_result)
      elif granularity == 'refine':
        for refine_result in delta_result:
          res.append(refine_result)
      else:
        assert(False)
  return res
  
def analyse_oracle_policy(data, figconfig):
  stats = ageneric_merge_iterations(data)
  stats = ageneric_map_on_stats(relative_error, stats)
  stats = ageneric_map_on_stats(cumulative_time, stats)
  stats = ageneric_map_on_stats(speedup, stats)
  stats = ageneric_map_on_stats(filter_first_optimal, stats)
  reduced = reduce_everything(stats, granularity='refine')
  
  accuracy_threshold = figconfig.get('min_accuracy',
                                     config.APPROXIMATE_ACCURACY_THRESHOLD)
  
  x = np.array([])
  y = np.array([])
  for pt in reduced:
    accuracy = calculate_relative_accuracy(pt['relative_error'])
    if accuracy >= accuracy_threshold:
      y = np.concatenate((y, pt['speedup']))
      x_pts = np.ones_like(pt['speedup']) * accuracy 
      x = np.concatenate((x, x_pts))
      
  return (x,y)

  
def generate_oracle_policy_scatter(data, figconfig):
  (x, y) = analyse_oracle_policy(daya, figconfig)
  
  plt.scatter(x, y)
  
  plt.autoscale(tight=True)
  ymin, ymax = plt.ylim()
  plt.ylim ( (0, ymax) )
  
  plt.xlabel('Accuracy (%)')
  plt.ylabel('Speedup')
  plt.title('Speedup against accuracy under oracle policy')  

def generate_oracle_policy_binned(data, figconfig):
  (x, y) = analyse_oracle_policy(data, figconfig)
  
  accuracy_threshold = figconfig.get('min_accuracy',
                                     config.APPROXIMATE_ACCURACY_THRESHOLD)
  num_bins = figconfig.get('num_bins',
                           config.APPROXIMATE_NUM_BINS)
  
  # there's also a similar logspace function
  bins = np.linspace(accuracy_threshold, 100, num_bins + 1)
  confidence_interval = functools.partial(analysis.t_error, config.CONFIDENCE_LEVEL)
  lower_err, _, _ = scipy.stats.binned_statistic(x, y, bins=bins, 
                                 statistic=lambda x : confidence_interval(x)[0])
  upper_err, _, _ = scipy.stats.binned_statistic(x, y, bins=bins, 
                                 statistic=lambda x : confidence_interval(x)[1])
  mean, _, _ = scipy.stats.binned_statistic(x, y, bins=bins, statistic='mean')

  # fix last bin to always be zero
  # TODO: or just replace bins above with (..., num_bins, endpoint=False)?
  #mean[-1] = lower_err[-1] = upper_err[-1] = 0
  #plt.plot([x, x, x], [lower_bound, mean, upper_bound])
  print(len(bins),len(mean))
  
  bar_width = bins[1] - bins[0]
  opacity = 0.4
  
  plt.bar(bins[0:-1], mean, yerr=[-lower_err, upper_err], alpha=opacity)

def find_speedups(accuracy_threshold, test):
  for refine_iteration in test:
    accuracy = calculate_relative_accuracy(refine_iteration['relative_error'])
    if accuracy >= accuracy_threshold:
      return refine_iteration['speedup']

def analyse_oracle_policy_interpolate(data, figconfig):
  stats = ageneric_merge_iterations(data)
  stats = ageneric_map_on_stats(relative_error, stats)
  stats = ageneric_map_on_stats(cumulative_time, stats)
  stats = ageneric_map_on_stats(speedup, stats)
  reduced = reduce_everything(stats, granularity='file')
  
  accuracy_threshold = figconfig.get('min_accuracy',
                                     config.APPROXIMATE_ACCURACY_THRESHOLD)
  accuracies = np.linspace(accuracy_threshold, 100.0, 1000)
  
  means = np.array([])
  lower_bounds = np.array([])
  upper_bounds = np.array([])
  for accuracy in accuracies:
    f = functools.partial(find_speedups, accuracy)
    speedups = list(map(f, reduced))
    speedups = np.concatenate(speedups)
    
    lower, upper = analysis.t_error(config.CONFIDENCE_LEVEL, speedups,
                                    centred=False)
    mean = np.mean(speedups)
    
    means = np.append(means, mean)
    lower_bounds = np.append(lower_bounds, lower)
    upper_bounds = np.append(upper_bounds, upper) 
        
  return (accuracies, means, lower_bounds, upper_bounds)
  
def generate_oracle_policy_interpolate(data, figconfig):
  (accuracies, means, lowers, uppers) = analyse_oracle_policy_interpolate(data, figconfig)
  
  confidence_level = int(config.CONFIDENCE_LEVEL * 100)
  plt.plot(accuracies, lowers, 'r:',
           label='Lower bound ({0}% confidence)'.format(confidence_level))
  plt.plot(accuracies, uppers, 'b:', 
           label='Upper bound ({0}% confidence)'.format(confidence_level))
  plt.plot(accuracies, means, 'g', label='Mean')
  
  plt.xlabel('Accuracy (%)')
  plt.ylabel('Speedup')
  plt.title('Speedup against accuracy')
  
  plt.legend(loc='lower left')