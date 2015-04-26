import matplotlib.pyplot as plt
import numpy as np

import config.visualisation as config
from visualisation import analysis

def _merge_iterations_helper(x):
  # operates on array of test iterations
  test_iteration = x[0]
  n_refine_iterations = len(test_iteration)
  
  res = []
  for i in range(n_refine_iterations):
    fixed_keys = ["epsilon", "cost", "task_assignments_changed"]
    d = {k : test_iteration[i][k] for k in fixed_keys}
    d["refine_time"] = list(map(lambda y : y["refine_time"], test_iteration))
    d["overhead_time"] = list(map(lambda y : y["overhead_time"], test_iteration))
    res.append(d)
  
  return res

def af_merge_iterations(d):
  return analysis.dict_map(_merge_iterations_helper, d)

def aih_merge_iterations(d):
  return analysis.dict_map(lambda a : list(map(_merge_iterations_helper, a)), d)

def af_map_on_stats(func, d):
  return analysis.dict_map(func, d)

def aih_map_on_stats(func, d):
  return analysis.dict_map(lambda a : list(map(func, a)), d)

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