import functools

import matplotlib.pyplot as plt
import numpy as np
import scipy.stats

import config.visualisation as config
from visualisation import analysis, plot

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

def discard_iterations(x):
  # return first test iteration
  return x[0]

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
  first_refine['cost_change_absolute'] = float('+inf')
  first_refine['cost_change_relative'] = float('+inf')
  first_refine['cost_change_as_in_policy'] = float('+inf')
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
    baseline = np.array(x[-1]['refine_time_cumulative'])
    res = []
    for refine_it in x:
      refine_it = refine_it.copy()
      speedup = (baseline / refine_it['refine_time_cumulative']) - 1.0
      refine_it['speedup'] = speedup * 100.0
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

def analyse_oracle_policy_points(data, figconfig):  
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
  (x, y) = analyse_oracle_policy_scatter(data, figconfig)
  
  plt.scatter(x, y)
  
  plt.autoscale(tight=True)
  ymin, ymax = plt.ylim()
  plt.ylim ( (0, ymax) )
  
  plt.xlabel(r'Accuracy (\%)')
  plt.ylabel('Speedup (\%)')
  plt.title('Speedup against accuracy under oracle policy')  

def generate_oracle_policy_binned(data, figconfig):
  (x, y) = analyse_oracle_policy_points(data, figconfig)
  
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

def oracle_condition(accuracy_threshold, test):
  for refine_iteration in test:
    accuracy = calculate_relative_accuracy(refine_iteration['relative_error'])
    if accuracy >= accuracy_threshold:
      return refine_iteration
    
def oracle_speedups(accuracy_threshold, test):
  return oracle_condition(accuracy_threshold, test)['speedup']

def standard_condition(_parameter, test):
  assert(_parameter == None)
  return test[-1]

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
    f = functools.partial(oracle_speedups, accuracy)
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
           label=r'Lower bound ({0}\% confidence)'.format(confidence_level))
  plt.plot(accuracies, uppers, 'b:', 
           label=r'Upper bound ({0}\% confidence)'.format(confidence_level))
  plt.plot(accuracies, means, 'g', label='Mean')
  
  plt.xlabel(r'Accuracy (\%)')
  plt.ylabel('Speedup (\%)')
  plt.title('Speedup under oracle policy against accuracy')
  
  plt.legend(loc='lower left')

def cost_heuristic(cost_threshold, test):
  for refine_it in test:
    if refine_it['cost_change_as_in_policy'] < cost_threshold:
      return refine_it
  return test[-1]

def task_assignment_heuristic(threshold, test):
  for refine_it in test:
    if refine_it['task_assignments'] < threshold:
      return refine_it
  return test[-1]

def analyse_terminating_condition_parameter(stats, condition, extractValue, parameter):
  if condition == 'cost':
    f = cost_heuristic
  elif condition == 'task_assignments':
    f = task_assignment_heuristic
  elif condition == 'oracle':
    f = oracle_condition
  elif condition == 'standard':
    f = standard_condition
  else:
    print("Unrecognised condition ", condition)
    assert(False)
  def findValue(x):
    refine_it = f(parameter, x)
    return extractValue(refine_it)
  values = np.array(list(map(findValue, stats)))
  return values 

def analyse_terminating_condition_setup(stats, condition):
  stats = ageneric_map_on_stats(relative_error, stats)
  if condition == 'cost':
    stats = ageneric_map_on_stats(cost_change, stats)
  reduced = reduce_everything(stats, granularity='file')
  return reduced

def analyse_terminating_condition(stats, condition, figconfig, extractValue):  
  n_samples = 1000
  if condition == 'cost':
    max_cost_parameter = figconfig.get('max_cost_parameter', 
                                       config.APPROXIMATE_MAX_COST_PARAMETER)
    parameters = np.linspace(0, max_cost_parameter, n_samples)
  elif condition == 'task_assignments':
    max_ta_parameter = figconfig.get('max_task_assignments_parameter', 
                                       config.APPROXIMATE_MAX_TASK_ASSIGNMENTS_PARAMETER)
    parameters = np.linspace(0, max_ta_parameter, n_samples)
  else:
    assert(False)
  
  n_tests = len(stats)
  values_by_parameter = np.empty((n_samples, n_tests))
  for i in range(n_samples):
    values_by_parameter[i] = analyse_terminating_condition_parameter(stats,
                              condition, extractValue, parameters[i])

  return (parameters, values_by_parameter)

def analyse_percentiles(data, percentiles):
  return {p : np.percentile(data, p, axis=1) for p in percentiles}

def extractAccuracy(refine_it):
  return calculate_relative_accuracy(refine_it['relative_error'])

def extractSpeeds(refine_it):
  return refine_it['speedup']

