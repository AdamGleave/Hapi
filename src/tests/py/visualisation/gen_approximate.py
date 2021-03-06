import functools

import matplotlib.pyplot as plt
import numpy as np
import scipy.stats

import config.visualisation as config
from visualisation import analysis, plot

HEURISTIC_NAMES = {
  'cost': 'Cost convergence',
  'task_assignments': 'Task migration convergence',
}
HEURISTIC_PARAMETER_NAMES = {
  'cost': ('cost change threshold', 'c', 'Cost change threshold $c$ (\%)'),
  'task_assignments': ('task migration threshold', 't', 'Task migration threshold $t$'),
}

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
      refine_it['speedup'] = speedup
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
  
  accuracy_threshold = figconfig.get('min_accuracy_oracle_policy',
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
  
  plt.xlabel(r'Solution accuracy (\%)')
  plt.ylabel('Speedup ($\\times$)')
  
  plt.legend(loc='lower left')

def cost_heuristic(cost_threshold, test):
  for refine_it in test:
    if refine_it['cost_change_as_in_policy'] * 100 < cost_threshold:
      return refine_it
  return test[-1]

def task_assignment_heuristic(threshold, test):
  for refine_it in test:
    if refine_it['task_assignments_changed'] < threshold:
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
  return {p : np.percentile(data, p, axis=1) 
          for (p, _label, color_) in percentiles}

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

def heuristic_parameter_format(parameter):
  if type(parameter) == int:
    return str(parameter) # exact
  else:
    return '{:.1f}\%'.format(parameter) # 3 s.f.

def generate_terminating_condition_accuracy_plot(parameters, percentiles,
                                     condition, heuristic_parameter, figconfig):
  percentiles_config = figconfig.get('percentiles', 
                                     config.APPROXIMATE_DEFAULT_PERCENTILES)
  
  for (percentile, label, color) in percentiles_config:
    plt.plot(parameters, percentiles[percentile], label=label, color=color)
  
  min_accuracy = figconfig.get('min_accuracy_terminating_condition', 
                               config.APPROXIMATE_ACCURACY_THRESHOLD)
  
  ymin, ymax = plt.ylim()
  ymin = max(min_accuracy, ymin)
  ymax = 100.0
  plt.ylim(ymin, ymax)
  
  plt.autoscale(axis='x', tight=True)
  
  xmin, xmax = plt.xlim()
  target_accuracy = figconfig.get('target_accuracy',
                                  config.APPROXIMATE_TARGET_ACCURACY)
  plt.plot((xmin, xmax), (target_accuracy, target_accuracy), 'k--')
  annotation = '{0}\% target'.format(target_accuracy)
  plt.annotate(annotation, xy=(xmin, target_accuracy), xycoords='data',
               xytext=(6,-2), textcoords='offset points',
               verticalalignment='top')
  
  # This actually doesn't look very good, since there's typically a vertical
  # line at the same point from one of the percentiles. But can maybe fix it up?
  # (Perhaps an arrow to the point would be better?)
#   plt.plot((heuristic_parameter, heuristic_parameter), (ymin, ymax), 'k--')
#   annotation = 'heuristic parameter'
#   ymid = (ymin + ymax) / 2
#   plt.annotate(annotation, xy=(heuristic_parameter, ymid), xycoords='data',
#                xytext=(3,0), textcoords='offset points',
#                rotation='vertical', verticalalignment='center')

  parameter_name, _, parameter_label = HEURISTIC_PARAMETER_NAMES[condition]    
  plt.xlabel(parameter_label)
  plt.ylabel(r'Solution accuracy (\%)')
  
  legend_loc = figconfig.get('parameters_legend', 
                             {'cost': 'best', 'task_assignments': 'best'})
  if legend_loc and legend_loc[condition]:
    plt.legend(loc=legend_loc[condition])

# def nsf(num, n=3):
#     """n-Significant Figures"""
#     numstr = ("{0:.%ie}" % (n-1)).format(num)
#     return float(numstr)

def cdf_title(condition, parameter):
  parameter_name, parameter_variable, _ = HEURISTIC_PARAMETER_NAMES[condition]
  title = r'\smaller{{{0} ${1}={2}$}}'.format(parameter_name.capitalize(),
                    parameter_variable, heuristic_parameter_format(parameter))
  return title

def generate_terminating_condition_accuracy_distribution(data, parameter, 
                                                         condition, figconfig):
  stats = ageneric_map_on_stats(discard_iterations, data)
  reduced = analyse_terminating_condition_setup(stats, condition)

  accuracies = analyse_terminating_condition_parameter(reduced, condition,
                                                     extractAccuracy, parameter)
  target_accuracy = figconfig.get('target_accuracy', 
                                    config.APPROXIMATE_TARGET_ACCURACY)
  
  num_below_target = np.sum(accuracies < target_accuracy)
  percent_breached = float(num_below_target) / len(accuracies) * 100.0
  
  print("Accuracy: mean - ", np.mean(accuracies), 
        "; worst accuracy - ", np.min(accuracies))
  
  def genericDraw():
    # draw CDF
    plot.cdf([accuracies], labels=['Heuristic'], colours={'Heuristic': 'b'})
    
    # draw vertical line at desired accuracy
    plt.plot((target_accuracy, target_accuracy), (0, 100), 'k--')
    
    annotation = 'accuracy target'.format(target_accuracy)
    plt.annotate(annotation, xy=(target_accuracy, 50), xycoords='data',
                 xytext=(-1,0), textcoords='offset points', rotation='vertical', 
                 horizontalalignment='right', verticalalignment='center') 
    
    if percent_breached:
      inaccuracy_annotation = r'{0:.3}\%'.format(percent_breached)
      plt.annotate(inaccuracy_annotation,
                   xy=(target_accuracy, percent_breached), xycoords='data',
                   xytext=(-1,1), textcoords='offset points',
                   horizontalalignment='right') 
      
    # add labels
    plt.xlabel(r'Solution accuracy (\%)')
    title = figconfig.get('title', True)
    if title:
      plt.title(cdf_title(condition, parameter))
    
    plt.ylim(-5, 100)
  
  wide = plt.figure()
  genericDraw()
  xmin, xmax = plt.xlim()
  width = 100 - target_accuracy
  at_least = target_accuracy - width * 0.2
  xmin = min(xmin, at_least)
  width = 100 - xmin
  plt.xlim(xmin, 100 + width * 0.02)
  
  narrow = plt.figure()
  genericDraw()
  width = 100 - at_least
  plt.xlim(at_least, 100 + width * 0.02)
  
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
  
  annotate_means = figconfig.get('speed_annotate_means', [])
  def format(mean):
    return r"{0:.1f}\times".format(mean)
  heuristic_name = HEURISTIC_NAMES[condition]
  plot.cdf([oracle_speeds, heuristic_speeds],
           labels=['Oracle', heuristic_name], 
           colours={heuristic_name: '#0000CD', 'Oracle': '#568203'},
           annotate_means=annotate_means,
           annotate_means_format=format)
  
  plt.xlabel('Speedup ($\\times$)')
  title = figconfig.get('title', True)
  if title:
    plt.title(cdf_title(condition, parameter))
  
  legend_loc = figconfig.get('speed_legend', 'lower right')
  if legend_loc:
    plt.legend(loc=legend_loc)
  
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
        
    # fake the type as 'approximate_full', since it only has one file in it
    training_data = ('approximate_full', training_times)
    test_data = ('approximate_full', test_times)
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
  if condition == 'task_assignments':
    heuristic_parameter = int(heuristic_parameter)
  
  print("Heuristic parameter for ", condition, " - ", heuristic_parameter)
  
  generate_terminating_condition_accuracy_plot(parameters, percentiles, 
                                      condition, heuristic_parameter, figconfig)
  yield (condition + '_parameters', fig)
  
  wide, narrow = generate_terminating_condition_accuracy_distribution(test_data, 
                                      heuristic_parameter, condition, figconfig)
  yield (condition + '_accuracy_wide', wide)
  yield (condition + '_accuracy_narrow', narrow)
  
  fig = plt.figure()
  generate_terminating_condition_speed_distribution(test_data, 
                                      heuristic_parameter, condition, figconfig)
  yield (condition + '_speed', fig)
 
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
  file = figconfig['over_time_file']
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
  
  plt.xlabel('Cumulative runtime (\si{\second})')
  plt.ylabel('Solution cost')
