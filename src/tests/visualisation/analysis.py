import numpy as np

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

def full_summary_stats(d):
  def statsLambda(x):
    return {'mean': np.mean(x), 'sd': np.std(x)}
  return full_map_on_iterations(statsLambda, d)

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
  
def flatten_dict(d, keys):
  return [d[k] for k in keys]