def analyse_terminating_condition_percentiles(data, condition, figconfig):
  stats = ageneric_map_on_stats(discard_iterations, data)
  reduced = analyse_terminating_condition_setup(stats, condition)

  (parameters, accuracies) = analyse_terminating_condition(reduced, condition,
                                                     figconfig, extractAccuracy)
  percentiles_config = figconfig.get('percentiles', 
                                     config.APPROXIMATE_DEFAULT_PERCENTILES)
  percentiles = analyse_percentiles(accuracies, percentiles_config)
  
  return (parameters, percentiles)

def generate_terminating_condition_accuracy_plot(parameters, percentiles, figconfig):
  percentiles_config = figconfig.get('percentiles', 
                                   config.APPROXIMATE_DEFAULT_PERCENTILES)
  
  for (percentile, percentile_label) in percentiles_config.items():
    plt.plot(parameters, percentiles[percentile], label=percentile_label)
  
  min_accuracy = figconfig.get('min_accuracy', 
                               config.APPROXIMATE_ACCURACY_THRESHOLD)
  
  ymin, ymax = plt.ylim()
  ymin = max(min_accuracy, ymin)
  plt.ylim(ymin, 100.0)
  
  plt.autoscale(axis='x', tight=True)
  
  xmin, xmax = plt.xlim()
  target_accuracy = figconfig.get('target_accuracy',
                                  config.APPROXIMATE_TARGET_ACCURACY)
  plt.plot((xmin, xmax), (target_accuracy, target_accuracy), 'k--')
  annotation = '{0}\% target'.format(target_accuracy)
  plt.annotate(annotation, xy=(xmin, target_accuracy), xycoords='data',
               xytext=(6,-2), textcoords='offset points',
               verticalalignment='top') 
    
  plt.xlabel('Parameter')
  plt.ylabel(r'Accuracy (\%)')
  plt.title('Accuracy against heuristic parameter')
  
  plt.legend(loc='lower left')
  
# SOMEDAY: could write a terminating condition speed plot here?
# But this is perhaps best reserved for when trying a particular parameter.

def generate_terminating_condition_accuracy_distribution(data, parameter, 
                                                          condition, figconfig):
  stats = ageneric_map_on_stats(discard_iterations, data)
  reduced = analyse_terminating_condition_setup(stats, condition)

  accuracies = analyse_terminating_condition_parameter(reduced, condition,
                                                     extractAccuracy, parameter)
  target_accuracy = figconfig.get('target_accuracy', 
                                    config.APPROXIMATE_TARGET_ACCURACY)
  
  accuracies_sorted = np.sort(accuracies)
  points_breached = np.where(accuracies_sorted < target_accuracy)
  most_accurate_breach_index = points_breached[-1]
  percent_breached = (float(most_accurate_breach_index) + 1) * 100.0 \
                   / len(accuracies_sorted)
  
  def genericDraw():
    # draw CDF
    plot.cdf([accuracies], labels=['Heuristic'], colours={'Heuristic': 'b'})
    
    # draw vertical line at desired accuracy
    plt.plot((target_accuracy, target_accuracy), (0, 100), 'k--')
    
    annotation = 'accuracy target'.format(target_accuracy)
    plt.annotate(annotation, xy=(target_accuracy, 50), xycoords='data',
                 xytext=(3,0), textcoords='offset points',
                 rotation='vertical', verticalalignment='center') 
    
    inaccuracy_annotation = r'{0:.3}\%'.format(percent_breached)
    plt.annotate(inaccuracy_annotation,
                 xy=(target_accuracy, percent_breached), xycoords='data',
                 xytext=(-1,1), textcoords='offset points',
                 horizontalalignment='right') 
      
    # add labels
    plt.xlabel(r'Accuracy (\%)')
    title = r'CDF for accuracy (heuristic parameter {0:.3f})'.format(parameter)
    plt.title(title)
  
  wide = plt.figure()
  genericDraw()
  xmin, xmax = plt.xlim()
  width = 100 - target_accuracy
  at_least = target_accuracy - width * 0.2
  xmin = min(xmin, at_least)
  plt.xlim(xmin, 100)
  
  narrow = plt.figure()
  genericDraw()
  plt.xlim(at_least, 100)
  
  return (wide, narrow)
  
  
def generate_terminating_condition_speed_distribution(data, parameter,
                                                          condition, figconfig):
  stats = ageneric_merge_iterations(data)
  stats = ageneric_map_on_stats(cumulative_time, stats)
  stats = ageneric_map_on_stats(speedup, stats)
  reduced = analyse_terminating_condition_setup(stats, condition)

  heuristic_speeds = analyse_terminating_condition_parameter(reduced, condition, 
                                                       extractSpeeds, parameter)
  target_accuracy = figconfig.get('target_accuracy',
                                  config.APPROXIMATE_TARGET_ACCURACY)
  oracle_speeds = analyse_terminating_condition_parameter(reduced,
                          'oracle', extractSpeeds, target_accuracy)
