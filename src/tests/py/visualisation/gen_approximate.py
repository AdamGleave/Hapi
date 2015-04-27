import matplotlib.pyplot as plt
import numpy as np

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

#def first_optimal(x):
#  return -1

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
  
def reduce_everything(data):
  type, d = data
  res = []
  for file_results in d.values():
    if type == 'approximate_offline':
      results = file_results
    else:
      results = [file_results]
    for delta_result in results:
      for refine_result in delta_result:
        res.append(refine_result)
  return res
  
def analyse_oracle_policy(data):
  stats = ageneric_merge_iterations(data)
  stats = ageneric_map_on_stats(relative_error, stats)
  stats = ageneric_map_on_stats(cumulative_time, stats)
  stats = ageneric_map_on_stats(speedup, stats)
  return reduce_everything(stats)
  
def generate_oracle_policy(data, figconfig):
  reduced = analyse_oracle_policy(data)
  accuracy_threshold = figconfig.get('min_accuracy',
                                     config.APPROXIMATE_ACCURACY_THRESHOLD)
  
  x = np.array([])
  y = np.array([])
  for pt in reduced:
    accuracy = 100.0 / (pt['relative_error'] + 1.0)
    if accuracy >= accuracy_threshold:
      y = np.concatenate((y, pt['speedup']))
      x_pts = np.ones_like(pt['speedup']) * accuracy 
      x = np.concatenate((x, x_pts))
  
  plt.scatter(x, y)
  
  # Flip the x-axis so it is *decreasing* relative error
  # XXX: Or make it relative accuracy instead?
  plt.autoscale(tight=True)
  ymin, ymax = plt.ylim()
  # discard any speedups below 0
  plt.ylim ( (0, ymax) )
  
  #plt.xscale('log')
  plt.xlabel('Accuracy (%)')
  plt.ylabel('Speedup')
  plt.title('Speedup against accuracy under oracle policy')

  # Aggregating the data is the hard bit, but think need to do this otherwise
  # you won't have enough data points.
  
  # You could present it for just one graph? This might simplify things a bit.
  # But limited validity -- you'd only be removing noise from test process.
  
  # Only way I can think of is to bin on relative accuracy, then compute CI
  # within that bin. The bins will   need to be logarithmic, though.
  
  # Presentation would thus be as a bar chart? Slightly strange, but maybe best.
  
  # Scatter plot is also a good format, actually.
  
  

  # Q: optimal iterations span from positive to zero speedup
  
  # compute confidence interval of speedups (binned by accuracy?)
  
  # merge iterations
  # cumulative time
  # speedups
  # confidence intervals of speedups (probably need to bin by accuracy)
  
  
  # x: relative accuracy
  # y: refine speedups 
  
  # Input data: should aggregate over *everything* included. 
  # (Could optionally apply a filter to limit the data range beforehand.)
  
  # Compute speedups to normalise times across different graphs
  # Then plot speedups with confidence intervals 

  # Will need some way of aggregating over af or aih as needed, worth writing
  # this as a utility function.
  
  # Will also need to think about what the correct way of computing confidence
  # intervals on relative values.  