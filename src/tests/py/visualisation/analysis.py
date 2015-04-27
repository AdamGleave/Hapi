import math
import numpy as np
import scipy.stats

import itertools

def _error(x):
  mu = np.mean(x)
  sigma = np.std(x)
  n = len(x)
  sample_sigma = sigma / math.sqrt(n)
  
  return (mu, sample_sigma, n)

def normal_error(alpha, x, centred=True):
  (mu, sample_sigma, _n) = _error(x)
  if centred:
    mu = 0
  return scipy.stats.norm.interval(alpha, scale=sample_sigma)

def t_error(alpha, x, centred=True):
  (mu, sample_sigma, n) = _error(x)
  if centred:
    mu = 0
  return scipy.stats.t.interval(alpha, n, loc=mu, scale=sample_sigma)

def interval_to_upper_lower(l):
  def upper_lower(x):
    # note pyplot expects all errors to be positive
    # negate sign for lower
    lower = list(map(lambda y : -y[0], x))
    upper = list(map(lambda y : y[1], x))
    return [lower, upper]
  return list(map(upper_lower, l))

def moving_average(x, n, type='simple'):
  # from pyplot demo
  """
  compute an n period moving average.

  type is 'simple' | 'exponential'
  """                                                                                                                                                        
  x = np.asarray(x)
  if type == 'simple':
      weights = np.ones(n)
  else:                                                                                                                                   
      weights = np.exp(np.linspace(-1., 0., n))

  weights /= weights.sum()

  a =  np.convolve(x, weights, mode='full')[:len(x)]
  a[:n] = a[n]
  return a

def dict_map(func, d, depth=1):
  if depth==1:
    return {k : func(v) for k, v in d.items()}
  else:
    return {k : dict_map(func, v, depth-1) for k, v in d.items()}

def full_map_on_iterations(func, d):
  return dict_map(func, d, 2)

def full_mean(d):
  return full_map_on_iterations(np.mean, d)

def full_sd(d):
  return full_map_on_iterations(np.std, d)

def full_swap_file_impl(x):
  # SOMEDAY: stripping type here isn't the best way of handling it
  type, d = x
  new_d = {} 
  for (file_name, file_res) in d.items():
    for (impl_name, impl_res) in file_res.items():
      new_impl_res = new_d.get(impl_name, {})
      new_file_res = impl_res
      new_impl_res[file_name] = new_file_res
      new_d[impl_name] = new_impl_res
      
  return new_d

def ion_map_on_implementations(func, data):
  type, d = data
  assert(type == 'incremental_online')
  return (type, dict_map(func, d, 2))

def ion_map_on_iterations(func, d):
  return ion_map_on_implementations(lambda x : np.array(list(map(func, x))), d)
  
def flatten_dict(d, key_matrix):
  if len(key_matrix) == 0:
    return d
  else:
    keys = key_matrix[0]
    key_matrix = key_matrix[1:]
    return [flatten_dict(d[k], key_matrix) for k in keys]
  
def chain_lists(l):
  return list(itertools.chain(*l))
  
def convert_time(time_str):
  if time_str == 'Timeout':
    return float('+inf')
  else:
    return float(time_str)

def full_extract_time(d, final_key):
  def timeLambda(x):
    return list(map(lambda y : convert_time(y[final_key]), x))
  return full_map_on_iterations(timeLambda, d)

def cluster_time_to_seconds(cluster_time):
  seconds = float(cluster_time) / (1000 * 1000)
  return seconds - 600

def ion_extract_time(d, final_key):
  dtype = [('cluster_time', 'float'), (final_key, 'float')]
  def extract_lambda(x):
    extracted = list(map(lambda y : (cluster_time_to_seconds(y[0]),
                                     convert_time(y[1][final_key])), x))
    return np.array(extracted, dtype=dtype)
  return ion_map_on_implementations(lambda x : list(map(extract_lambda, x)), d)

def ion_filter_cluster_time(at_least):
  def filter_lambda(x):
    mask = np.where(x['cluster_time'] >= at_least)
    return x[mask]
  return filter_lambda

def ion_get_cluster_timestamp(x):
  return list(map(lambda y : y[0], x))

def ion_get_scheduler_time(x):
  return list(map(lambda y : y[1], x))