#   standard_speeds = analyse_terminating_condition_parameter(reduced,
#                           'standard', extractSpeeds, None)
  
  heuristic_speeds = np.concatenate(heuristic_speeds)
  oracle_speeds = np.concatenate(oracle_speeds)
#   standard_speeds = np.concatenate(standard_speeds)
  
  plot.cdf([oracle_speeds, heuristic_speeds],
           labels=['Oracle', 'Heuristic'], 
           colours={'Heuristic': 'b', 'Oracle': 'g'})
  
  plt.xlabel('Speedup (\%)')
  title = r'CDF for speedup (heuristic parameter {0:.3f})'.format(parameter)
  plt.title(title)
  
  plt.legend(loc='lower right')
  
def split_training_and_test(data, figconfig):
  # split data into training set and test set
  type, times = data
  n_training = figconfig['training']
  n_test = figconfig['test']
  if type == 'approximate_full':
    # split on files
    files = sorted(times.keys()) # sort so order is deterministic
    assert(len(files) == n_training + n_test)
    training_files = files[0:n_training]
    test_files = files[n_training:len(files)]
    
    training_times = {k : times[k] for k in training_files}
    test_times = {k : times[k] for k in test_files}
    training_data = (type, training_times)
    test_data = (type, test_times)
  elif type == 'approximate_incremental_offline':
    # split on the deltas. note we only consider a single file.
    file = figconfig['file']
    times = times[file]
    
    assert(len(times) == n_training + n_test)
    # Now this is a bit of a hack. We want to pretend it's in full format,
    # for consistency.
    training_times = {k : times[k] for k in range(n_training)}
    test_times = {k : times[k] for k in range(n_training,len(times))}
        
    # fake the type as 'full', since it only has one file in it
    training_data = ('full', training_times)
    test_data = ('full', test_times)
  else:
    print("ERROR: Unrecognised type ", type)
    assert(False)
    
  return (training_data, test_data)

def has_task_assignments(data):
  type, times = data
  some_key = list(times.keys())[0]
  some_time = times[some_key][0][0]
  # task assignments will be -1 if not computed, 
  # should never be -1 if computed
  # SOMEDAY(adam): They might be if timeout occurs, but then we don't want
  # that in our dataset anyway.
  
  return some_time['task_assignments_changed'] != -1

def generate_policy_combined_for_condition(training_data, test_data,
                                           condition, figconfig):
  fig = plt.figure()
  parameters, percentiles = analyse_terminating_condition_percentiles\
                                           (training_data, condition, figconfig)
  accuracy_at_percentile = figconfig.get('accuracy_at_percentile', 
                                         config.APPROXIMATE_ACCURACY_AT_PERCENTILE)
  percentile = percentiles[accuracy_at_percentile]
  target_accuracy = figconfig.get('target_accuracy',
                                  config.APPROXIMATE_TARGET_ACCURACY)
  indices_with_accuracy, = np.nonzero(percentile >= target_accuracy)
  last_index_with_accuracy = indices_with_accuracy[-1]
  heuristic_parameter = parameters[last_index_with_accuracy]
  
  generate_terminating_condition_accuracy_plot(parameters, percentiles, figconfig)
  yield (condition + '_policy_parameters', fig)
  
  wide, narrow = generate_terminating_condition_accuracy_distribution(test_data, 
                                      heuristic_parameter, condition, figconfig)
  yield (condition + '_policy_accuracy_wide', wide)
  yield (condition + '_policy_accuracy_narrow', narrow)
  
  fig = plt.figure()
  generate_terminating_condition_speed_distribution(test_data, 
                                      heuristic_parameter, condition, figconfig)
  yield (condition + '_policy_speed', fig)
 
def generate_policy_combined(data, figconfig):
  # TBC: Automatically detect conditions, or at least support both
  training_data, test_data = split_training_and_test(data, figconfig)
  
  conditions = ['cost']
  if has_task_assignments(training_data):
    conditions += ['task_assignments']
  
  for condition in conditions:
    for x in generate_policy_combined_for_condition(training_data, test_data,
                                                    condition, figconfig):
      yield x
  
def generate_cost_vs_time_plot(data, figconfig):
  type, d = data
  # easy enough to extend to incremental_offline, but would need to add
  # 'delta_id' parameter to figconfig along with 'file' parameter
  assert(type == 'approximate_full')
  file = figconfig['file']
  d = {file : d[file]}
  
  data = (type, d)
  stats = ageneric_merge_iterations(data)
  stats = ageneric_map_on_stats(cumulative_time, stats)
  reduced = reduce_everything(stats, granularity='refine')
  
  xs = []
  ys = []
  for refine_it in reduced:
    xs.append(np.mean(refine_it['refine_time_cumulative']))
    ys.append(refine_it['cost'])
  
  plt.plot(xs, ys, marker='.')
  plt.yscale('log')
  
  plt.xlabel('Runtime (\si{\second})')
  plt.ylabel('Solution cost')
  plt.title('Solution cost against algorithm runtime')