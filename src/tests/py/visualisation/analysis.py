import math
import numpy as np
import scipy.stats

def _error(x):
  mu = np.mean(x)
  sigma = np.std(x)
  n = len(x)
  sample_sigma = sigma / math.sqrt(n)
  
  return (mu, sample_sigma, n)

def normal_error(alpha, x):
  (_mu, sample_sigma, _n) = _error(x)
  return scipy.stats.norm.interval(alpha, scale=sample_sigma)

def t_error(alpha, x):
  (_mu, sample_sigma, n) = _error(x)
  return scipy.stats.t.interval(alpha, n, scale=sample_sigma)

def interval_to_upper_lower(l):
  def upper_lower(x):
    # note pyplot expects all errors to be positive
    # negate sign for lower
    lower = list(map(lambda y : -y[0], x))
    upper = list(map(lambda y : y[1], x))
    return [lower, upper]
  return list(map(upper_lower, l))

def convert_time(time_str):
  if time_str == 'Timeout':
    return float('+inf')
  else:
    return float(time_str)

def full_map_on_iterations(func, d):
  res = {k1 : 
           {k2 : func(v2) 
            for k2, v2 in v1.items()}
         for k1, v1 in d.items()}
  return res

def full_extract_time(d, final_key):
  def timeLambda(x):
    return list(map(lambda x : convert_time(x[final_key]), x))
  return full_map_on_iterations(timeLambda, d)

def full_mean(d):
  return full_map_on_iterations(np.mean, d)

def full_sd(d):
  return full_map_on_iterations(np.std, d)

def full_swap_file_impl(data):
  new_data = {} 
  for (file_name, file_res) in data.items():
    for (impl_name, impl_res) in file_res.items():
      new_impl_res = new_data.get(impl_name, {})
      new_file_res = impl_res
      new_impl_res[file_name] = new_file_res
      new_data[impl_name] = new_impl_res
      
  return new_data

def extract_summary_stats(l):
  mean = [x['mean'] for x in l]
  sd = [x['sd'] for x in l]
  return (mean, sd)
  
def flatten_dict(d, key_matrix):
  if len(key_matrix) == 0:
    return d
  else:
    keys = key_matrix[0]
    key_matrix = key_matrix[1:]
    return [flatten_dict(d[k], key_matrix) for k in keys]